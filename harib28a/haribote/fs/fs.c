#include "bootpack.h"
#include "hd.h"
#include "fs.h"
#include <string.h>
#include <stdio.h>

void init_fs();
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);
/**
 * 6MB~7MB: buffer for FS
 */
PUBLIC	u8 *		fsbuf;
PUBLIC	const int	FSBUF_SIZE	= 0x100000;

struct file_desc	f_desc_table[NR_FILE_DESC];
struct inode		inode_table[NR_INODE];
struct super_block	super_block[NR_SUPER_BLOCK];
struct inode *		root_inode;

/*****************************************************************************
 *                                init_fs
 *****************************************************************************/
/**
 * <Ring 1> Do some preparation.
 * 
 *****************************************************************************/
void init_fs()
{
	
	int i;
	/* f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		memset1(&f_desc_table[i], 0, sizeof(struct file_desc));

	/* inode_table[] */
	for (i = 0; i < NR_INODE; i++)
		memset1(&inode_table[i], 0, sizeof(struct inode));

	/* super_block[] */
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;

	/* open the device: hard disk */
	hd_open(MINOR(ROOT_DEV));

	RD_SECT(ROOT_DEV,1);
	sb = (struct super_block *)fsbuf;
	if(sb->magic != MAGIC_V1){
		debug("mkfs");
		mkfs();
	}
	
	/* load super block of ROOT */
	read_super_block(ROOT_DEV);

	sb = get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);

	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
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
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	fsbuf = (u8 *)memman_alloc(memman,FSBUF_SIZE);

	/* get the geometry of ROOTDEV */
	struct part_info geo;
	driver_msg.type		= DEV_IOCTL;
	driver_msg.DEVICE	= MINOR(ROOT_DEV);
	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	driver_msg.PROC_NR	= 0; //TASK_FS
	hd_ioctl(&driver_msg);
	
	debug("dev size: 0x%x sectors", geo.size);

	/************************/
	/*      super block     */
	/************************/
	struct super_block sb;
	sb.magic	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;  //512 * 8 = 4096个i_node
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;  //i_node所占用的扇区数=4096 * 32 / 512 = 32
	sb.nr_sects	  = geo.size; /* partition size in sector， 这个分区总共有多少个扇区 */
	sb.nr_imap_sects  = 1; //inode-map所占用的扇区数
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1; //secotr-map所占用的扇区数
	sb.n_1st_sect	  = 1 + 1 +   /* boot sector & super block */
						sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects; //数据区的第一个扇区编号
	sb.root_inode	  = ROOT_INODE;  //root directory占用的inode编号
	sb.inode_size	  = INODE_SIZE; 
	struct inode x;
	sb.inode_isize_off= (int)&x.i_size - (int)&x; //i_size在i-node结构中的偏移
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x; //start_sect在i-node结构中的偏移
	sb.dir_ent_size	  = DIR_ENTRY_SIZE; //DIR_ENTRY结构的大小
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de; //inode_nr在dir_entry中的偏移
	sb.dir_ent_fname_off = (int)&de.name - (int)&de; //name在dir_entry中的偏移

	memset1(fsbuf, 0x90, SECTOR_SIZE);
	memcpy1(fsbuf, &sb,  SUPER_BLOCK_SIZE);
	
	debug("sb.n_1st_sect = %d",sb.n_1st_sect);
	
	/* write the super block */
	WR_SECT(ROOT_DEV, 1);

	/************************/
	/*       inode map      */
	/************************/
	memset1(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++)
		fsbuf[0] |= 1 << i;

	assert(fsbuf[0] == 0x1F);/* 0001 1111 : 
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
		debug("pi->i_start_sect = %d",MAKE_DEV(DEV_CHAR_TTY, i));
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
		
		sprintf(pde->name, "dev_tty%d", i);
		
		debug("pde->inode_nr = %d",pde->inode_nr);
		debug("pde->name = %s",pde->name);
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
//TODO: 将pos的类型改为u32，因此硬盘大小不能超过4G
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
	
	//assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	//send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	hd_rdwt(&driver_msg);
	return 0;
}
 
/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 * 
 * @param dev  From which device the super block comes.
 *****************************************************************************/
PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= 0;
	//assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	//send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	hd_rdwt(&driver_msg);

	/* find a free slot in super_block[] */
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		//panic("super_block slots used up");
		;

	//assert(i == 0); /* currently we use only the 1st slot */

	struct super_block * psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;
	super_block[i].sb_dev = dev;
}

PUBLIC int do_close(int fd, struct TASK *pcaller)
{
	put_inode(pcaller->filp[fd]->fd_inode);
	pcaller->filp[fd]->fd_inode = 0;
	pcaller->filp[fd] = 0;

	return 0;
}

