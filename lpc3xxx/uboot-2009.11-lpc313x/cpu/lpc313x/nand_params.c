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
#include <malloc.h>

#if defined(CONFIG_CMD_NAND)
#include <asm/arch/sysreg.h>
#include <asm/arch/nand.h>

#define LPC313x_NAND_BPARAM_PAGE_SZ	256

const NAND_BOOT_CFG_PARAMS_T nandbootparams[] =
{
	{
		.tag = "NANDflsh",
		.interface_width = 8,
		.page_size_in_bytes = 2048,
		.page_size_in_32bit_words = 512,
		.pages_per_block = 64,
		.nbr_of_blocks = 2048,
		.amount_of_address_bytes = 5,
		.amount_of_erase_address_bytes = 3,
		.support_read_terminate = 1,
		.page_increment_byte_nr = 2,
		/* first 4bytes to fill with readid response */
		.device_name = { 0x2C, 0xAA, 0xFF, 0x15, 0x20, 'M', 'T', '2',
				'9', 'F', '2', 'G', '0', '8',},
		/* assuming 90MHz clock (1clock = 11ns)to NAND block */
		/* Note, timing macros tcopy naake clk +1 values. */
		/* tsrd=3, tals=3, talh=1, tcls=3, tclh=1, */
		.timing1 = NAND_TIM1_TSRD(3) | NAND_TIM1_TALS(3) |
			NAND_TIM1_TALH(1) | NAND_TIM1_TCLS(3) | NAND_TIM1_TCLH(1),
		/* tdrd=3, tebi=1, tch=1, tcs=4, treh=2, trp=4, twh=2, twp = 3*/
		.timing2 = NAND_TIM2_TDRD(3) | NAND_TIM2_TEBI(1) |
			NAND_TIM2_TCH(1) | NAND_TIM2_TCS(4) | NAND_TIM2_TRH(2) |
			NAND_TIM2_TRP(4) | NAND_TIM2_TWH(2) | NAND_TIM2_TWP(3),
		.ecc_mode = 5,
		.id_mask = 0x6, /*except 2nd & 3rd byte check remains id bytes */
	},
};

static int write_bbt_in_blk0(struct nand_chip *chip,unsigned int *bad_list_buf)
{
	nand_info_t *nand = NULL;
	unsigned int bad_blk_count = 0;
	char* buff_ptr = NULL;
	unsigned int temp_buff[2];
	int curr_pos = 0;
	int magic_word_pos;
	int page_nr;
	size_t page_addr;
	size_t len;
	unsigned int off = 0;

	curr_pos = 0;
	page_nr = 1;
	bad_blk_count = *bad_list_buf;
	while (curr_pos < (bad_blk_count + 1)) {

		if ( (bad_blk_count + 1 - curr_pos) < (nandbootparams->page_size_in_32bit_words - 2)) {
			magic_word_pos = bad_blk_count + 1;
		}
		else {
			magic_word_pos = curr_pos + nandbootparams->page_size_in_32bit_words - 2;
		}
		// printf("curr_pos:%d, bad_blk_count:%d, magic_pos:%d\n", curr_pos, bad_blk_count, magic_word_pos);

		/* 
		 * save last 2 words in current page to write with magic id & CRC.
		 * Not needed if CBad < (chip->page_size_in_32bit_words - 2)
		 */
		temp_buff[0] = bad_list_buf[magic_word_pos];
		temp_buff[1] = bad_list_buf[magic_word_pos + 1];

		/* insert magic word */
		buff_ptr = (char*)&bad_list_buf[magic_word_pos];
		buff_ptr[0] = 'B';
		buff_ptr[1] = 'A';
		buff_ptr[2] = 'D';
		buff_ptr[3] = (char)(page_nr & 0xFF);

		/* compute CRC32 */
		bad_list_buf[magic_word_pos + 1] = crc32_compute((u8*)&bad_list_buf[curr_pos],
				((u32)&buff_ptr[4] - (u32)&bad_list_buf[curr_pos]));

		page_addr = page_nr * nandbootparams->page_size_in_bytes;
		len = nandbootparams->page_size_in_bytes;
		if (nand_write(&nand_info[0], page_addr, &len,
					(void*)(bad_list_buf + curr_pos))) {
			printf("nand write failed\n");
			return 1;
		}
		/*put back last 2 words*/
		bad_list_buf[magic_word_pos] = temp_buff[0];
		bad_list_buf[magic_word_pos + 1] = temp_buff[1];
		/* move to next page */
		page_nr++;
		/* update current position in bad block list */
		curr_pos = magic_word_pos;
	}
	return 0;
}

static unsigned int* prepare_bbt_for_blk0(struct nand_chip *chip)
{
	nand_info_t *nand = NULL;
	unsigned int bad_blk_count = 0;
	unsigned int* bad_list_buf = (unsigned int*)EXT_SDRAM_BASE;
	unsigned int off = 0;

	/* Prepare BBT in Memory and Write it to NAND for ROM Bootloader */
	nand = &nand_info[nand_curr_device];
	for (off = 0; off < nand->size; off += nand->erasesize) {
		if (nand_block_isbad(nand, off)) {
			bad_blk_count++;
			*((u32*)(bad_list_buf + bad_blk_count)) = off;
		}
	}

	printf("total bad blocks: %d\n",bad_blk_count);

	/* update bad block count */
	*bad_list_buf = bad_blk_count;

	return bad_list_buf;
}

