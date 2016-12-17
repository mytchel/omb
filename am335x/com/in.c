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

static uint8_t
getdata(void)
{
  uint8_t i;

  for (i = 0; i < 0xf; i++) {
    if (uart->lsr & 1) {
      i = (uint8_t) (uart->hr & 0xff);
      if (i == '\r') {
	i = '\n';
      }

      return i;
    }
  }

  waitintr(UART0_INTR);
  return getdata();
}

int
readloop(void)
{
  struct response_read *resp;
  uint8_t respbuf[MESSAGELEN], buf[1024];
  struct readreq *req;
  uint32_t start, end;

  req = nil;
  start = end = 0;
  resp = (struct response_read *) respbuf;
  
  uart->ier = 1;

  while (true) {
  retry:
    while (end + 1 == start)
      sleep(1000);

    buf[end++] = getdata();

    if (req == nil) {
      do {
	req = readreqs;
	if (req == nil) {
	  goto retry;
	}
      } while (!cas(&readreqs, req, req->next));
    }

    if (end - start < req->len) {
      /* Don't have enough data to serve request */
      continue;
    }

    resp->head.ret = OK;
    resp->body.len = end - start;
    memmove(resp->body.data, &buf[start], end - start);
    start += end - start;

    if (reply(addr, req->mid, resp) != OK) {
      return ERR;
    }

    free(req);
  }

  return OK;
}


