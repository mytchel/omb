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
#include <fs.h>
#include <fssrv.h>

int
fsmountloop(int addr, struct fsmount *mount)
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

    resp->head.ret = ENOIMPL;

    switch (req->head.type) {
    case REQ_getfid:
      if (mount->getfid)
	mount->getfid((struct request_getfid *) req,
		      (struct response_getfid *) resp);
      break;

    case REQ_clunk:
      if (mount->clunk)
	mount->clunk((struct request_clunk *) req,
		     (struct response_clunk*) resp);
      break;

    case REQ_stat:
      if (mount->stat)
	mount->stat((struct request_stat *) req,
		    (struct response_stat *) resp);
      break;

    case REQ_wstat:
      if (mount->wstat)
	mount->wstat((struct request_wstat *) req,
		     (struct response_wstat *) resp);
      break;

    case REQ_open:
      if (mount->open)
	mount->open((struct request_open *) req,
		    (struct response_open *) resp);
      break;

    case REQ_close:
      if (mount->close)
	mount->close((struct request_close *) req,
		     (struct response_close *) resp);
      break;

    case REQ_create:
      if (mount->create)
	mount->create((struct request_create *) req,
		      (struct response_create *) resp);
      break;

    case REQ_remove:
      if (mount->remove)
	mount->remove((struct request_remove *) req,
		      (struct response_remove *) resp);
      break;

    case REQ_map:
      if (mount->map)
	mount->map((struct request_map *) req,
		   (struct response_map *) resp);
      break;

    case REQ_unmap:
      if (mount->unmap)
	mount->unmap((struct request_unmap *) req,
		     (struct response_unmap *) resp);
      break;

    case REQ_read:
      if (mount->read)
	mount->read((struct request_read *) req,
		    (struct response_read *) resp);
      break;

    case REQ_write:
      if (mount->write)
	mount->write((struct request_write *) req,
		     (struct response_write *) resp);
      break;
    }

    if (reply(addr, mid, resp) != OK) {
      return ELINK;
    }
  }
  
  return ERR;
}
