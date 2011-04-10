/*
 * fs/partitions/tegrapart.c
 * Copyright (c) 2011 Gilles Grandou
 *
 * msdos mbr code taken from msdos.c
 *  Code extracted from drivers/block/genhd.c
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 * 
 *  Build partitions for mmc0 using:
 *
 *  - tegrapart= arguments from cmdline
 *    This allows to create partitions for boot partitions
 *    Unfortunately tegrapart does not list all available partitions
 *    so we cannot solely rely on it.
 *
 *  - MBR partition for others
 *  The code used to read MBR is taken from msdos.c, cleaned of
 *  specific useless stuff (EFI, BSD, ...), and modified to take
 *  the initial MBR offset into account.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/msdos_fs.h>

#include "check.h"
#include "msdos.h"

#include <asm/unaligned.h>


char *partition_list = NULL;


unsigned 
strtohex(char *ptr)
{
	unsigned x = 0;
	unsigned c;

	for(;;) {
		c = *ptr++ | 0x20;
		if ((c >= '0') && (c <= '9'))
			x = (x<<4) | (c & 0xf);
		else if ((c >= 'a') && (c <= 'f'))
			x = (x<<4) | (c - 87);
		else
			return x;
	}
}


#define SYS_IND(p)	(get_unaligned(&p->sys_ind))

#define NR_SECTS(p)	({ __le32 __a =	get_unaligned(&p->nr_sects);	\
				le32_to_cpu(__a); \
			})

#define START_SECT(p)	({ __le32 __a =	get_unaligned(&p->start_sect);	\
				le32_to_cpu(__a); \
			})

static inline int is_extended_partition(struct partition *p)
{
	return (SYS_IND(p) == DOS_EXTENDED_PARTITION ||
		SYS_IND(p) == WIN98_EXTENDED_PARTITION ||
		SYS_IND(p) == LINUX_EXTENDED_PARTITION);
}

#define MSDOS_LABEL_MAGIC1	0x55
#define MSDOS_LABEL_MAGIC2	0xAA

static inline int
msdos_magic_present(unsigned char *p)
{
	return (p[0] == MSDOS_LABEL_MAGIC1 && p[1] == MSDOS_LABEL_MAGIC2);
}


static void
tegra_msdos_parse_extended(struct parsed_partitions *state, struct block_device *bdev,
		               u32 mbr_offset, u32 first_sector, u32 first_size)
{
	struct partition *p;
	Sector sect;
	unsigned char *data;
	u32 this_sector, this_size;
	int sector_size = bdev_logical_block_size(bdev) / 512;
	int loopct = 0;		/* number of links followed
				   without finding a data partition */
	int i;

	this_sector = first_sector;
	this_size = first_size;

	while (1) {
		if (++loopct > 2) {
			printk(KERN_INFO "tegra_msdos_parse_extended: loopcnt>2. exit\n");
			return;
		}

		printk(KERN_INFO "tegra_msdos_parse_extended: read part sector, start=%d+%d size=%d\n",
				mbr_offset, this_sector, this_size);

		data = read_dev_sector(bdev, mbr_offset+this_sector, &sect);
		if (!data) {
			printk(KERN_INFO "tegra_msdos_parse_extended: read error. exit\n");
			return; 
		}

		if (!msdos_magic_present(data + 510)) {
			printk(KERN_INFO "tegra_msdos_parse_extended: no msdos magic. exit\n");
			goto done;
		}

		p = (struct partition *) (data + 0x1be);

		/* 
		 * First process the data partition(s)
		 */
		for (i=0; i<4; i++, p++) {
			u32 offs, size, next;

			if (!NR_SECTS(p) || is_extended_partition(p))
				continue;

			offs = START_SECT(p)*sector_size;
			size = NR_SECTS(p)*sector_size;
			next = this_sector + offs;
			if (i >= 2) {
				if (offs + size > this_size)
					continue;
				if (next < first_sector)
					continue;
				if (next + size > first_sector + first_size)
					continue;
			}

			printk(KERN_INFO "tegra_msdos_parse_extended: put_partition %d start=%d+%d size=%d\n",
					state->next, mbr_offset, next, size);
			put_partition(state, state->next++, mbr_offset+next, size);
			loopct = 0;
		}
	
		printk(KERN_INFO "tegra_msdos_parse_extended: done with this sector\n");

		p -= 4;
		for (i=0; i<4; i++, p++)
			if (NR_SECTS(p) && is_extended_partition(p)) {
				printk(KERN_INFO "tegra_msdos_parse_extended: extended part slot %d\n", i+1);
				break;
			}

		if (i == 4)
			goto done;	 /* nothing left to do */

		this_sector = first_sector + START_SECT(p) * sector_size;
		this_size = NR_SECTS(p) * sector_size;
		put_dev_sector(sect);
	}
done:
	printk(KERN_INFO "tegra_msdos_parse_extended: done\n");
	put_dev_sector(sect);
}



