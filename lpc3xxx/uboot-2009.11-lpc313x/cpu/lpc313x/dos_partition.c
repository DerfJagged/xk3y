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
#include "dos_partition.h"

#ifdef CONFIG_MMC
static partition_t partition_table_data[MAX_PARTITION + 1];

/*
 * Read a 32bit little endian value from an address which may be misaligned.
 * Helper function for reading partition table entries...
 */
static unsigned int unaligned_read32_le (unsigned char *src)
{
	unsigned int value = 0;
	unsigned int bytes_remaining = 4;

	do {
		value |= *src++;
		value = (value << 24) | (value >> 8);
	}
	while (--bytes_remaining);

	return value;
}


/*
 * returns 1 if partition type indicates an extended partition
 */
static int p_type_is_extended (int type)
{
	return ((type == 0x05) || (type == 0x0F) || (type == 0x85)) ? 1 : 0;
}

static int p_table_sector_scan (partition_t *parent, partition_t *primary_parent,
		unsigned char *buf)
{
	int i;
	partition_t *pinfo;
	unsigned char *pinfo_raw;


	if(mci_read_blocks((int)mmc_get_dev(0), parent->start_lba, 1, buf) == 0) {
		printf("not able to read first sector for patition table\n");
		return -1;
	}

	/* check for valid partition table signature bytes */
	if (*((unsigned short *) &buf[510]) != 0xAA55)
		return -1;

	pinfo = (partition_t *) buf;
	
	/* first offset (others will be 462, 478 and 494) */
	pinfo_raw = &buf[446];

	/* scan all partition entries (only first 2 are used for non-MBR) */
	for (i = 0; i < 4; i++) {
		memset (pinfo, 0, sizeof(partition_t));
		if ((pinfo->size_lba = unaligned_read32_le (&pinfo_raw[12])) != 0) {
			pinfo->start_lba = unaligned_read32_le (&pinfo_raw[8]);
			pinfo->type = pinfo_raw[4];
			pinfo->start_lba += p_type_is_extended (pinfo->type) ?
				primary_parent->start_lba :
				parent->start_lba;
		}

		pinfo++;
		pinfo_raw += 16;
	}

	return 0;
}

int partition_table_probe (void)
{
	int i;
	int logical_index;
	partition_t temp, *parent, *primary_parent;

	unsigned char buf[512];

	memset (partition_table_data, 0, sizeof(partition_table_data));

	if (p_table_sector_scan (partition_table_data, partition_table_data, buf) < 0) {
		printf ("Fatal Error: No valid partition table found... \n");
		return -1;
	}

	memcpy (&partition_table_data[1], buf, sizeof(partition_t) * 4);

	return 0;
}

int partition_get_info (partition_t *pinfo, unsigned int partition_number)
{
	int i = MAX_PARTITION;

	/* is the partition number being requested valid ?? */
	if ((partition_number < 1) || (partition_number > MAX_PARTITION)) {
		/*
		 * check if we discovered bootIt partition. if yes return it for all
		 * invalid partion numbers
		 */
		while ( i > 0 ) {
			if (partition_table_data[i].type == BOOT_IT_PARTITION) {
				/* If yes, copy partition information about
				 *  the requested partition into supplied buffer */
				memcpy (pinfo, &partition_table_data[i], sizeof(partition_t));
				return 0;
			}
			i--;
		}
		return -1;
	}

	/* was the partition number being requested found during probing ?? */
	if (partition_table_data[partition_number].size_lba == 0)
		return -1;

	/* is the partition an extended partition ?? */
	if (p_type_is_extended (partition_table_data[partition_number].type))
		return -1;

	/* If yes, copy partition information about the requested
	 * partition into supplied buffer */
	memcpy (pinfo, &partition_table_data[partition_number], sizeof(partition_t));

	return 0;
}
#endif
