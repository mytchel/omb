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
mboxnew(reg_t page)
{
  struct message *mm;
  struct mbox *m;

  m = (struct mbox *) page;
  m->head = 0;
  m->tail = 0;

  m->messages = (struct message **) (page + sizeof(struct mbox));
  m->mlen = PAGE_SIZE / 4 / sizeof(struct message);

  m->data = (uint8_t *) m->messages + m->mlen * sizeof(struct message);
  m->dlen = PAGE_SIZE - sizeof(struct mbox)
    - m->mlen * sizeof(struct message);

  mm = (struct message *) m->data;
  mm->rlen = m->dlen - sizeof(struct message);
  mm->len = 0;

  return m;
}

static struct message *
findandfillspot(struct mbox *mbox, void *buf, size_t len)
{
  size_t olen, rlen, nlen;
  struct message *m, *n;

  rlen = roundptr(len);

  m = (struct message *) mbox->data;
  while ((uint8_t *) m < mbox->data + mbox->dlen) {
    olen = m->rlen;
    n = (struct message *) ((uint8_t *) m +
			    sizeof(struct message) +
			    olen);

    if (m->len == 0) {
      /* If next is free join it with this then check again */
      if (((uint8_t *) n < mbox->data + mbox->dlen) && (n->len == 0)) {
	nlen = olen + sizeof(struct message) + n->rlen;
	cas(&m->rlen, (void *) olen, (void *) nlen);
	continue;
      }

      if (rlen <= olen) {
	if (cas(&m->len, (void *) 0, (void *) len)) {

	  /* Create new free chunk with excess if there is any */
	  if (sizeof(struct message) + rlen < olen) {
	    n = (struct message *) ((uint8_t *) m +
				    sizeof(struct message) +
				    rlen);

	    n->rlen = olen - (sizeof(struct message) + rlen);
	    n->len = 0;

	    m->rlen = rlen;
	  }

	  memmove(m->body, buf, len);
	  return m;
	} else {
	  continue;
	}
      }
    }

    m = n;
  }

  return nil;
}

void
kmessagefree(struct message *m)
{
  /* Mark position free */
  m->len = 0;
}

int
ksend(int pid, void *buf, size_t len)
{
  struct message *m, *o;
  size_t otail, ntail;
  struct mbox *mbox;
  struct proc *p;

  p = findproc(pid);
  if (p == nil) {
    return ERR;
  }

  mbox = p->mbox;

  m = findandfillspot(mbox, buf, len);
  if (m == nil) {
    return ERR;
  }

 try:
  otail = mbox->tail;
  ntail = (otail + 1) % mbox->mlen;

  if (ntail == mbox->head) {
    kmessagefree(m);
    return ERR;
  }

  o = mbox->messages[otail];
  if (cas(&mbox->messages[otail], o, m)) {
    if (cas(&mbox->tail, (void *) otail, (void *) ntail)) {
      return OK;
    } else {
      goto try;
    }
  } else {
    goto try;
  }
  
  return ERR;
}

struct message *
krecv(void)
{
  struct message *m;
  struct mbox *mbox;
  size_t h, n;
  
 try:
  mbox = up->mbox;

  h = mbox->head;
  if (h == mbox->tail) {
    return nil;
  }

  n = (h + 1) % mbox->mlen;
  
  m = mbox->messages[h];
  if (cas(&mbox->head, (void *) h, (void *) n)) {
    return m;
  } else {
    goto try;
  }
}
