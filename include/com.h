#/*
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

#ifndef _COM_H_
#define _COM_H_

#define MESSAGESIZE 64
#define MESSAGEBODY (64-sizeof(int)-sizeof(int))

struct message {
  int from;
  int type;
  uint8_t body[MESSAGEBODY];
};


int
sendnb(int to, struct message *m);

int
send(int to, struct message *m);

int
recvnb(struct message *m);

int
recv(struct message *m);


#define ASSERT_MESSAGE_SIZE(m) \
  STATIC_ASSERT(sizeof(struct m) == MESSAGESIZE, \
		too_big##m)

#define COM_MEMREQ   1
struct memreq {
  int from;
  int type;
  size_t len;
  int flags;
  reg_t pa;
  uint8_t extra[MESSAGESIZE
		- sizeof(int)
		- sizeof(int)
		- sizeof(size_t)
		- sizeof(int)
		- sizeof(reg_t)];
};

ASSERT_MESSAGE_SIZE(memreq);

#define COM_MEMRESP   2
struct memresp {
  int from;
  int type;
  int ret;
  uint8_t extra[MESSAGESIZE
		- sizeof(int)
		- sizeof(int)
		- sizeof(int)];
};

ASSERT_MESSAGE_SIZE(memresp);

#endif
