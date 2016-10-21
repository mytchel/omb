/*
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
#include <fs.h>
#include <stdarg.h>
#include <string.h>
#include <block.h>

#include "sdmmcreg.h"
#include "sdhcreg.h"
#include "omap_mmc.h"
#include "mmc.h"

static bool
cardscr(struct mmc *mmc)
{
  struct mmc_command cmd;
  uint8_t *p, buf[8];
  int i;

  cmd.cmd = SD_APP_SEND_SCR;
  cmd.resp_type = RESP_LEN_48;
  cmd.arg = 0xaaaaaaaa;
  cmd.data_type = DATA_READ;
  cmd.data_len = 8;
  cmd.data = buf;

  if (!mmchssendappcmd(mmc, &cmd)) {
    return false;
  }

  p = (uint8_t *) mmc->scr;
  for (i = 7; i >= 0; i--) {
    *p++ = buf[i];
  }

  return true;
}

static bool
cardenable4bitmode(struct mmc *mmc)
{
  struct mmc_command cmd;

  if (SCR_SD_BUS_WIDTHS(mmc->scr) & SCR_SD_BUS_WIDTHS_4BIT) {

    cmd.cmd = SD_APP_SET_BUS_WIDTH;
    cmd.resp_type = RESP_LEN_48;
    cmd.data_type = DATA_NONE;
    cmd.arg = 2;

    if (!mmchssendappcmd(mmc, &cmd)) {
      return false;
    }

    mmc->regs->hctl |= MMCHS_SD_HCTL_DTW_4BIT;
    
  } else {
    printf("%s sd card doesn't support 4 bit access!\n", mmc->name);
  }

  return true;
}

static bool
cardenablehighspeedmode(struct mmc *mmc)
{
  return true;
}

bool
sdinit(struct mmc *mmc)
{
  mmc->nblk = SD_CSD_V2_CAPACITY(mmc->csd);

  if (!cardscr(mmc)) {
    printf("%s failed to get scr\n", mmc->name);
    return false;
  }

  if (!cardenable4bitmode(mmc)) {
    printf("%s failed to enable 4 bit mode.\n", mmc->name);
    return false;
  }
 
  if (!cardenablehighspeedmode(mmc)) {
    printf("%s failed to enable high speed mode.\n", mmc->name);
    return false;
  }

  return true;
}