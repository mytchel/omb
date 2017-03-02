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

void
mboxinit(struct mbox *m, reg_t start)
{
  m->swaiting = nil;
  m->rwaiting = nil;

  m->head = 0;
  m->rtail = 0;
  m->wtail = 0;

  m->messages = (struct message *) start;
  m->len = PAGE_SIZE / sizeof(struct message);

  memset(m->messages, 0, m->len * sizeof(struct message));
}

void
mboxclose(struct mbox *m)
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

  if (cas(&mbox->wtail, (void *) otail, (void *) ntail) != OK) {
    return ERR;
  }
  
  memmove(&mbox->messages[otail], m, sizeof(struct message));

  do {
    otail = mbox->rtail;
    ntail = (otail + 1) % mbox->len;
  } while (cas(&mbox->rtail, (void *) otail, (void *) ntail) != OK);

  p = procwlistpop(&mbox->rwaiting);
  if (p != nil) {
    printf("%i waking up %i for message\n", up->pid, p->pid);
    procready(p);
  }
  
  return OK;
}

int
ksendnb(int to, struct message *m)
{
  struct proc *p;

  m->from = up->pid;
  
  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  return mboxaddmessage(&p->mbox, m);
}

int
ksend(int to, struct message *m)
{
  struct proc *p;
  int r;

  m->from = up->pid;

  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  while (true) {
    r = mboxaddmessage(&p->mbox, m);
    if (r == EFULL) {
      if (procwlistadd(&p->mbox.swaiting, up) == OK) {
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

  if (cas(&mbox->head, (void *) h, (void *) n) != OK) {
    return ERR;
  }

  p = procwlistpop(&mbox->swaiting);
  if (p != nil) {
    procready(p);
  }

  return OK;
}

int
krecvnb(struct message *m)
{
  return mboxgetmessage(&up->mbox, m);
}

int
krecv(struct message *m)
{
  int r;

  while (true) {
    printf("%i trying to get message\n", up->pid);
    r = mboxgetmessage(&up->mbox, m);
    if (r == EEMPTY) {
      if (procwlistadd(&up->mbox.rwaiting, up) == OK) {
	printf("%i waiting for message\n", up->pid);
	procrecv();
      }
    } else {
      printf("got message, return\n");
      return r;
    }
  }
}
