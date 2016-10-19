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

struct fat_file {
  uint32_t fid;
  char name[NAMEMAX];
  size_t opened;
  uint32_t fatattr, attr;
  uint32_t size;
  uint32_t startcluster;
  struct fat_file *next;
};


struct fat_bs_ext32 {
  uint8_t spf[4];
  uint8_t extflags[2];
  uint8_t version[2];
  uint8_t rootcluster[4];
  uint8_t info[2];
  uint8_t backup[2];
  uint8_t res_0[12];
  uint8_t drvn;
  uint8_t res_1;
  uint8_t sig;
  uint8_t volid[4];
  uint8_t vollabel[11];
  uint8_t fattypelabel[8];
  uint8_t bootcode[420];
}__attribute__((packed));


struct fat_bs_ext16
{
  uint8_t drvn;
  uint8_t res_1;
  uint8_t sig;
  uint8_t volid[2];
  uint8_t vollabel[11];
  uint8_t fattypelabel[8];
  uint8_t bootcode[448];
}__attribute__((packed));


struct fat_bs {
  uint8_t jmp[3];
  uint8_t oem[8];
  uint8_t bps[2];
  uint8_t spc;
  uint8_t res[2];
  uint8_t nft;
  uint8_t rde[2];
  uint8_t sc16[2];
  uint8_t mdt;
  uint8_t spf[2];
  uint8_t spt[2];
  uint8_t heads[2];
  uint8_t hidden[4];
  uint8_t sc32[4];

  union {
    struct fat_bs_ext32 high;
    struct fat_bs_ext16 low;
  } ext;
  
  uint8_t boot_signature[2];

}__attribute__((packed));


#define FAT_ATTR_read_only        0x01
#define FAT_ATTR_hidden           0x02
#define FAT_ATTR_system           0x04
#define FAT_ATTR_volume_id        0x08
#define FAT_ATTR_directory        0x10
#define FAT_ATTR_archive          0x20
#define FAT_ATTR_lfn \
  (FAT_ATTR_read_only|FAT_ATTR_hidden| \
   FAT_ATTR_system|FAT_ATTR_volume_id)


struct fat_lfn {
  uint8_t order;
  uint8_t first[10];
  uint8_t attr;
  uint8_t type;
  uint8_t chksum;
  uint8_t next[12];
  uint8_t zero[2];
  uint8_t final[4];
}__attribute__((packed));


struct fat_dir_entry {
  uint8_t name[8];
  uint8_t ext[3];
  uint8_t attr;
  uint8_t reserved1;
  uint8_t create_dseconds;
  uint8_t create_time[2];
  uint8_t create_date[2];
  uint8_t last_access[2];
  uint8_t cluster_high[2];
  uint8_t mod_time[2];
  uint8_t mod_date[2];
  uint8_t cluster_low[2];
  uint8_t size[4];
}__attribute__((packed));


typedef enum { FAT12, FAT16, FAT32 } fat_t;

struct fat {
  int fd;
  fat_t type;

  uint32_t nfid;
  struct fat_file *files;

  uint32_t bps;
  uint32_t spc;

  uint32_t spf;
  uint8_t nft;

  uint32_t nclusters;
  uint32_t nsectors;

  uint32_t rde;
  uint32_t rootdir;
  uint32_t dataarea;

  uint8_t *table;

  struct fat_bs bs;
};

struct fat *
fatinit(int fd);

struct fat_file *
fatfilefind(struct fat *fat, struct fat_file *parent,
	    char *name, int *err);

struct fat_file *
fatfindfid(struct fat *fat, uint32_t fid);

void
fatclunkfid(struct fat *fat, uint32_t fid);

int
fatreadfile(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len);

int
fatreaddir(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len);
