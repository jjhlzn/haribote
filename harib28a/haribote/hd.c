
#include "bootpack.h"
#include "hd.h"
#include <stdio.h>
#include <string.h>

void	port_read(u16 port, void* buf, int n);
int waitfor(int mask, int val, int timeout);

struct FIFO32 *hdfifo;
int hasInterrupt = 0;
u8	hdbuf[SECTOR_SIZE * 2];

struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
static struct hd_info	hd_info[1];
PRIVATE struct part_ent PARTITION_ENTRY;
#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
							dev / NR_PRIM_PER_DRIVE : \
						   (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

void inthandler2e(int *esp)
{
	unsigned char hd_status = io_in8(REG_STATUS);
	//fifo32_put(hdfifo,3000);
	hasInterrupt = 1;
	return;
}

void init_hd(struct FIFO32 * fifo)
{
	hdfifo = fifo;
	
	/* Get the number of drives from the BIOS data area */
	unsigned char * pNrDrives = (unsigned char *)(0x475);
	//printl("NrDrives:%d.\n", *pNrDrives);
	//assert(*pNrDrives);
	char strbuf[50];
	
	sprintf(strbuf,"NrDrives:%d", *pNrDrives);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 220+16+16, 220+8*50, 220+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 220+16+16, COL8_000000, strbuf);	


	//put_irq_handler(AT_WINI_IRQ, hd_handler);
	//enable_irq(CASCADE_IRQ);
	//enable_irq(AT_WINI_IRQ);
}


u8* hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	
	char strbuf[100];
	sprintf(strbuf,"after interrupt_wait");
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 240+16+16, 240+8*50, 240+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 240+16+16, COL8_000000, strbuf);	
	
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	sprintf(strbuf,"after port_read");
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 260+16+16, 260+8*50, 260+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 260+16+16, COL8_000000, strbuf);
	
	u16* hdinfo = (u16*)hdbuf;

	hd_info[drive].primary[0].base = 0;
	/* Total Nr of User Addressable Sectors */
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
	
	return hdbuf;
	//print_identify_info((u16*)hdbuf);
}

void interrupt_wait(){
	while(hasInterrupt == 1);
	hasInterrupt = 0;
}

void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT)){
		//panic("hd error.");
		char strbuf[100];
		sprintf(strbuf,"hd error");
		boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 360+16+16, 360+8*50, 360+16+16+16);
		putfonts8_asc(binfo->vram, binfo->scrnx, 10, 360+16+16, COL8_000000, strbuf);
		return;
	}

	/* Activate the Interrupt Enable (nIEN) bit */
	io_out8(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	io_out8(REG_FEATURES, cmd->features);
	io_out8(REG_NSECTOR,  cmd->count);
	io_out8(REG_LBA_LOW,  cmd->lba_low);
	io_out8(REG_LBA_MID,  cmd->lba_mid);
	io_out8(REG_LBA_HIGH, cmd->lba_high);
	io_out8(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	io_out8(REG_CMD,     cmd->command);
}

int waitfor(int mask, int val, int timeout)
{
	//int t = get_ticks();

	//while(((get_ticks() - t) * 1000 / HZ) < timeout)
	while(1)   //简单处理，先不实现get_ticks()
		if ((io_in8(REG_STATUS) & mask) == val)
			return 1;

	return 0;
}

/*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 * 
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 * 
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info * hdi = &hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY) {
		get_part_table(drive, drive, part_tbl);

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED);
		}
		//assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED) {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */

			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

			s = ext_start_sect + part_tbl[1].start_sect;

			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
				break;
		}
	}
	else {
		//assert(0);
	}
}

/*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 *****************************************************************************/
void print_hdinfo(char * str)
{
	int drive = 0;
	struct hd_info * hdi = &hd_info[drive];
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) {
		sprintf(str+strlen(str),"%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i == 0 ? " " : "     ",
		       i,
		       hdi->primary[i].base,
		       hdi->primary[i].base,
		       hdi->primary[i].size,
		       hdi->primary[i].size);
	}
	for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		sprintf(str+strlen(str),"         "
		       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i,
		       hdi->logical[i].base,
		       hdi->logical[i].base,
		       hdi->logical[i].size,
		       hdi->logical[i].size);
	}
}
