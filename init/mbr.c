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
#include <string.h>
#include <block.h>

#define MBR_part_len 16
#define MBR_part1    0x1be
#define MBR_part2    0x1ce
#define MBR_part3    0x1de
#define MBR_part4    0x1ee

struct mbrpartition {
  uint8_t status;
  uint8_t start_chs[3];
  uint8_t type;
  uint8_t end_chs[3];
  uint32_t lba;
  uint32_t sectors;
} __attribute__((__packed__));

struct mbr {
  uint8_t bootstrap[446];
  struct mbrpartition parts[4];
  uint8_t bootsignature[2];
};

struct block {
  uint32_t blk, nblk;
  uint8_t *buf;
  struct block *next;
};

struct partition {
  uint8_t type;
  
  uint8_t lname;
  char name[NAMEMAX];
  
  struct stat stat;

  uint32_t lba, sectors;

  struct block *blocks;
  struct mbrpartition *mbr;
};

static struct stat rootstat = {
  ATTR_rd|ATTR_dir,
  0,
};

static uint8_t *rootbuf = nil;

static struct blkdevice *device = nil;
static char path[1024];

static struct mbr mbr;
struct partition raw;
struct partition parts[4];

static void
initpart(struct partition *p, char *name)
{
  p->lname = strlcpy(p->name, name, NAMEMAX);
  p->stat.attr = ATTR_wr|ATTR_rd;
}

static void
initparts(void)
{
  char name[NAMEMAX] = "a";
  int i;

  for (i = 0; i < 4; i++) {
    initpart(&parts[i], name);
    name[0]++;

    parts[i].mbr = &mbr.parts[i];
    parts[i].type = 0;
    parts[i].blocks = nil;
  }

  initpart(&raw, "raw");

  raw.stat.size = device->nblk * device->blksize;
  raw.stat.dsize = raw.stat.size;
  raw.type = 1;
  raw.lba = 0;
  raw.sectors = device->nblk;
  raw.mbr = nil;
  raw.blocks = nil;
}

static void
updatembr(void)
{
  uint8_t *buf;
  int i;

  if (!(device->read(0, (uint8_t *) &mbr))) {
    printf("mbr read failed\n");
    exit(ERR);
  }

  rootstat.size = 1;
  rootstat.dsize = 1 + raw.lname;

  for (i = 0; i < 4; i++) {
    parts[i].type = parts[i].mbr->type;
    if (parts[i].type != 0) {
      rootstat.dsize += 1 + parts[i].lname;
      rootstat.size++;

      memmove(&parts[i].lba, &parts[i].mbr->lba,
	      sizeof(uint32_t));

      memmove(&parts[i].sectors, &parts[i].mbr->sectors,
	      sizeof(uint32_t));

      parts[i].stat.size = parts[i].sectors * device->blksize;
      parts[i].stat.dsize = parts[i].stat.size;
    }
  }

  if (rootbuf != nil) {
    free(rootbuf);
  }
  
  rootbuf = malloc(rootstat.dsize);
  if (rootbuf == nil) {
    exit(-1);
  }

  buf = rootbuf;

  memmove(buf, &raw.lname, sizeof(uint8_t));
  buf += sizeof(uint8_t);
  memmove(buf, raw.name, sizeof(uint8_t) * raw.lname);
  buf += sizeof(uint8_t) * raw.lname;

  for (i = 0; i < 4; i++) {
    if (parts[i].type != 0) {
      memmove(buf, &parts[i].lname, sizeof(uint8_t));
      buf += sizeof(uint8_t);
      memmove(buf, parts[i].name, parts[i].lname);
      buf += parts[i].lname;

      printf("part %s/%s of type %h\n",
	     path, parts[i].name, parts[i].type);
    }
  }
}

static struct partition *
fidtopart(uint32_t fid)
{
  if (fid == 1) {
    return &raw;
  } else {
    return &parts[fid - 2];
  }
}

static void
bgetfid(struct request_getfid *req, struct response_getfid *resp)
{
  int i;

  if (strncmp(raw.name, req->body.name, NAMEMAX)) {
    resp->body.fid = 1;
    resp->body.attr = raw.stat.attr;
    resp->head.ret = OK;
    return;
  } else {
    for (i = 0; i < 4; i++) {
      if (parts[i].type != 0 &&
	  strncmp(parts[i].name, req->body.name, NAMEMAX)) {

	resp->body.attr = parts[i].stat.attr;
	resp->body.fid = i + 2;
	resp->head.ret = OK;
	return;
      }
    }
  } 

  resp->head.ret = ENOFILE;
}

static void
bclunk(struct request_clunk *req, struct response_clunk *resp)
{
  resp->head.ret = OK;
}

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  struct partition *part;
  struct stat *s = nil;

  if (req->head.fid == ROOTFID) {
    s = &rootstat;
  } else {
    part = fidtopart(req->head.fid);
    s = &(part->stat);
  }

  if (s == nil) {
    resp->head.ret = ENOFILE;
  } else { 
    memmove(&resp->body.stat, s, sizeof(struct stat));
    resp->head.ret = OK;
  }
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  resp->head.ret = OK;
  resp->body.offset = 0;
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  resp->head.ret = OK;
}

