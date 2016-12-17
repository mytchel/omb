/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <mem.h>
#include <string.h>
#include <fs.h>
#include <fssrv.h>
#include <am335x/uart.h>

#include "com.h"

volatile struct uart_regs *uart = nil;
struct readreq *readreqs = nil;
int addr = ERR;

static struct stat rootstat = {
  ATTR_wr|ATTR_rd|ATTR_appendonly,
  0,
  UART_LEN,
};

static size_t
puts(uint8_t *s, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++) {
    if (s[i] == '\n') {
      putc('\r');
    }

    putc(s[i]);
  }
	
  return i;
}

static void
addreadreq(struct request_read *req, uint32_t mid)
{
  struct readreq *n, *o;

  n = malloc(sizeof(struct readreq));

  n->len = req->body.len;
  n->mid = mid;
  n->next = nil;
  
  while (true) {
    o = readreqs;

    while (o != nil && o->next != nil)
      o = o->next;

    if (o == nil) {
      if (cas(&readreqs, nil, n)) {
	break;
      }
    } else if (cas(&o->next, nil, n)) {
      break;
    }
  }
}

static void
bwrite(struct request_write *req, struct response_write *resp)
{
  resp->body.len = puts(req->body.data, req->body.len);
  resp->head.ret = OK;
}

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  memmove(&resp->body.stat, &rootstat, sizeof(struct stat));
  resp->head.ret = OK;
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  resp->head.ret = OK;
  resp->body.offset = 0;
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  resp->head.ret = OK;
}

int
mountloop(void)
{
  uint8_t reqbuf[MESSAGELEN], respbuf[MESSAGELEN];
  struct response *resp;
  struct request *req;
  uint32_t mid;

  req = (struct request *) reqbuf;
  resp = (struct response *) respbuf;

  while (true) {
    if (recv(addr, &mid, req) != OK) {
      return ELINK;
    }

    switch (req->head.type) {
    case REQ_stat:
      bstat((struct request_stat *) req,
	    (struct response_stat *) resp);
      break;

    case REQ_open:
      bopen((struct request_open *) req,
	    (struct response_open *) resp);
      break;

    case REQ_close:
      bclose((struct request_close *) req,
	     (struct response_close *) resp);
      break;

    case REQ_write:
      bwrite((struct request_write *) req,
	     (struct response_write *) resp);
      break;

    case REQ_read:
      addreadreq((struct request_read *) req, mid);
      continue;

    default:
      resp->head.ret = ENOIMPL;
    }

    if (reply(addr, mid, resp) != OK) {
      return ELINK;
    }
  }

  return ERR;
}
 
int
commount(char *path)
{
  int f, fd;
  size_t size = UART_LEN;

  addr = serv();
  if (addr < 0) {
    return addr;
  }

  fd = open(path, O_WRONLY|O_CREATE, rootstat.attr);
  if (fd < 0) {
    return -2;
  }

  if (mount(addr, path, rootstat.attr) == ERR) {
    return -3;
  }

  close(fd);

  f = fork(FORK_proc);
  if (f > 0) {
    unserv(addr);
    return f;
  }

  uart = (struct uart_regs *)
    mmap(MEM_io|MEM_rw, size, 0, 0, (void *) UART0);

  if (uart == nil) {
    exit(-4);
  }

  f = fork(FORK_thread);
  if (f < 0) {
    f = -5;
  } else if (f > 0) {
    f = readloop();
  } else {
    f = mountloop();
  }

  exit(f);
}
