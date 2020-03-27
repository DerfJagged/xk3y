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

#ifndef DOS_PARTITION_H_
#define DOS_PARTITION_H_

#define MAX_PARTITION		10

#define BOOT_IT_PARTITION	0xDF

typedef struct
{
	unsigned int start_lba;
	unsigned int size_lba;
	unsigned char type;
	unsigned char _pad[3];
}
partition_t;

extern int partition_table_probe (void);
extern int partition_get_info (partition_t *pinfo, unsigned int partition_number);

#endif /* DOS_PARTITION_H */


