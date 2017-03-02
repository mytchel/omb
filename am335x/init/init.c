/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytchel@openmailbox.org>
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

#include <c.h>
#include <com.h>
#include <mem.h>

int
main(void)
{
  struct memresp resp;
  struct memreq req;

  req.type = COM_MEMREQ;
  req.len = 0x4000;
  req.flags = MEM_r|MEM_w;
  req.pa = 0;

  if (send(1, (struct message *) &req) != OK) {
    exit();
  }

  while (true) {
    if (recv((struct message *) &resp) == OK) {
      if (resp.type == COM_MEMRESP) {
	break;
      }
    } else {
      return ERR;
    }
  }  

  if (mmap(MEM_r|MEM_w, (void *) 0x8000, 0x4000) != OK) {
    return ERR;
  }

  while (true)
    ;
       
  return OK;
}