int tegra_msdos_parse(struct parsed_partitions *state, struct block_device *bdev, u32 mbr_offset)
{
	int sector_size = bdev_logical_block_size(bdev) / 512;
	Sector sect;
	unsigned char *data;
	struct partition *p;
	int slot;

	printk(KERN_INFO "tegra_msdos_parse: mbr_offset=%d\n", mbr_offset);

	data = read_dev_sector(bdev, mbr_offset, &sect);
	if (!data) {
		printk(KERN_INFO "tegra_msdos_parse: read error. exit\n");
		return -1;
	}
	if (!msdos_magic_present(data + 510)) {
		printk(KERN_INFO "tegra_msdos_parse: no msdos magic\n");
		put_dev_sector(sect);
		return 0;
	}

	p = (struct partition *) (data + 0x1be);
	for (slot = 1; slot <= 4; slot++, p++) {
		if (p->boot_ind != 0 && p->boot_ind != 0x80) {  // 0x1be,0x1ce,0x1de,0x1fe
			printk("tegra_msdos_parse: slot %d, boot_ind=0x%x. exit\n", slot, p->boot_ind);
			put_dev_sector(sect);
			return 0;
		}
	}

	p = (struct partition *) (data + 0x1be);

	for (slot = 1 ; slot <= 4 ; slot++, p++) {
		u32 start = START_SECT(p)*sector_size; // 0015f800	1439744
		u32 size = NR_SECTS(p)*sector_size;    // 01c41400	29627392
		printk(KERN_INFO "tegra_msdos_parse: slot %d, start=%d size=%d\n", slot, start, size);
		if (!size)
			continue;
		if (is_extended_partition(p)) {
			printk(KERN_INFO "tegra_msdos_parse: slot %d extended partition\n", slot);
			//put_partition(state, state->next++, start+mbr_offset, size == 1 ? 1 : 2);
			printk(" <");
			tegra_msdos_parse_extended(state, state->bdev, mbr_offset, start, size); 
			printk(" >");
			continue;
		}
		printk(KERN_INFO "tegra_msdos_parse: put_partition\n");
		put_partition(state, state->next++, start+mbr_offset, size);
	}

	printk("\n");
	printk(KERN_INFO "tegra_msdos_parse: done\n");

	put_dev_sector(sect);
	return 1;
}



#define STATE_NAME	0
#define STATE_OFFSET	1
#define STATE_SIZE	2
#define STATE_BLOCKSIZE	3

int
parse_tegrapart(struct parsed_partitions *state)
{
	char *ptr;
	char *pstart;
	int pstate;
	char name[8];
	u32 offset, size, blocksize, kblocksize;
	int done;
	int ret=0;

	printk(KERN_INFO "parse_tegrapart: tegrapart=%s\n", partition_list);

	kblocksize = bdev_logical_block_size(state->bdev);

	ptr = partition_list;
	pstart = ptr;
	pstate = STATE_NAME;
	name[0] = '\0';
	offset = 0;
	size = 0;
	blocksize = 0;
	done = 0;
	do {
		switch(pstate) {
		case STATE_NAME:
			if (*ptr==':') {
				int len=ptr-pstart;
				if (len>7)
					len=7;
				memcpy(name, pstart, len);
				name[len] = '\0';
				pstate++;
				pstart = ptr+1;
			}
			break;
		case STATE_OFFSET:
			if (*ptr==':') {
				offset=strtohex(pstart);
				pstate++;
				pstart = ptr+1;
			}
			break;
		case STATE_SIZE:
			if (*ptr==':') {
				size=strtohex(pstart);
				pstate++;
				pstart = ptr+1;
			}
			break;
		case STATE_BLOCKSIZE:
			if (*ptr=='\0')
				done = 1;
			if ((*ptr==',') || (*ptr=='\0')) {
				blocksize=strtohex(pstart);
				pstate = STATE_NAME;
				pstart = ptr+1;

				offset = offset*blocksize/kblocksize;
				size   = size*blocksize/kblocksize;


				if (!strcasecmp(name, "mbr")) {
					printk(KERN_INFO "parse_tegrapart: mbr start=%d\n", offset);
					return tegra_msdos_parse(state, state->bdev, offset);
				}

				printk(KERN_INFO "parse_tegrapart: part #%d [%s] start=%d size=%d\n",
						state->next, name, offset, size);

				put_partition(state, state->next++, offset, size);
				ret = 1;

			}
			break;
		}
		ptr++;
	}
	while (!done);

	printk(KERN_INFO "parse_tegrapart: done without mbr\n");
	return ret;
}



int 
tegrapart_partition(struct parsed_partitions *state)
{
	printk(KERN_INFO "tegrapart_partition: state->bdev->bd_disk->disk_name = %s\n", state->bdev->bd_disk->disk_name);

	if (strcmp(state->bdev->bd_disk->disk_name, "mmcblk1")) {
		printk(KERN_INFO "tegrapart_partition: exit\n");
		return 0;
	}

	if (state->parts[0].size) {
		printk(KERN_INFO "tegrapart_partition: part[0].size = 0. exit\n");
		return 0;
	}

	state->next = 1;

	return parse_tegrapart(state);
}



static int 
__init partition_setup(char *options)
{
	if (options && *options && !partition_list)
		partition_list = options;

	return 0;
}


__setup("tegrapart=", partition_setup);

