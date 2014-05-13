/*
 *  linux/kernel/hd.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 * 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 */
#include <haribote/hdreg.h>
//#include <asm/system.h>
#include <asm/io.h>
#include "bootpack.h"
#include "fs.h"
#include "hd_linux.h"
#include <stdio.h>
#include <string.h>
//#include <asm/segment.h>

#define MAJOR_NR 3
#include "blk.h"

#define nop() __asm__ ("nop"::)

#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7
#define MAX_HD		2

//static void recal_intr(void);

//static int recalibrate = 1;
static int reset = 1;

/*
 *  This struct defines the HD's and their types.
 */
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static struct orange_hd_info	orange_hd_info[1];
static int NR_HD = 0;
#endif

static struct hd_struct {
	long start_sect;
	long nr_sects;
} hd[5*MAX_HD]={{0,0},};
static char hdbuf[SECTOR_SIZE * 2];
//CHANGED TESTED
#define port_read(port,buf,nr) \
__asm__("push %%ecx; push %%edi; push %%edx; cld;rep;insw; pop %%edx; pop %%edi; pop %%ecx;"::"d" (port),"D" (buf),"c" (nr))

//CHANGED
#define port_write(port,buf,nr) \
__asm__("push %%ecx; push %%edi; push %%edx; cld;rep;outsw; pop %%edx; pop %%edi; pop %%ecx;"::"d" (port),"S" (buf),"c" (nr))

extern void hd_interrupt(void);
extern void rd_load(void);
extern void hd_cmd_out(struct hd_cmd* cmd,void (*intr_addr)(void));
extern int waitfor(int mask, int val, int timeout);
extern int sys_setup(void * BIOS);

/* This may be used only once, enforced by 'static int callable' */
int sys_setup(void * BIOS)
{
	//debug("here1");
	//static int callable = 1;
	int i,drive;
	//unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;
	hd_identify(0);
	NR_HD = 1;
	for (i = 0 ; i < NR_HD ; i++) {
		hd[i*5].start_sect = orange_hd_info[0].primary[0].base;
		hd[i*5].nr_sects = orange_hd_info[0].primary[0].size;
	}
	for (drive=0 ; drive<NR_HD ; drive++) {
		//debug("here3");
		if (!(bh = bread(0x300 + drive*5,0))) {
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		//debug("here4");
		//����Ҫ����������
		//if (bh->b_data[510] != 0x55 || (unsigned char) 
		//    bh->b_data[511] != 0xAA) {
		//	printk("Bad partition table on drive %d\n\r",drive);
		//	panic("");
		//}
		
		//��ȡ������
		p = 0x1BE + (void *)bh->b_data;
		for (i=1;i<5;i++,p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
			//debug("start_sect = 0x%x, nr_sects = 0x%x", p->start_sect, p->nr_sects);
		}
		brelse(bh);
	}
	if (NR_HD)
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");
	//rd_load();
	mount_root(); //���ظ��ļ�ϵͳ
	return (0);
}

//static int controller_ready(void)
//{
//	int retries=10000;

//	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
//	return (retries);
//}

static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

//static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
//		unsigned int head,unsigned int cyl,unsigned int cmd,
//		void (*intr_addr)(void))
//{
//	register int port asm("dx");

//	if (drive>1 || head>15)
//		panic("Trying to write bad sector");
//	if (!controller_ready())
//		panic("HD controller not ready");
//	do_hd = intr_addr;
//	outb_p(hd_info[drive].ctl,HD_CMD);
//	port=HD_DATA;
//	outb_p(hd_info[drive].wpcom>>2,++port);
//	outb_p(nsect,++port);
//	outb_p(sect,++port);
//	outb_p(cyl,++port);
//	outb_p(cyl>>8,++port);
//	outb_p(0xA0|(drive<<4)|head,++port);
//	outb(cmd,++port);
//}

//static int drive_busy(void)
//{
//	unsigned int i;

//	for (i = 0; i < 10000; i++)
//		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
//			break;
//	i = inb(HD_STATUS);
//	i &= BUSY_STAT | READY_STAT | SEEK_STAT;
//	if (i == (READY_STAT | SEEK_STAT))
//		return(0);
//	printk("HD controller times out\n\r");
//	return(1);
//}

//static void reset_controller(void)
//{
//	int	i;

//	outb(4,HD_CMD);
//	for(i = 0; i < 100; i++) nop();
//	outb(hd_info[0].ctl & 0x0f ,HD_CMD);
//	if (drive_busy())
//		printk("HD-controller still busy\n\r");
//	if ((i = inb(HD_ERROR)) != 1)
//		printk("HD-controller reset failed: %02x\n\r",i);
//}

//static void reset_hd(int nr)
//{
//	reset_controller();
//	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
//		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
//}

void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

static void bad_rw_intr(void)
{
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

static void read_intr(void)
{
	//�ú��������жϴ˴ζ���������Ƿ��������������������������æ״̬����������ִ�д���
	//����Ӳ�̲���ʧ�����⣬�����ٴ�����Ӳ�̸�λ����ִ�����������Ȼ����ʡ�ÿ�ζ���������
	//����Ե�ǰ����������������ۼƣ�����������������������������һ�룬���...
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();
		return;
	}
	//debug("invoke read_intr");
	port_read(HD_DATA,CURRENT->buffer,256);
	//port_read(HD_DATA,hdbuf,256);
	//char msg[200];
	//memset(msg,0,200);
	//string_memory((u8 *)CURRENT->buffer+ PARTITION_TABLE_OFFSET,20,msg);
	//debug(msg);
	CURRENT->errors = 0;
	CURRENT->buffer += 512;
	CURRENT->sector++;
	if (--CURRENT->nr_sectors) {
		do_hd = &read_intr;
		return;
	}
	end_request(1);
	do_hd_request();
}

static void write_intr(void)
{
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();
		return;
	}
	if (--CURRENT->nr_sectors) {
		CURRENT->sector++;
		CURRENT->buffer += 512;
		do_hd = &write_intr;
		port_write(HD_DATA,CURRENT->buffer,256);
		return;
	}
	end_request(1);
	do_hd_request();
}

