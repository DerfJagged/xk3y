/*
* (C) Copyright 2010 NXP Semiconductors
*
* See file CREDITS for list of people who contributed to this
* project.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/

#include <common.h>
#include <linux/mtd/mtd.h>
#include <nand.h>

#if defined(CONFIG_CMD_NAND)
#include <asm/arch/sysreg.h>
#include <asm/arch/nand.h>

#define LPC313x_NAND_BPARAM_PAGE_SZ	256
#define TIMEOUT_WAIT_ERASE		500

/* Max Calculation: (14*510) + (1*509) */
#define MAX_NUM_BAD_BLOCKS		7649

extern const NAND_BOOT_CFG_PARAMS_T nandbootparams[];
volatile unsigned int mem_based_bbt[MAX_NUM_BAD_BLOCKS + 1];

static void wait_on_int (u32 int_mask, u32 tmo)
{
  unsigned long time_start = get_timer(0);

  /* Wait for MLC NAND ready */
  while (!(NAND_CTRL->irq_status_raw & int_mask)) {
    if ( (get_timer(0) - time_start) >= tmo) {
      printf ("\nbailing at timeout status 0x%08lx bits 0x%08x\n",
        NAND_CTRL->irq_status_raw, int_mask);
      break;
    }
  }
}

static int is_bad_block(unsigned int blkaddr)
{
  unsigned int i = 0;
  while(mem_based_bbt[i] != 0xFFFFFFFF) {
    if(blkaddr == mem_based_bbt[i])
      return 1;
    i++;
  }
  return 0;
}

static int nand_erase_block(unsigned long offset)
{
  int count;

  NAND_CTRL->irq_status_raw = NAND_IRQ_RB1_POS_EDGE;
  NAND_CTRL->set_cmd= NAND_CMD_ERASE1;

  for (count=0; count< nandbootparams->amount_of_erase_address_bytes;
    count++, offset >>= 8 )
    NAND_CTRL->set_addr = (u8)( offset & 0xff );
  NAND_CTRL->set_cmd = NAND_CMD_ERASE2;

  wait_on_int(NAND_IRQ_RB1_POS_EDGE, TIMEOUT_WAIT_ERASE);

  NAND_CTRL->set_cmd = NAND_CMD_STATUS;
  udelay(5);
  if (NAND_CTRL->read_data & NAND_STATUS_FAIL) {
    return 0;
  }

  return 1;
}

static int erase_all_blocks(void)
{
  unsigned int page_num;
  unsigned int blksize = ( nandbootparams->page_size_in_bytes *
    nandbootparams->pages_per_block);
  unsigned char *wbuffer = (unsigned char *)(0x30000000);
  unsigned char *rbuffer = (unsigned char *)(0x30020000);

  size_t len = 0;
  int ret = 0;
  unsigned int i = 0;
  unsigned int bad_blk_cnt = 0;

  memset(wbuffer,0xa5a5a5a5,(128 * 1024));

  for( i = 0, page_num = nandbootparams->pages_per_block;
    i < nandbootparams->nbr_of_blocks;
    i++,page_num += nandbootparams->pages_per_block ) {
      NAND_CTRL->set_ce = NAND_SETCE_WP | NAND_SETCE_CV(0);
      if(nand_erase(&nand_info[0],(i * blksize),nand_info[0].erasesize) == 0) {

        len = 128 * 1024;
        ret = nand_write(&nand_info[0], (i * blksize), &len, wbuffer);
        if(ret) {
          printf("nand write failed for block%d\n",i);
          return 1;
        }
        printf("\\\r");

        len = 128 * 1024;
        memset(rbuffer, 0x0,(128 * 1024));
        ret = nand_read(&nand_info[0], (i * blksize), &len, rbuffer);
        if(ret) {
          printf("nand read failed for block%d\n",i);
          return 1;
        }

        printf("-\r");
        if(memcmp(wbuffer, rbuffer, (128 * 1024)) && (i != 0)) {
          bad_blk_cnt++;
          mem_based_bbt[bad_blk_cnt] = (i * blksize);
        }
      }
      else {
        bad_blk_cnt++;
        mem_based_bbt[bad_blk_cnt] = (i * blksize);
      }
      printf("/\r");

  }
  mem_based_bbt[0] = bad_blk_cnt;
  printf("\n");

  /* Write NAND params and Bad Block Table in Block0 */
  write_params_bbt(mem_based_bbt);

  return 0;
}

static void erase_blocks_skipping_blk0_bbt_blks(void)
{
  unsigned int page_num;
  unsigned char bad = 0;
  unsigned int blkoff = 0;

  /* Now, Erase all the blocks excluding Factory Bad blocks */
  printf("Erasing all the blocks skipping Factory Bad Blocks,");
  printf("last four blocks of MTD BBT and Block 0\n");
  NAND_CTRL->set_ce = NAND_SETCE_WP | NAND_SETCE_CV(0);

  for(page_num = nandbootparams->pages_per_block, blkoff = 1;
    page_num < (nandbootparams->nbr_of_blocks *
    nandbootparams->pages_per_block);
  page_num += nandbootparams->pages_per_block ,blkoff++) {
    /* Erase Block */
    bad = is_bad_block(blkoff * (nandbootparams->page_size_in_bytes *
      nandbootparams->pages_per_block));
    if(!bad) {
      nand_erase_block(page_num);
    }
  }
}

