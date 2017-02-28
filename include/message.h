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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MESSAGEBODY (64-sizeof(int))

struct message {
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


#define MEMREQ_kern     1
#define MEMREQ_user     2
#define MEMREQ_max      3

struct memreq_kern {
  int from;
};

STATIC_ASSERT(sizeof(struct memreq_kern) <= MESSAGEBODY,
	      memreq_kern_too_big);

  
struct memresp_kern {
  reg_t start;
};

STATIC_ASSERT(sizeof(struct memresp_kern) <= MESSAGEBODY,
	      memresp_kern_too_big);

struct memreq_user {
  #define MEMREQ_user_ram   1
  #define MEMREQ_user_io    2
  int from;
  int type;
  reg_t pa;
  size_t len;
  int flags;
};

STATIC_ASSERT(sizeof(struct memreq_user) <= MESSAGEBODY,
	      memreq_user_too_big);

struct memresp_user {
  int ret; /* OK or error */
  int code;
};

STATIC_ASSERT(sizeof(struct memresp_user) <= MESSAGEBODY,
	      memresp_user_too_big);

#endif
