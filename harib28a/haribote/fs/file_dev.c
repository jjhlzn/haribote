/*
 *  linux/fs/file_dev.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <errno.h>
//#include <fcntl.h>

//#include <linux/sched.h>
//#include <linux/kernel.h>
#include <asm/segment.h>
#include "errno.h"
#include "bootpack.h"
#include "sys/types.h"
#include "fs.h"
#include "fcntl.h"

#define ERROR		99
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

////文件读函数 - 根据i节点和文件结构，读取文件中数据
//由i节点我们可以知道设备号，由filp结构可以知道文件中当前读写指针位置。buf指定用户空间
//中缓冲区的位置，count是需要读取的字节数。返回值是实际读取的字节数，或出错号
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left,chars,nr;
	struct buffer_head * bh;

	//首先判断参数的有效性
	if ((left=count)<=0)
		return 0;
	
	while (left) { 
		//利用bmap获得磁盘逻辑块号
		if (  (nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE))  ) {
			if (!(bh=bread(inode->i_dev,nr)))
				break;
		} else
			bh = NULL;
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE-nr , left );
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			char * p = nr + bh->b_data;
			while (chars-->0)
				put_fs_byte(*(p++),buf++);
			brelse(bh);
		} else {
			while (chars-->0)
				put_fs_byte(0,buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count-left)?(count-left):-ERROR;
}

////文件写函数 - 根据i节点和文件结构信息，将用户数据写入文件中
//由i节点我们可以知道设备号，而由file结构可以知道文件中当前读写指针位置。buf指定用户态缓冲区
//的位置，count为需要写入的字节数。返回值是实际写入的字节数，或出错号（小于0）
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;
	int block,c;
	struct buffer_head * bh;
	char * p;
	int i=0;

/*
 * ok, append may not work when many processes are writing at the same time
 * but so what. That way leads to madness anyway.
 */
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	while (i<count) {
		if (!(block = create_block(inode,pos/BLOCK_SIZE)))
			break;
		if (!(bh=bread(inode->i_dev,block)))
			break;
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE-c;
		if (c > count-i) c = count-i;
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-->0)
			*(p++) = get_fs_byte(buf++);
		brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i?i:-1);
}