//static void recal_intr(void)
//{
//	if (win_result())
//		bad_rw_intr();
//	do_hd_request();
//}


//ִ��Ӳ�̶�д�������

void do_hd_request(void)
{
	//debug("invoke do_hd_request");
	int i,r;
	unsigned int block,dev;
	//unsigned int sec,head,cyl;
	unsigned int nsect;

	//���������ȼ��������ĺϷ��ԡ��������������û�����������˳����ο�blk.h)
	//Ȼ��ȡ�豸���е����豸���Լ��豸��ǰ�������е���ʼ������.���豸�ż���ӦӲ��
	//�ϸ�������������豸�Ų����ڻ�����ʼ�������ڸ÷���������-2������������󣬲�����
	//�����repeat����������INIT_REQUEST��ʼ��������Ϊһ��Ҫ���дһ�����ݣ�2��������
	//��1024�ֽڣ�����������������Ų��ܴ��ڷ�����������ڶ���������Ȼ��ͨ���������豸��
	//��Ӧ��������ʼ�����ţ��Ͱ���Ҫ��д�Ŀ��Ӧ������Ӳ�̵ľ���������block�ϡ������豸��
	//��5�������ɵõ���Ӧ��Ӳ�̺š�
	INIT_REQUEST;
	dev = MINOR(CURRENT->dev);  //���豸�ţ���ӦӲ���ϸ�������
	block = CURRENT->sector;  //��ʼ������
	if ( (dev >= 5*NR_HD) || ( (block+2) > hd[dev].nr_sects)) {
		debug("dev = %d, NR_HD = %d, block = %d, hd[%d].nr_sects = %d",dev, NR_HD, block, dev, hd[dev].nr_sects);
		end_request(0);
		goto repeat;
	}
	block += hd[dev].start_sect; //���̾���������
	dev /= 5;
	//debug("drive = %d",dev);
	//__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
	//	"r" (hd_info[dev].sect));
	//__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
	//	"r" (hd_info[dev].head));
	//sec++;
	//CURRENT->nr_sectors = 1;
	nsect = CURRENT->nr_sectors;
	//if (reset) {
	//	reset = 0;
	//	recalibrate = 1;
	//	reset_hd(CURRENT_DEV);
	//	return;
	//}
	//if (recalibrate) {
	//	recalibrate = 0;
	//	hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
	//		WIN_RESTORE,&recal_intr);
	//	return;
	//}	
	//debug("block = %d, sec_count = %d", block, nsect);
	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= nsect;
	cmd.lba_low	= block & 0xFF;
	cmd.lba_mid	= (block >>  8) & 0xFF;
	cmd.lba_high	= (block >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, dev, (block >> 24) & 0xF);

	
	if (CURRENT->cmd == WRITE) {
		//hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		cmd.command	= ATA_WRITE;
		hd_cmd_out(&cmd,&write_intr);
		//����������DRQ��λ���˳�ѭ�������ȵ�ѭ������Ҳû����λ�����ʾ���͵�Ҫ��дӲ��
		//����ʧ�ܣ�������תȥ������ֵ���������ִ����һ��Ӳ�����󡣷������ǾͿ�����Ӳ��
		//���������ݼĴ����˿�HD_DATAд��1�����������ݡ�
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		if (!r) {
			bad_rw_intr();
			goto repeat;
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	} else if (CURRENT->cmd == READ) {
		//hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
		cmd.command	= ATA_READ;
		hd_cmd_out(&cmd,&read_intr);
	} else
		panic("unknown hd-command");
}
int hasInterrupt = 0;
void inthandler2e(int *esp)
{
	io_in8(REG_STATUS);
	io_out8(PIC1_OCW2, 0x66);	/* ֪ͨPIC1 IRQ-14�������Ѿ����  */
	io_out8(PIC0_OCW2, 0x62);	/* ֪ͨPIC0 IRQ-02�������Ѿ����  */
	if(do_hd)
		do_hd();
	else
		debug("WANR: do_hd is NULL");
	hasInterrupt = 1;
	//debug("hd interrupt happen");
	return;
}

void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	//set_intr_gate(0x2E,&hd_interrupt);
	//outb_p(inb_p(0x21)&0xfb,0x21);
	//outb(inb_p(0xA1)&0xbf,0xA1);
}

