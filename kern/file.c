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

#include "head.h"

struct cfile {
  struct lock lock;
  struct bindingfid *fid;

  uint32_t offset;
};

static struct page *
getfidpage(struct bindingfid *fid, size_t offset, int *r);

void
bindingfidfree(struct bindingfid *fid)
{
  struct request_clunk *req;
  struct bindingfid *p, *o;
  uint8_t buf[MESSAGELEN];
  struct message m;

  if (atomicdec(&fid->refs) > 0) {
    return;
  }

  req = (struct request_clunk *) buf;
  req->head.type = REQ_clunk;
  req->head.fid = fid->fid;

  m.message = buf;
  m.reply = buf;
  
  kmessage(fid->binding->addr, &m);
  
  if (fid->parent) {
  remove:
    p = nil;
    for (o = fid->parent->children; o != nil; p = o, o = o->cnext) {
      if (o != fid) continue;
      
      if (p == nil) {
	if (!cas(&fid->parent->children, fid, fid->cnext)) {
	  goto remove;
	}
      } else {
	if (!cas(&p->cnext, fid, fid->cnext)) {
	  goto remove;
	}
      }

      break;
    }

    bindingfidfree(fid->parent);
  } else if (fid->parent == nil && fid->binding != nil) {
    bindingfree(fid->binding);
  }
 
  free(fid);
}

static int
filestatfid(struct bindingfid *fid, struct stat *stat)
{
  struct response_stat *resp;
  struct request_stat *req;
  uint8_t buf[MESSAGELEN];
  struct message m;

  m.message = buf;
  m.reply = buf;

  req = (struct request_stat *) buf;
  resp = (struct response_stat *) buf;
  req->head.type = REQ_stat;
  req->head.fid = fid->fid;

  if (kmessage(fid->binding->addr, &m) != OK) {
    return ELINK;
  } else if (resp->head.ret == OK) {
    memmove(stat, &resp->body.stat, sizeof(struct stat));
    return OK;
  } else {
    return resp->head.ret;
  }
}

static struct bindingfid *
findfileindir(struct bindingfid *fid, char *name, int *ret)
{
  struct response_getfid *resp;
  struct request_getfid *req;
  struct bindingfid *nfid;
  uint8_t buf[MESSAGELEN];
  struct message m;

  for (nfid = fid->children; nfid != nil; nfid = nfid->cnext) {
    if (strncmp(nfid->name, name, NAMEMAX)) {
      atomicinc(&nfid->refs);
      return nfid;
    }
  }

  m.message = buf;
  m.reply = buf;

  req = (struct request_getfid *) buf;
  resp = (struct response_getfid *) buf;
  
  req->head.type = REQ_getfid;
  req->head.fid = fid->fid;

  req->body.len = strlcpy(req->body.name, name, NAMEMAX);

  if (kmessage(fid->binding->addr, &m) != OK) {
    *ret = ELINK;
    return nil;
  } else if (resp->head.ret != OK) {
    *ret = resp->head.ret;
    return nil;
  }

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *ret = ENOMEM;
    return nil;
  }

  nfid->refs = 1;
  nfid->binding = fid->binding;

  nfid->fid = resp->body.fid;
  nfid->attr = resp->body.attr;
  strlcpy(nfid->name, name, NAMEMAX);

  atomicinc(&fid->refs);
  nfid->parent = fid;
  nfid->children = nil;

  do {
    nfid->cnext = fid->children;
  } while (!cas(&fid->children, nfid->cnext, nfid));

  return nfid;
}

struct bindingfid *
findfile(struct path *path, int *ret)
{
  struct bindingfid *nfid, *fid;
  struct bindingl *b;
  struct path *p;
 
  b = ngroupfindbindingl(up->ngroup, up->root);
  if (b == nil) {
    atomicinc(&up->root->refs);
    return up->root;
  }

  fid = b->rootfid;
  atomicinc(&fid->refs);

  p = path;
  while (p != nil) {
    if (!(fid->attr & ATTR_dir)) {
      bindingfidfree(fid);
      *ret = ENOFILE;
      return nil;
    }

    nfid = findfileindir(fid, p->s, ret);
    if (*ret == ENOCHILD && p->next == nil) {
      return fid;
    } else if (*ret != OK) {
      bindingfidfree(fid);
      return nil;
    }

    /* Free parent binding, should not clunk it as we have a 
     * reference to its child. */
    bindingfidfree(fid);

    b = ngroupfindbindingl(up->ngroup, nfid);
    if (b != nil) {
      bindingfidfree(nfid);
      fid = b->rootfid;
      atomicinc(&fid->refs);
    } else {
      /* Use the fid from parent */
      fid = nfid;
    }

    p = p->next;
  }

  return fid;
}

