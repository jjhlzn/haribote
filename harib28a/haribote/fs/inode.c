/*
 *  linux/fs/inode.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <sys/stat.h>

//#include <linux/sched.h>
//#include <linux/kernel.h>
//#include <linux/mm.h>
//#include <asm/system.h>

#include "bootpack.h"
#include "fs.h"

struct m_inode inode_table[NR_INODE]={{0,},};

static void read_inode(struct m_inode * inode);
static void write_inode(struct m_inode * inode);

static inline void wait_on_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	sti();
}

static inline void lock_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	inode->i_lock=1;
	sti();
}

static inline void unlock_inode(struct m_inode * inode)
{
	inode->i_lock=0;
	wake_up(&inode->i_wait);
}

/* 释放设备dev在内存i节点表中的所有i节点 */
void invalidate_inodes(int dev)
{
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dev == dev) {
			if (inode->i_count)
				printk("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0;
		}
	}
}

/* 同步所有i节点 */
//把内存i节点表中所有i节点与设备上i节点作同步操作
void sync_inodes(void)
{
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dirt && !inode->i_pipe)
			write_inode(inode);
	}
}

/* 文件数据块映射到盘块的处理操作 (block位图处理函数，bmap - block map) */
//参数：inode  - 文件的i节点指针
//     block  - 文件中的数据块号
//     create - 创建块标志
//该函数把指定的数据块block对应到设备上逻辑块上，并返回逻辑块号。如果创建标志置位，则在设备上
//对应不存在时就申请信磁盘块，返回文件数据块block对应在设备上的逻辑块号（盘块号）
static int _bmap(struct m_inode * inode,int block,int create)
{
	struct buffer_head * bh;
	int i;

	if (block<0)
		panic("_bmap: block<0");
	if (block >= 7+512+512*512)
		panic("_bmap: block>big");
	if (block<7) {
		if (create && !inode->i_zone[block])
			if ( (inode->i_zone[block]=new_block(inode->i_dev)) ) {
				inode->i_ctime=CURRENT_TIME;
				inode->i_dirt=1;
			}
		return inode->i_zone[block];
	}
	block -= 7;
	if (block<512) {
		if (create && !inode->i_zone[7])
			if ( (inode->i_zone[7]=new_block(inode->i_dev)) ) {
				inode->i_dirt=1;
				inode->i_ctime=CURRENT_TIME;
			}
		if (!inode->i_zone[7])
			return 0;
		if (!(bh = bread(inode->i_dev,inode->i_zone[7])))
			return 0;
		i = ((unsigned short *) (bh->b_data))[block];
		if (create && !i)
			if ( (i=new_block(inode->i_dev)) ) {
				((unsigned short *) (bh->b_data))[block]=i;
				bh->b_dirt=1;
			}
		brelse(bh);
		return i;
	}
	block -= 512;
	if (create && !inode->i_zone[8])
		if ( (inode->i_zone[8]=new_block(inode->i_dev)) ) {
			inode->i_dirt=1;
			inode->i_ctime=CURRENT_TIME;
		}
	if (!inode->i_zone[8])
		return 0;
	if (!(bh=bread(inode->i_dev,inode->i_zone[8])))
		return 0;
	i = ((unsigned short *)bh->b_data)[block>>9];
	if (create && !i)
		if ( (i=new_block(inode->i_dev)) ) {
			((unsigned short *) (bh->b_data))[block>>9]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	if (!i)
		return 0;
	if (!(bh=bread(inode->i_dev,i)))
		return 0;
	i = ((unsigned short *)bh->b_data)[block&511];
	if (create && !i)
		if ( (i=new_block(inode->i_dev)) ) {
			((unsigned short *) (bh->b_data))[block&511]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	return i;
}

/* 读文件数据块block在设备上对应的逻辑块号 */
//参数: inode - 文件的内存i节点指针
//     block - 文件中的数据块号
int bmap(struct m_inode * inode,int block)
{
	return _bmap(inode,block,0);
}

/* 取文件数据块block在设备上对应的逻辑块号 */
//如果对应的逻辑块不存在就创建一块，返回设备上对应的已存在或新建的逻辑块号
//参数：inode - 文件的内存i节点指针
//     block - 文件中的数据块号
int create_block(struct m_inode * inode, int block)
{
	return _bmap(inode,block,1);
}
		
/* 放回（放置）一个i节点（回写入设备） */
//该函数主要用于把i节点引用计数递减1，并且若是管道i节点，则唤醒等待的进程
//若是块设备文件i节点则刷新设备。并且若i节点的链接计数为0，则释放该i节点占用的所有磁盘逻辑块，
//并释放该i节点
void iput(struct m_inode * inode)
{
	if (!inode)
		return;
	wait_on_inode(inode);
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	if (inode->i_pipe) {  //不支持pipe
		//wake_up(&inode->i_wait);
		//if (--inode->i_count)
		//	return;
		//free_page(inode->i_size);
		//inode->i_count=0;
		//inode->i_dirt=0;
		//inode->i_pipe=0;
		return;
	}
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
	}
repeat:
	if (inode->i_count>1) {
		inode->i_count--;
		return;
	}
	//如果该i节点链接数为0,则释放数据盘块和i节点
	if (!inode->i_nlinks) {
		truncate(inode);
		free_inode(inode);
		return;
	}
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */
		wait_on_inode(inode);
		goto repeat;
	}
	inode->i_count--;
	return;
}