/*-------------------------------�ϰ�������Ϊ�˵��ԣ�-----------------------------------------*/


#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
dev / NR_PRIM_PER_DRIVE : \
(dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)
void interrupt_wait3(){
	while(!hasInterrupt);
	hasInterrupt = 0;
}

void hd_cmd_out(struct hd_cmd* cmd,void (*intr_addr)(void))
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT)){
		panic("hd error.");
		return;
	}
	//debug("features = %d, count = %d, lba_low = %d, lba_mid = %d, lba_high = %d, device = 0x%x, command = %d",
	//	   cmd->features, cmd->count, cmd->lba_low, cmd->lba_mid,cmd->lba_high,cmd->device,cmd->command);
	do_hd = intr_addr;
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

u16* hd_identify(int drive)
{
	//debug("here");
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);  //device�Ĵ���
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd,NULL);
	interrupt_wait3();
	
	port_read(REG_DATA, hdbuf, SECTOR_SIZE/2);
	
	u16* hdinfo = (u16*)hdbuf;

	orange_hd_info[drive].primary[0].base = 0;
	/* Total Nr of User Addressable Sectors */
	orange_hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
	
	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	debug( "HD size: %dMB\n", sectors * 512 / 1000000);
	//debug("hdinfo = 0x%x",(int)hdinfo);
	return hdinfo;
	//print_identify_info((u16*)hdbuf);
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
	//debug("drive = %d", drive);
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
	hd_cmd_out(&cmd,NULL);
	interrupt_wait3();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE/2);
	memcpy1(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
	char msg[200];
	memset(msg,0,200);
	string_memory((u8 *)hdbuf + PARTITION_TABLE_OFFSET,20,msg);
	debug(msg);
}
  

/*                                partition
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
	//debug("drive = %d, style = %d",drive, style);
	struct orange_hd_info * hdi = &orange_hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY) {
		get_part_table(drive, drive, part_tbl); //��ȡ��������

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;
			//debug("start_sector = %d, size = %d",part_tbl[i].start_sect, part_tbl[0].nr_sects);
			
			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED);
		}
		//assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED) {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect; //��չ�����Ŀ�ʼ��ַ
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */

			get_part_table(drive, s, part_tbl); //��ȡ��չ������

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;
			//debug("start_sector = %d, size = %d",part_tbl[0].start_sect, part_tbl[0].nr_sects);

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
	struct orange_hd_info * hdi = &orange_hd_info[drive];
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



int waitfor(int mask, int val, int timeout)
{
	//int t = get_ticks();

	//while(((get_ticks() - t) * 1000 / HZ) < timeout)
	while(1){   //�򵥴����Ȳ�ʵ��get_ticks()
		if ((io_in8(REG_STATUS) & mask) == val)
			return 1;
	}
	return 0;
}