static struct bindingfid *
filecreate(struct bindingfid *parent,
	   char *name, uint32_t cattr,
	   int *ret)
{
  struct response_create *resp;
  struct request_create *req;
  struct bindingfid *nfid;
  uint8_t buf[MESSAGELEN];
  struct message m;

  m.message = buf;
  m.reply = buf;

  req = (struct request_create *) buf;
  resp = (struct response_create *) buf;
  
  req->head.type = REQ_create;
  req->head.fid = parent->fid;

  req->body.attr = cattr;
  req->body.len = strlcpy(req->body.name, name, NAMEMAX);
  
  if (kmessage(parent->binding->addr, &m) != OK) {
    *ret = ELINK;
    return nil;
  } else if (resp->head.ret != OK) {
    *ret = resp->head.ret;
    return nil;
  } 

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *ret = ENOMEM;
    return nil;
  }

  nfid->refs = 1;
  nfid->binding = parent->binding;

  nfid->fid = resp->body.fid;

  nfid->attr = cattr;
  strlcpy(nfid->name, name, NAMEMAX);

  atomicinc(&parent->refs);
  nfid->parent = parent;
  nfid->children = nil;

  do {
    nfid->cnext = parent->children;
  } while (!cas(&parent->children, nfid->cnext, nfid));

  return nfid;
}

int 
filestat(struct path *path, struct stat *stat)
{
  struct bindingfid *fid;
  int ret;

  ret = OK;
  fid = findfile(path, &ret);
  if (ret != OK) {
    return ret;
  }

  ret = filestatfid(fid, stat);

  bindingfidfree(fid);

  return ret;
}

static bool
checkmode(uint32_t attr, uint32_t mode)
{
  if ((mode & O_WRONLY) && !(attr & ATTR_wr)) {
    return false;
  } else if ((mode & O_RDONLY) && !(attr & ATTR_rd)) {
    return false;
  } else if ((mode & O_DIR) && !(attr & ATTR_dir)) {
    return false;
  } else if ((mode & O_FILE) && (attr & ATTR_dir)) {
    return false;
  } else {
    return true;
  }
}

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *ret)
{
  struct bindingfid *fid, *nfid;
  struct response_open *resp;
  struct request_open *req;
  uint8_t buf[MESSAGELEN];
  struct cfile *cfile;
  struct message m;
  struct chan *c;
  struct path *p;

  fid = findfile(path, ret);
  if (*ret != OK) {
    if (cmode == 0) {
      if (*ret == ENOCHILD || fid == nil) {
	*ret = ENOFILE;
      }
      return nil;
    } else if (!(fid->attr & ATTR_wr)) {
      bindingfidfree(fid);
      *ret = EMODE;
      return nil;
    } else {
      /* Create the file first. */
      for (p = path; p != nil && p->next != nil; p = p->next);
      if (p == nil) {
	bindingfidfree(fid);
	return nil;
      }

      *ret = OK;
      nfid = filecreate(fid, p->s, cmode, ret);
      if (*ret != OK) {
	bindingfidfree(fid);
	return nil;
      }

      bindingfidfree(fid);
      
      fid = nfid;
    }
  }

  if (!checkmode(fid->attr, mode)) {
    *ret = EMODE;
    bindingfidfree(fid);
    return nil;
  }

  m.message = buf;
  m.reply = buf;

  req = (struct request_open *) buf;
  resp = (struct response_open *) buf;
  
  req->head.type = REQ_open;
  req->head.fid = fid->fid;
  req->body.mode = mode;
  
  if (kmessage(fid->binding->addr, &m) != OK) {
    *ret = ELINK;
    bindingfidfree(fid);
    return nil;
  } else if (resp->head.ret != OK) {
    *ret = resp->head.ret;
    bindingfidfree(fid);
    return nil;
  }

  c = channew(CHAN_file, mode);
  if (c == nil) {
    bindingfidfree(fid);
    *ret = ENOMEM;
    return nil;
  }
	
  cfile = malloc(sizeof(struct cfile));
  if (cfile == nil) {
    bindingfidfree(fid);
    chanfree(c);
    *ret = ENOMEM;
    return nil;
  }
	
  c->aux = cfile;

  cfile->fid = fid;
  cfile->offset = resp->body.offset;

  return c;
}