static int checkflashparams (struct nand_chip *chip, size_t offset, unsigned char *writeparam)
{
	size_t end = offset + sizeof(NAND_BOOT_CFG_PARAMS_T);
	size_t amount_loaded = 0;
	size_t blocksize, len;
	u_char *char_ptr = NULL;
	NAND_BOOT_CFG_PARAMS_T *tmpparams = NULL;

	char_ptr = (u_char*)malloc(chip->subpagesize);
	memset(char_ptr,0x0,chip->subpagesize);

	blocksize = nand_info[0].erasesize;
	len = min(blocksize, sizeof(NAND_BOOT_CFG_PARAMS_T));

	/* Aligned len to subpage boundary */
	len = (((len) + (chip->subpagesize - 1)) & ~ (chip->subpagesize - 1));

	while (amount_loaded < sizeof(NAND_BOOT_CFG_PARAMS_T) && offset < end) {
		if (nand_block_isbad(&nand_info[0], offset)) {
			offset += blocksize;
		} else {
			if (nand_read(&nand_info[0], offset, &len, char_ptr))
				return 1;
			offset += blocksize;
			amount_loaded += len;
		}
	}
	if (amount_loaded != len)
		return 1;

	tmpparams = (NAND_BOOT_CFG_PARAMS_T *)char_ptr;

	if((strncmp(tmpparams->tag,"NANDflsh", 8) == 0) &&
			(tmpparams->interface_width == nandbootparams->interface_width ) &&
			(tmpparams->page_size_in_bytes == nandbootparams->page_size_in_bytes) &&
			(tmpparams->page_size_in_32bit_words ==
			 nandbootparams->page_size_in_32bit_words)) {
		printf("flash params are already written into flash\n");
		*writeparam = 0;
		return 0;
	}
	else {
		printf("flash params are not written into flash\n");
		*writeparam = 1;
		return 0;
	}

}

static int writeparams(struct nand_chip *chip, size_t offset, u_char *buf)
{
	size_t end = offset + sizeof(NAND_BOOT_CFG_PARAMS_T);
	size_t amount_saved = 0;
	size_t blocksize, len;
	u_char *char_ptr = NULL;

	blocksize = nand_info[0].erasesize;
	len = min(blocksize, sizeof(NAND_BOOT_CFG_PARAMS_T));

	/* Aligned len to subpage boundary */
	len = (((len) + (chip->subpagesize - 1)) & ~ (chip->subpagesize - 1));

	char_ptr = (u_char*)malloc(chip->subpagesize);

	memset(char_ptr,0x0,chip->subpagesize);
	memcpy(char_ptr,buf,sizeof(NAND_BOOT_CFG_PARAMS_T));

	/* Compute CRC32. */
	*((u32*)(char_ptr + (LPC313x_NAND_BPARAM_PAGE_SZ - 4))) =
		crc32_compute(char_ptr, LPC313x_NAND_BPARAM_PAGE_SZ - 4);

	while (amount_saved < sizeof(NAND_BOOT_CFG_PARAMS_T) && offset < end) {
		if (nand_block_isbad(&nand_info[0], offset)) {
			offset += blocksize;
		} else {
			if (nand_write(&nand_info[0], offset, &len,
						&char_ptr[amount_saved]))
				return 1;
			offset += blocksize;
			amount_saved += len;
		}
	}
	if (amount_saved != len) {
		printf("amount saved: %d, while actual: %d\n",amount_saved,len);
		return 1;
	}
	return 0;
}

int write_params_bbt(unsigned int* bad_list_buf)
{
	nand_erase_options_t nand_erase_options;
	struct nand_chip *chip = (struct nand_chip*) nand_info[nand_curr_device].priv;

	nand_erase_options.length =
		nandbootparams->page_size_in_bytes * nandbootparams->pages_per_block;
	nand_erase_options.quiet = 1;
	nand_erase_options.jffs2 = 0;
	nand_erase_options.scrub = 0;
	nand_erase_options.offset = 0;

	printf ("Erasing Nand...\n");
	if (nand_erase_opts(&nand_info[0], &nand_erase_options))
		return 1;
	printf ("Writing Flash Params to Nand... ");
	if (writeparams(chip,0x0, (u_char *) nandbootparams)) {
		puts("FAILED!\n");
		return 1;
	}
	puts ("done\n");
	
	if(bad_list_buf == NULL)
		bad_list_buf = prepare_bbt_for_blk0(chip);
	write_bbt_in_blk0(chip,bad_list_buf);
	return 0;
}

int prepare_write_nand_params_bbt(unsigned char isbooting)
{
	int ret = 0;
	unsigned char writeparam = 0;
	struct nand_chip *chip = (struct nand_chip*) nand_info[nand_curr_device].priv;
	unsigned int* bad_list_buf = NULL;

	checkflashparams(chip,0x0,&writeparam);

	if(writeparam) {
		if(write_params_bbt(bad_list_buf))
			return 1;
	}
	else { 
		if(!isbooting) { 
			printf("It will overwrite the current BBT of Block0 ");
			printf("with MTD BBT and\n");
			printf("make Block 0 BBT in sync with MTD BBT\n");
			printf("Do you really want to overwrite Block0 BBT: <y/n>\n");

			while(1) {
				ret = getc();
				if(ret != 0)
					break;
			}

			if(ret != 'y')
				return 0;

			if(write_params_bbt(bad_list_buf))
				return 1;
		}
	} 

	return 0;
}

int do_lpcnand_params (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	prepare_write_nand_params_bbt(0);
	return 0;
}

U_BOOT_CMD(nand_params, 1, 1, do_lpcnand_params,
	"write NAND Flash params to be used by LPC BootROM code",
	""
);
#endif
