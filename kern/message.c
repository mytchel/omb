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

struct mbox *
mboxnew(reg_t start, size_t len)
{
  struct mbox *m;

  m = (struct mbox *) start;

  m->swaiting = nil;
  m->rwaiting = nil;

  m->head = 0;
  m->rtail = 0;
  m->wtail = 0;

  m->len = (len - sizeof(struct mbox)) / sizeof(struct message);
  memset(m->messages, 0, m->len * sizeof(struct message));
  
  return m;
}

void
mboxfree(struct mbox *m)
{
  /* TODO: free stuff */
}

static int
mboxaddmessage(struct mbox *mbox, struct message *m)
{
  size_t otail, ntail;
  struct proc *p;
  
  otail = mbox->wtail;
  ntail = (otail + 1) % mbox->len;

  if (ntail == mbox->head) {
    return EFULL;
  }

  if (!cas(&mbox->wtail, (void *) otail, (void *) ntail)) {
    return ERR;
  }
  
  memmove(&mbox->messages[otail], m, sizeof(struct message));

  do {
    otail = mbox->rtail;
    ntail = (otail + 1) % mbox->len;
  } while (!cas(&mbox->rtail, (void *) otail, (void *) ntail));

  do {
    p = mbox->rwaiting;
  } while (p != nil && !cas(&mbox->rwaiting, p, p->wnext));

  if (p != nil) {
    procready(p);
  }
  
  return OK;
}

int
ksendnb(int to, struct message *m)
{
  struct mbox *mbox;
  struct proc *p;
  
  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  mbox = p->mbox;
  if (mbox != nil) {
    return mboxaddmessage(mbox, m);
  } else {
    return ERR;
  }
}

int
ksend(int to, struct message *m)
{
  struct mbox *mbox;
  struct proc *p;
  int r;

  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  mbox = p->mbox;
  if (mbox == nil) {
    return ERR;
  }

  while (true) {
    r = mboxaddmessage(mbox, m);
    if (r == EFULL) {
      if (procwlistadd(&mbox->swaiting, up) == OK) {
	procsend();
      }
    } else {
      return r;
    }
  }
}

static int
mboxgetmessage(struct mbox *mbox, struct message *m)
{
  struct proc *p;
  size_t h, n;

  h = mbox->head;
  if (h == mbox->rtail) {
    return EEMPTY;
  }

  n = (h + 1) % mbox->len;
    
  memmove(m, &mbox->messages[h], sizeof(struct message));

  if (cas(&mbox->head, (void *) h, (void *) n)) {
    return OK;
  } else {
    return ERR;
  }

  do {
    p = mbox->swaiting;
  } while (p != nil && !cas(&mbox->swaiting, p, p->wnext));

  if (p != nil) {
    procready(p);
  }
}

int
krecvnb(struct message *m)
{
  struct mbox *mbox;

  mbox = up->mbox;
  if (mbox == nil) {
    return ERR;
  }

  return mboxgetmessage(mbox, m);
}

int
krecv(struct message *m)
{
  struct mbox *mbox;
  int r;

  mbox = up->mbox;
  if (mbox == nil) {
    return ERR;
  }
    
  while (true) {
    r = mboxgetmessage(mbox, m);
    if (r == EEMPTY) {
      if (procwlistadd(&mbox->rwaiting, up) == OK) {
	procrecv();
      }
    } else {
      return r;
    }
  }
}