int
fileremove(struct path *path)
{
  struct response_remove *resp;
  struct request_remove *req;
  uint8_t buf[MESSAGELEN];
  struct bindingfid *fid;
  struct message m;
  int ret;

  req = OK;
  fid = findfile(path, &ret);
  if (ret != OK) {
    return ret;
  }

  if (fid->refs != 1) {
    return ERR;
  }

  m.message = buf;
  m.reply = buf;

  req = (struct request_remove *) buf;
  resp = (struct response_remove *) buf;
  
  req->head.type = REQ_remove;
  req->head.fid = fid->fid;

  if (kmessage(fid->binding->addr, &m) != OK) {
    ret = ELINK;
  } else {
    ret =  resp->head.ret;
  }

  bindingfidfree(fid);
  return ret;
}

static int
filebigread(struct cfile *cfile, void *buf, size_t n)
{
  size_t l, t, poff;
  struct page *p;
  int r;
  
  r = OK;
  l = 0;
  while (l < n) {
    poff = PAGE_ALIGN(cfile->offset + l - (PAGE_SIZE - 1));

    p = getfidpage(cfile->fid, poff, &r);
    if (p == nil) {
      return r;
    }

    t = n - l > PAGE_SIZE ? PAGE_SIZE : n - l;

    memmove(buf + l, (void *) (p->pa + (cfile->offset + l - poff)),
	    t);

    pagefree(p);

    l += t;
  }

  return l;
}

static int
filelittleread(struct cfile *cfile, void *buf, size_t n)
{
  struct response_read *resp;
  struct request_read *req;
  uint8_t mbuf[MESSAGELEN];
  struct message m;

  req = (struct request_read *) mbuf;
  resp = (struct response_read *) mbuf;

  m.message = mbuf;
  m.reply = mbuf;
  
  req->head.type = REQ_read;
  req->head.fid = cfile->fid->fid;
  req->body.offset = cfile->offset;
  req->body.len = n;

  if (kmessage(cfile->fid->binding->addr, &m) != OK) {
    return ERR;
  } else if (resp->head.ret == OK) {

    if (resp->body.len > n) {
      memmove(buf, resp->body.data, n);
      return n;
    } else {
      memmove(buf, resp->body.data, resp->body.len);
      return resp->body.len;
    }

  } else {
    return resp->head.ret;
  }
}

int
fileread(struct chan *c, void *buf, size_t n)
{
  struct cfile *cfile;
  int r;
  
  cfile = (struct cfile *) c->aux;

  lock(&cfile->lock);

  if (n > WRITEDATAMAX) {
    r = filebigread(cfile, buf, n);
  } else {
    r = filelittleread(cfile, buf, n);
  }

  if (r > 0) {
    cfile->offset += r;
  }
  
  unlock(&cfile->lock);
  return r;
}

static int
filebigwrite(struct cfile *cfile, void *buf, size_t n)
{
  struct page *p;
  size_t l, t, poff;
  int r;
  
  r = OK;
  l = 0;
  while (l < n) {
    poff = PAGE_ALIGN(cfile->offset + l - (PAGE_SIZE - 1));

    p = getfidpage(cfile->fid, poff, &r);
    if (p == nil) {
      return r;
    }

    t = n - l > PAGE_SIZE ? PAGE_SIZE : n - l;

    memmove((void *) (p->pa + (cfile->offset + l - poff)),
	    buf + l, t);

    pagefree(p);

    l += t;
  }

  return l;
}

static int
filelittlewrite(struct cfile *cfile, void *buf, size_t n)
{
  struct response_write *resp;
  struct request_write *req;
  uint8_t mbuf[MESSAGELEN];
  struct message m;

  req = (struct request_write *) mbuf;
  resp = (struct response_write *) mbuf;

  m.message = mbuf;
  m.reply = mbuf;
  
  req->head.type = REQ_write;
  req->head.fid = cfile->fid->fid;
  req->body.offset = cfile->offset;
  req->body.len = n;
  memmove(req->body.data, buf, n);

  if (kmessage(cfile->fid->binding->addr, &m) != OK) {
    return ERR;
  } else if (resp->head.ret == OK) {
    return resp->body.len;
  } else {
    return resp->head.ret;
  }
}

int
filewrite(struct chan *c, void *buf, size_t n)
{
  struct cfile *cfile;
  int r;
  
  cfile = (struct cfile *) c->aux;

  lock(&cfile->lock);

  if (n > WRITEDATAMAX) {
    r = filebigwrite(cfile, buf, n);
  } else {
    r = filelittlewrite(cfile, buf, n);
  }

  if (r > 0) {
    cfile->offset += r;
  }
  
  unlock(&cfile->lock);
  return r;
}

int
fileseek(struct chan *c, size_t offset, int whence)
{
  struct cfile *cfile;

  cfile = (struct cfile *) c->aux;
  lock(&cfile->lock);

  switch(whence) {
  case SEEK_SET:
    cfile->offset = offset;
    break;
  case SEEK_CUR:
    cfile->offset += offset;
    break;
  default:
    unlock(&cfile->lock);
    return ERR;
  }

  unlock(&cfile->lock);
  return OK;
}

