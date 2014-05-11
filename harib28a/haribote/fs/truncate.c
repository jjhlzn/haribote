/*
 *  linux/fs/truncate.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <linux/sched.h>

#include "bootpack.h"
#include <sys/stat.h>
#include "fs.h"

/* 释放所有一次间接块。 */
//参数dev是文件系统所在设备的设备号；block是逻辑块号
static void free_ind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	//读取一次间接块，并释放其上表明使用的所有逻辑块，然后释放一次间接块的缓冲块。函数free_block()
    //用于释放设备上指定逻辑块号的磁盘块
	if ( (bh=bread(dev,block)) ) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_block(dev,*p);
		brelse(bh);
	}
	free_block(dev,block);
}

/* 释放所有二次间接快 */
static void free_dind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if ( (bh=bread(dev,block)) ) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_ind(dev,*p);
		brelse(bh);
	}
	free_block(dev,block);
}

/* 截断文件数据函数 */
//将节点对应的文件长度截为0,并释放占用的设备空间
void truncate(struct m_inode * inode)
{
	int i;
	//首先判断指定i节点有效性。如果不是常规文件与目录文件，则返回。
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	//释放数据块
	for (i=0;i<7;i++)
		if (inode->i_zone[i]) {
			free_block(inode->i_dev,inode->i_zone[i]);
			inode->i_zone[i]=0;
		}
	free_ind(inode->i_dev,inode->i_zone[7]);
	free_dind(inode->i_dev,inode->i_zone[8]);
	inode->i_zone[7] = inode->i_zone[8] = 0;
	inode->i_size = 0;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}