/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 * 
 * @param dev Device nr.
 * 
 * @return Super block ptr.
 *****************************************************************************/
PUBLIC struct super_block * get_super_block(int dev)
{
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		if (sb->sb_dev == dev)
			return sb;

	print_on_screen("super block of devie not found.");

	return 0;
}


/*****************************************************************************/
/**
 * Get all the inodes of files
 *
******************************************************************************/
PUBLIC struct FILEINFO* get_all_files(int dev){
	int i, j;
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	/**
	 * Search the dir for the files.
	 */
	int dir_blk0_nr = root_inode->i_start_sect;
	debug("root_inode->i_start_sect = %d",root_inode->i_start_sect);
	int nr_dir_blks = (root_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries = root_inode->i_size / DIR_ENTRY_SIZE; /**
															   * including unused slots
															   * (the file has been deleted
															   * but the slot is still there)
															   */
	debug("nr_dir_blks = %d",nr_dir_blks);
	debug("nr_dir_entries = %d",nr_dir_entries);

	
	int m = 0;
	struct dir_entry * pde;
	struct FILEINFO * p_file = (struct FILEINFO * )memman_alloc(memman, (nr_dir_entries+1) * sizeof(struct FILEINFO));
	struct FILEINFO * file_list = p_file;
	//获取文件名
	for (i = 0; i < nr_dir_blks; i++) {
		debug("read sect[%d]",dir_blk0_nr + i);
		RD_SECT(root_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;
			strcpy(p_file->name,pde->name);
			p_file->size = pde->inode_nr; //暂时用i_size来存储inode_nr
			p_file++;
			debug(pde->name);
			
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}
	
	//获取文件大小
	p_file = file_list; //重置p_file到开头
	m=0;
	for (i = 0; i < nr_dir_blks; i++) {
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;
			struct inode *tmp_inode = get_inode(dev,p_file->size); //这时p_file->inode_nr
													//get_inode需要读取扇区，会覆盖fsbuf,因此需要和上面获取文件名的循环独立开
			p_file->size = tmp_inode->i_size;
			debug("filename = [%s], filesize = [%d]",p_file->name,p_file->size);
			p_file++;
			
			
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}
	p_file -> size = -1;
	return file_list;
}


/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 * 
 * @param dev Device nr.
 * @param num I-node nr.
 * 
 * @return The inode ptr requested.
 *****************************************************************************/
PUBLIC struct inode * get_inode(int dev, int num)
{
	if (num == 0) 
		return 0;

	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");
	
	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(dev, blk_nr);
	struct inode * pinode =
		(struct inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}

/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 * 
 * @param pinode I-node ptr.
 *****************************************************************************/
PUBLIC void put_inode(struct inode * pinode)
{
	//assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 * 
 * @param p I-node ptr.
 *****************************************************************************/
PUBLIC void sync_inode(struct inode * p)
{
	struct inode * pinode;
	struct super_block * sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf +
				 (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);
}



/* 打开进程的三个标准文件，标准输入、标准输出、标准出错 */
PUBLIC void open_std_files(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)
			break;
	if (i >= NR_FILE_DESC)
		panic("f_desc_table[] is full (PID:%d)", task->pid);
	task->filp[STDIN] = &f_desc_table[i];
	
	task->filp[STDIN]->fd_mode = O_RD;
	task->filp[STDIN]->fd_pos = 0;
	task->filp[STDIN]->fd_cnt = 1;
	struct inode* STDIN_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	STDIN_node->i_mode = I_CHAR_SPECIAL;
	//STDIN_node->i_size = task->pid;
	task->filp[STDIN]->fd_inode = STDIN_node;
	
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)
			break;
	if (i >= NR_FILE_DESC)
		panic("f_desc_table[] is full (PID:%d)", task->pid);
	task->filp[STDOUT] = &f_desc_table[i];
	
	task->filp[STDOUT]->fd_mode = O_WR;
	task->filp[STDOUT]->fd_pos = 0;
	task->filp[STDOUT]->fd_cnt = 1;
	struct inode* STDOUT_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	STDOUT_node->i_mode = I_CHAR_SPECIAL;
	//STDOUT_node->i_size = task->pid;
	task->filp[STDOUT]->fd_inode = STDOUT_node;
	
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)
			break;
	if (i >= NR_FILE_DESC)
		panic("f_desc_table[] is full (PID:%d)", task->pid);
	task->filp[STDERR] = &f_desc_table[i];
	
	task->filp[STDERR]->fd_mode = O_WR;
	task->filp[STDERR]->fd_pos = 0;
	task->filp[STDERR]->fd_cnt = 1;
	struct inode* STDERR_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	STDERR_node->i_mode = I_CHAR_SPECIAL;
	//STDERR_node->i_size = task->pid;
	task->filp[STDERR]->fd_inode = STDERR_node;
}