static struct page *
getfidnewpage(struct bindingfid *fid, size_t offset, int *r)
{
  struct response_map *resp;
  struct request_map *req;
  uint8_t buf[MESSAGELEN];
  struct pagel *po;
  struct message m;

  m.message = buf;
  m.reply = buf;
  req = (struct request_map *) buf;
  resp = (struct response_map *) buf;
  
  req->head.fid = fid->fid;
  req->head.type = REQ_map;
  req->body.offset = offset;

  if (kmessage(fid->binding->addr, &m) != OK) {
    *r = ERR;
    return nil;
  } else if (resp->head.ret != OK) {
    *r = resp->head.ret;
    return nil;
  }

  for (po = m.replyer->mgroup->pages; po != nil; po = po->next) {
    if (po->va == (reg_t) resp->body.addr) {
      return po->p;
    }
  }

  *r = ERR;
  return nil;
}

static void
fidpagefree(struct page *p)
{
  struct request_unmap *req;
  uint8_t buf[MESSAGELEN];
  struct bindingfid *fid;
  struct pagel *po, *pp;
  struct message m;

  printf("%i is going to unmap a page from a mapped file!!!\n", up->pid);
  fid = (struct bindingfid *) p->aux;

  m.message = buf;
  m.reply = buf;
  req = (struct request_unmap *) buf;

  req->head.type = REQ_unmap;
  req->head.fid = fid->fid;

 retry:
  pp = nil;
  for (po = fid->pages; po != nil; pp = po, po = po->next) {
    if (po->p == p) {
      printf("found page! va = 0x%h\n", po->va);

      if (pp == nil) {
	if (!cas(&fid->pages, po, po->next)) {
	  printf("remove failed\n");
	  goto retry;
	}
      } else if (!cas(&pp->next, po, po->next)) {
	printf("remove failed\n");
	goto retry;
      }

      req->body.offset = po->va;
      printf("send message\n");
      kmessage(fid->binding->addr, &m);
      free(po);
      break;
    }
  }
}

static struct page *
getfidpage(struct bindingfid *fid, size_t offset, int *r)
{
  struct pagel *pl, *po, *pn;
  struct page *p;

  for (pl = fid->pages; pl != nil; pl = pl->next) {
    if (pl->va == offset) {
      return pl->p;
    }
  }

  p = getfidnewpage(fid, offset, r);
  if (*r != OK) {
    return nil;
  }

  p->aux = fid;
  p->hook = &fidpagefree;
  p->hookrefs = atomicinc(&p->refs);

  pn = wrappage(p, offset, true, false);

 retry:
  po = fid->pages;
  if (po == nil || po->va > offset) {
    pn->next = po;
    if (!cas(&fid->pages, po, pn)) {
      goto retry;
    }
  } else {
    while (po != nil) {
      if (po->next == nil || po->next->va > offset) {
	pn->next = po->next;
	if (cas(&po->next, pn->next, pn)) {
	  break;
	} else {
	  goto retry;
	}
      } else {
	po = po->next;
      }
    }
  }

  atomicinc(&p->refs);
  return p;
}

struct pagel *
getfilepages(struct chan *c, size_t offset, size_t len, bool rw)
{
  struct pagel *pages, *pn;
  struct cfile *cfile;
  struct page *p;
  size_t off;
  int r;

  cfile = (struct cfile *) c->aux;
  
  off = 0;
  pages = pn = nil;
  r = OK;
  
  while (off < len) {
    p = getfidpage(cfile->fid, offset + off, &r);
    if (r != OK) {
      pagelfree(pages);
      return nil;
    }

    if (pn == nil) {
      pages = pn = wrappage(p, offset + off, rw, false);
    } else {
      pn->next = wrappage(p, offset + off, rw, false);
      pn = pn->next;
    }

    if (pn == nil) {
      pagefree(p);
      pagelfree(pages);
      return nil;
    }
    
    off += PAGE_SIZE;
  }

  return pages;
}

void
fileclose(struct chan *c)
{
  struct request_close *req;
  uint8_t buf[MESSAGELEN];
  struct cfile *cfile;
  struct message m;

  cfile = (struct cfile *) c->aux;

  lock(&cfile->lock);

  m.message = buf;
  m.reply = buf;
  req = (struct request_close *) buf;
  
  req->head.type = REQ_close;
  req->head.fid = cfile->fid->fid;

  kmessage(cfile->fid->binding->addr, &m);

  bindingfidfree(cfile->fid);
  free(cfile);
}

struct chantype devfile = {
  &fileread,
  &filewrite,
  &fileseek,
  &fileclose,
};
