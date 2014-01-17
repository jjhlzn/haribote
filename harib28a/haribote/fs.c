#include "bootpack.h"
#include "hd.h"
#include "fs.h"
#include <string.h>

void init_fs();
PRIVATE void mkfs();
/**
 * 6MB~7MB: buffer for FS
 */
PUBLIC	u8 *		fsbuf		= (u8*)0xe00000;
PUBLIC	const int	FSBUF_SIZE	= 0x100000;

static struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

/*****************************************************************************
 *                                init_fs
 *****************************************************************************/
/**
 * <Ring 1> Do some preparation.
 * 
 *****************************************************************************/
void init_fs()
{
	/* open the device: hard disk */
	hd_open(MINOR(ROOT_DEV));

	mkfs();
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
PRIVATE void mkfs()
{
	MESSAGE driver_msg;
	int i, j;

	int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */

	/* get the geometry of ROOTDEV */
	struct part_info geo;
	driver_msg.type		= DEV_IOCTL;
	driver_msg.DEVICE	= MINOR(ROOT_DEV);
	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	driver_msg.PROC_NR	= 0; //TASK_FS;
    //assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	//send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
	hd_ioctl(&driver_msg);
	
	//�������
	//printl("dev size: 0x%x sectors\n", geo.size);
	char strbuf[200];
	sprintf(strbuf,"dev size: 0x%x sectors", geo.size);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 420+16+16, 420+8*50, 420+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 420+16+16, COL8_000000, strbuf);

	/************************/
	/*      super block     */
	/************************/
	struct super_block sb;
	sb.magic	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;  //512 * 8 = 4096��i_node
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;  //i_node��ռ�õ�������=512*32 / 512 = 32
	sb.nr_sects	  = geo.size; /* partition size in sector�� ��������ܹ��ж��ٸ����� */
	sb.nr_imap_sects  = 1; //inode-map��ռ�õ�������
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1; //secotr-map��ռ�õ�������
	sb.n_1st_sect	  = 1 + 1 +   /* boot sector & super block */
	sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects; //�������ĵ�һ���������
	sb.root_inode	  = ROOT_INODE;  //root directoryռ�õ�inode���
	sb.inode_size	  = INODE_SIZE; 
	struct inode x;
	sb.inode_isize_off= (int)&x.i_size - (int)&x; //i_size��i-node�ṹ�е�ƫ��
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x; //start_sect��i-node�ṹ�е�ƫ��
	sb.dir_ent_size	  = DIR_ENTRY_SIZE; //DIR_ENTRY�ṹ�Ĵ�С
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de; //inode_nr��dir_entry�е�ƫ��
	sb.dir_ent_fname_off = (int)&de.name - (int)&de; //name��dir_entry�е�ƫ��

	memset1(fsbuf, 0x90, SECTOR_SIZE);
	memcpy1(fsbuf, &sb,  SUPER_BLOCK_SIZE);
	
	sprintf(strbuf,"addr of fsbuf = %x fsbuf[0] = %x fusbuf[1] = %x", fsbuf, fsbuf[0], fsbuf[1]);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 680+16+16, 680+8*50, 680+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 680+16+16, COL8_000000, strbuf);

	/* write the super block */
	WR_SECT(ROOT_DEV, 1);

	sprintf(strbuf,"devbase:0x%x00 sb:0x%x00 imap:0x%x00 smap:0x%x00    inodes:0x%x00, 1st_sector:0x%x00", 
			       geo.base * 2, //����Ϊʲô������2
			       (geo.base + 1) * 2,
			       (geo.base + 1 + 1) * 2,
			       (geo.base + 1 + 1 + sb.nr_imap_sects) * 2,
			       (geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
			       (geo.base + sb.n_1st_sect) * 2);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 440+16+16, 440+8*50, 440+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 440+16+16, COL8_000000, strbuf);

	/************************/
	/*       inode map      */
	/************************/
	memset1(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++)
		fsbuf[0] |= 1 << i;

	//assert(fsbuf[0] == 0x1F);/* 0001 1111 : 
	//			  *    | ||||
	//			  *    | |||`--- bit 0 : reserved
	//			  *    | ||`---- bit 1 : the first inode,
	//			  *    | ||              which indicates `/'
	//			  *    | |`----- bit 2 : /dev_tty0
	//			  *    | `------ bit 3 : /dev_tty1
	//			  *    `-------- bit 4 : /dev_tty2
	//			  */
	WR_SECT(ROOT_DEV, 2);

	/************************/
	/*      secter map      */
	/************************/
	memset1(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved
	 *                                `-------- for `/'
	 */
	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects);

	/* zeromemory the rest sector-map */
	memset1(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset1(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files:
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);

	/************************/
	/*          `/'         */
	/************************/
	memset1(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry * pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		
		//TODO ����
		//sprintf(pde->name, "dev_tty%d", i);
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
}

/*****************************************************************************
 *                                rw_sector
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 * 
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 * 
 * @return Zero if success.
 *****************************************************************************/
//TODO: ��pos�����͸�Ϊu32�����Ӳ�̴�С���ܳ���4G
PUBLIC int rw_sector(int io_type, int dev, u32 pos, int bytes, int proc_nr,
		     void* buf)
{
	MESSAGE driver_msg;

	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.BUF		= buf;
	driver_msg.CNT		= bytes;
	driver_msg.PROC_NR	= proc_nr;
	
	char strbuf[200];
	sprintf(strbuf,"add of buf = %x, pos = %d, bytes = %d",buf, pos,bytes);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 660+16+16, 660+8*50, 660+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 660+16+16, COL8_000000, strbuf);
	
	//assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	//send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	hd_rdwt(&driver_msg);
	return 0;
}