/* 从i节点表中获取一个空闲i节点项 */
//寻找引用计数count为0的i节点，并将其写盘后清零，返回其指针。引用计数被置1。
struct m_inode * get_empty_inode(void)
{
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table;
	int i;

	do {
		inode = NULL;
		for (i = NR_INODE; i ; i--) {
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count) {
				inode = last_inode;
				if (!inode->i_dirt && !inode->i_lock)
					break;
			}
		}
		if (!inode) {
			for (i=0 ; i<NR_INODE ; i++)
				printk("%04x: %6d\t",inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem");
		}
		wait_on_inode(inode);
		while (inode->i_dirt) {
			write_inode(inode);
			wait_on_inode(inode);
		}
	} while (inode->i_count);
	memset(inode,0,sizeof(*inode));
	inode->i_count = 1;
	return inode;
}

/* 获取管道i节点 */
struct m_inode * get_pipe_inode(void)
{
	//不支持PIPE
	//struct m_inode * inode;

	//if (!(inode = get_empty_inode()))
	//	return NULL;
	//if (!(inode->i_size=get_free_page())) {
	//	inode->i_count = 0;
	//	return NULL;
	//}
	//inode->i_count = 2;	/* sum of readers/writers */
	//PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;
	//inode->i_pipe = 1;
	//return inode;
	return NULL;
}

/*  获得一个i节点 */
//参数：dev-设备号； nr - i节点号
//从设备上读取指定节点号的i节点到内存i节点表中，并返回该i节点指针。
//首先在i节点表中搜寻，若找到指定节点号的i节点则经过一些判断后返回该i节点指针
//否则从设备dev上读取指定i节点号的i节点信息放入i节点表中，并返回该i节点指针
struct m_inode * iget(int dev,int nr)
{
	struct m_inode * inode, * empty;

	if (!dev)
		panic("iget with dev==0");
	empty = get_empty_inode();
	inode = inode_table;
	while (inode < NR_INODE+inode_table) {
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode++;
			continue;
		}
		wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode = inode_table;
			continue;
		}
		inode->i_count++;
		if (inode->i_mount) {
			int i;

			for (i = 0 ; i<NR_SUPER ; i++)
				if (super_block[i].s_imount==inode)
					break;
			if (i >= NR_SUPER) {
				printk("Mounted inode hasn't got sb\n");
				if (empty)
					iput(empty);
				return inode;
			}
			iput(inode);
			dev = super_block[i].s_dev;
			nr = ROOT_INO;
			inode = inode_table;
			continue;
		}
		if (empty)
			iput(empty);
		return inode;
	}
	if (!empty)
		return (NULL);
	inode=empty;
	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);
	return inode;
}

/* 读取指定i节点信息 */
//从设备上读取含有指定i节点信息的i节点盘块，然后复制到指定的i节点结构中。为了确定i节点所在的逻辑
//块号（或缓冲块），必须首先读取相应设备上的超级块，以获取用于计算逻辑块号的每块i节点数信息（INODE_PER_BLOCK)。
//在计算出i节点所在的逻辑块号后，就把该逻辑块读入一缓冲块中。然后把缓冲块中响应位置处的i节点复制到
//参数指定的位置处。
static void read_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to read inode without dev");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
			[(inode->i_num-1)%INODES_PER_BLOCK];
	brelse(bh);
	unlock_inode(inode);
}

/* 将i节点信息写入缓冲区中 */
static void write_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!inode->i_dirt || !inode->i_dev) {
		unlock_inode(inode);
		return;
	}
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to write inode without device");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num-1)%INODES_PER_BLOCK] =
			*(struct d_inode *)inode;
	bh->b_dirt=1;
	inode->i_dirt=0;
	brelse(bh);
	unlock_inode(inode);
}
