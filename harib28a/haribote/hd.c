
#include "bootpack.h"
#include "hd.h"
#include <stdio.h>

void	port_read(u16 port, void* buf, int n);
int waitfor(int mask, int val, int timeout);

struct FIFO32 *hdfifo;
int hasInterrupt = 0;
u8	hdbuf[SECTOR_SIZE * 2];

struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

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
