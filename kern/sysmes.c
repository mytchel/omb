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

#include <head.h>

reg_t
syssendnb(int to, struct message *m)
{
  printf("%i called sendnb\n", up->pid);

  if (validaddr(m, sizeof(struct message), MEM_ro) != OK) {
    return ERR;
  }
  
  return ksendnb(to, m);
}

reg_t
syssend(int to, struct message *m)
{
  printf("%i called send to %i, type %i\n", up->pid, to, m->type);
 
  if (validaddr(m, sizeof(struct message), MEM_ro) != OK) {
    return ERR;
  }

  return ksend(to, m);
}

reg_t
sysrecvnb(struct message *m)
{
  printf("%i called recvnb\n", up->pid);
 
  if (validaddr(m, sizeof(struct message), MEM_rw) != OK) {
    return ERR;
  }

  return krecvnb(m);
}

reg_t
sysrecv(struct message *m)
{
  printf("%i called recv\n", up->pid);
 
  if (validaddr(m, sizeof(struct message), MEM_rw) != OK) {
    return ERR;
  }

  return krecv(m);
}