static void
readroot(struct request_read *req, struct response_read *resp,
	 uint32_t offset, uint32_t len)
{
  if (offset + len >= rootstat.dsize) {
    len = rootstat.dsize - offset;
  }

  if (len > 0) {
    memmove(resp->body.data, rootbuf + offset, len);
  }
  
  resp->body.len = len;
  resp->head.ret = OK;
}

static void
bread(struct request_read *req, struct response_read *resp)
{
  if (req->head.fid == ROOTFID) {
    readroot(req, resp, req->body.offset, req->body.len);
  } else {
    resp->head.ret = ERR;
  }
}

static void
bmap(struct request_map *req, struct response_map *resp)
{
  struct block *n, *prev, *next;
  struct partition *part;
  uint32_t i, blk;

  if (req->head.fid == ROOTFID) {
    resp->head.ret= ERR;
    return;
  }

  part = fidtopart(req->head.fid);
  if (part == nil) {
    resp->head.ret = ERR;
    return;
  }

  blk = req->body.offset / device->blksize;

  next = nil;
  prev = part->blocks;
  while (prev != nil) {
    if (prev->blk > blk) {
      next = prev;
      prev = nil;
      break;
    } else if (prev->next == nil || prev->next->blk > blk) {
      next = prev->next;
      break;
    } else {
      prev = prev->next;
    }
  }

  n = malloc(sizeof(struct block));
  if (n == nil) {
    /* Probably not the best way to handle this. */
    resp->head.ret = ENOMEM;
    return;
  }
   
  n->blk = blk;
  n->nblk = PAGE_SIZE / device->blksize;

  printf("%s mapping from block %i to %i\n", device->name, part->lba + n->blk, part->lba + n->blk + n->nblk);

  n->buf = mmap(MEM_ram|MEM_rw, PAGE_SIZE, 0, 0, nil);
  if (n->buf == nil) {
    resp->head.ret = ENOMEM;
    free(n);
    return;
  }

  for (i = 0; i < n->nblk && n->blk + i < part->sectors; i++) {
    printf("%s reading block %i!\n", device->name,
	   part->lba + n->blk + i);
    if (!device->read(part->lba + n->blk + i,
		      &n->buf[i * device->blksize])) {
      printf("%s error reading block %i!\n", device->name,
	     part->lba + n->blk + i);
      munmap(n->buf, PAGE_SIZE);
      free(n);
      resp->head.ret = ERR;
      return;
    }
  }

  n->next = next;
  if (prev == nil) {
    part->blocks = n;
  } else {
    prev->next = n;
  }
 
  n->nblk = i;
  
  resp->body.addr = n->buf;
  resp->head.ret = OK;
}

static void
bunmap(struct request_unmap *req, struct response_unmap *resp)
{
  struct block *n, *prev;
  struct partition *part;
  uint32_t i, blk;

  printf("%s unmapping page %i\n", device->name, req->body.offset);
  
  if (req->head.fid == ROOTFID) {
    resp->head.ret= ERR;
    return;
  }

  part = fidtopart(req->head.fid);
  if (part == nil) {
    resp->head.ret = ERR;
    return;
  }

  blk = req->body.offset / device->blksize;

  prev = nil;
  for (n = part->blocks; n != nil; n = n->next) {
    if (n->blk != blk) continue;
    break;
  }

  if (n == nil) {
    resp->head.ret = ERR;
    return;
  }
  
  if (prev == nil) {
    part->blocks = n->next;
  } else {
    prev->next = n->next;
  }
    
  for (i = 0; i < n->nblk; i++) {
    printf("%s writing block %i!\n", device->name,
	   part->lba + n->blk + i);
    if (!device->write(part->lba + n->blk + i,
		       &n->buf[i * device->blksize])) {
      printf("%s error writing block %i!\n", device->name,
	     part->lba + n->blk + i);
    }
  }

  munmap(n->buf, PAGE_SIZE);
  free(n);
 
  resp->head.ret = OK;
}

static struct fsmount fsmount = {
  .getfid = &bgetfid,
  .clunk = &bclunk,
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .map = &bmap,
  .unmap = &bunmap,
  .read = &bread,
};

int
mbrmount(struct blkdevice *d, uint8_t *dir)
{
  int addr, fd;

  device = d;
  snprintf(path, sizeof(path), "%s/%s", dir, device->name);

  initparts();
  updatembr();

  fd = open(path, O_RDONLY|O_CREATE, ATTR_dir|ATTR_rd);
  if (fd < 0) {
    printf("%s failed to create %s.\n", device->name, path);
    return -1;
  }

  addr = serv();
  if (addr < 0) {
    return addr;
  }

  if (mount(addr, path, ATTR_rd|ATTR_dir) == ERR) {
    return -3;
  }

  return fsmountloop(addr, &fsmount);
}