static int get_bbt_info (void)
{
  size_t len = 0;
  unsigned int page_num;
  unsigned int page_addr;
  unsigned char page_data[2048];
  unsigned int *bbt_info = NULL;
  unsigned int total_bad_blks = 0;
  unsigned short i = 0;
  unsigned int magic_word_off = 0;
  unsigned int num_blks_str_in_memory = 0;
  unsigned int num_badblks_per_page = 0;
  unsigned short first_badblk_pos = 0;

  /* Read Page 1 of Block0 to get Factory BBT inforamtion */
  page_num = 1;

  num_blks_str_in_memory = 0;
  while(1) {

    page_addr = page_num * nandbootparams->page_size_in_bytes;
    len = nandbootparams->page_size_in_bytes;
    if (nand_read(&nand_info[0], page_addr, &len, page_data))
      return -1;

    /* Check for number of bad block in this page.
    * if it is > 0, than serach for "BAD" string.
    * if "BAD" is found, copy Bad block table addresses to RAM
    * Based table.
    */

    bbt_info = (unsigned int*)&page_data[0];
    if(page_num == 1) {

      /* As first 4 bytes are of number of Bad blocks */
      first_badblk_pos = 1;
      total_bad_blks = bbt_info[0];
      /* 509 Bad blocks for Page 1 */
      num_badblks_per_page = nandbootparams->page_size_in_32bit_words - 3;
    }
    else {
      first_badblk_pos = 0;
      /* 510 Bad blocks for Page (2-15) */
      num_badblks_per_page = nandbootparams->page_size_in_32bit_words - 2;
    }

    if(total_bad_blks > 0 && (total_bad_blks != 0xFFFFFFFF)) {

      if((total_bad_blks - num_blks_str_in_memory) > num_badblks_per_page) {
        magic_word_off = nandbootparams->page_size_in_32bit_words - 2;
      }
      else {
        magic_word_off = (total_bad_blks - num_blks_str_in_memory) + 1;
      }

      if((page_data[sizeof(unsigned int) * magic_word_off] == 'B') &&
        (page_data[(sizeof(unsigned int) * magic_word_off) + 1] == 'A') &&
        (page_data[(sizeof(unsigned int) * magic_word_off) + 2] == 'D')) {

          printf("Page %d found with Factory BBT information\n",page_num);
          for(i = first_badblk_pos; i < magic_word_off; i++) {
            mem_based_bbt[num_blks_str_in_memory] = bbt_info[i];
            num_blks_str_in_memory++;
          }
          if(num_blks_str_in_memory == total_bad_blks)
            break;
          page_num++;
      }
      else {
        printf("No Factory BBT on Page %d\n",page_num);
        printf("Please run nand_params first\n");
        break;
      }
    }
    else {
      printf("No Factory marked bad blocks on this device.\n");
      break;
    }
  }

  /* End of BBT Marker in Memory */
  mem_based_bbt[num_blks_str_in_memory] = 0xFFFFFFFF;

  return num_blks_str_in_memory;
}

int do_lpcnand_format (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  const char *cmd;
  int ret = 0;

  /* need at least two arguments */
  if (argc < 2)
    goto usage;

  cmd = argv[1];

  if (strcmp(cmd, "easy") == 0) {

    printf("It will restore whole nand device back to ");
    printf("the state indicated by block0\n");
    printf("Do you really want to format <y/n>\n");
    while(1) {
      ret = getc();
      if(ret != 0)
        break;
    }

    if(ret != 'y')
      return 0;

    memset(mem_based_bbt,0x0,sizeof(mem_based_bbt));
    ret = get_bbt_info();
    if(ret == -1) {
      printf("not able to read Page 1 of Block0\n");
      return 1;
    }
    if(ret > 0)
      erase_blocks_skipping_blk0_bbt_blks();
  }

  else if(strcmp(cmd, "hard") == 0) {
    printf("It will erase whole nand device.");
    printf("Do you really want to format <y/n>\n");
    while(1) {
      ret = getc();
      if(ret != 0)
        break;
    }

    if(ret != 'y')
      return 0;
    if(erase_all_blocks())
      printf("nand hard format faile\n");
  }
  else
    cmd_usage(cmdtp);

  return 0;

usage:
  cmd_usage(cmdtp);
  return 1;
}

U_BOOT_CMD(
           nand_format, 3, 1, do_lpcnand_format,
           "lpc313x nand format",
           "easy  - Erase all the blocks skipping Factory\n" 
           "              bad blocks Table which is stored in Block 0\n"
           "nand_format hard - Erase all the blocks and write flash params,\n"
           "              Bad block table in Block0\n"
           ""
           );

int do_lpcnand_badbbt_dump (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  int i = 0;
  int ret = 0;

  ret = get_bbt_info();
  if(ret == -1) {
    printf("not able to read Page 1 of Block0\n");
    return 1;
  }

  if(ret > 0) {
    printf("Block 0 based bad blocks:\n");
    while(mem_based_bbt[i] != 0xFFFFFFFF) {
      printf("%x\n",mem_based_bbt[i]);
      i++;
    }
  }
  return 0;
}

U_BOOT_CMD(nand_bad_block0_dump, 1, 1, do_lpcnand_badbbt_dump,
           "Display Block 0 based Factory Bad blocks",
           ""
           );
#endif
