/*
 *  linux/fs/bitmap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */
#include <string.h>

//#include <linux/sched.h>
//#include <linux/kernel.h>
#include "bootpack.h"
#include "sys/types.h"
#include "fs.h"

//将指定地址(addr)处的一块1024字节内存清零
//CHANGED
#define clear_block(addr) \
__asm__("cld\n\t" \
"rep\n\t" \
"stosl" \
: :"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)))
	

//将指定地址开始的第nr个位偏移处的比特位置(nr可大于32！）。返回原比特位值。
#define set_bit(nr,addr) ({\
	register int res; \
	__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
						 "=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})


//复位指定地址开始的第nr位偏移处的比特位，返回原比特位值的反码	
#define clear_bit(nr,addr) ({\
	register int res; \
	__asm__ __volatile__("btrl %2,%3\n\tsetnb %%al": \
						 "=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

//从addr开始寻找第1个0值比特位
//CHANGED
#define find_first_zero(addr) ({ \
	int __res; \
	__asm__("cld\n" \
			"1:\tlodsl\n\t" \
			"notl %%eax\n\t" \
			"bsfl %%eax,%%edx\n\t" \
			"je 2f\n\t" \
			"addl %%edx,%%ecx\n\t" \
			"jmp 3f\n" \
			"2:\taddl $32,%%ecx\n\t" \
			"cmpl $8192,%%ecx\n\t" \
			"jl 1b\n" \
			"3:" \
			:"=c" (__res):"c" (0),"S" (addr)); \
__res;})

/*  释放设备dev上数据区中的逻辑块block */
//复位指定逻辑块block对应的逻辑块位图比特位
//参数：dev是设备号，block是逻辑块号（盘块号）
void free_block(int dev, int block)
{
	struct super_block * sb;
	struct buffer_head * bh;

	if (!(sb = get_super(dev)))
		panic("trying to free block on nonexistent device");
	if (block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("trying to free block not in datazone");
	
	bh = get_hash_table(dev,block);
	if (bh) {
		if (bh->b_count > 1) {
			//printk("trying to free block (%04x:%d), count=%d\n",
			//	dev,block,bh->b_count);
			brelse(bh);
			return;
		}
		bh->b_dirt=0;
		bh->b_uptodate=0;
		if (bh->b_count)
			brelse(bh);
	}
	block -= sb->s_firstdatazone - 1 ;
	if (clear_bit(block&8191,sb->s_zmap[block/8192]->b_data)) {
		printk("block (%04x:%d) ",dev,block+sb->s_firstdatazone-1);
		panic("free_block: bit already cleared");
	}
	sb->s_zmap[block/8192]->b_dirt = 1;
}

/* 向设备申请一个逻辑块(盘块，区块） */
//函数首先取得设备的超级块，并在超级块中的逻辑块位图中寻找第一个0值比特位（代表一个空闲逻辑块）。
//然后置位对应逻辑块在逻辑块位图中的比特位。接着为该逻辑块在缓存中取得一块对应缓冲块。最后将该
//缓冲块清零，并设置其已更新标志和已修改标志。并返回逻辑块号。函数执行成功则返回逻辑块号（盘块
//号），否则返回0。
int new_block(int dev)
{
	struct buffer_head * bh;
	struct super_block * sb;
	int i,j;

	if (!(sb = get_super(dev)))
		panic("trying to get new block from nonexistant device");
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if ( (bh=sb->s_zmap[i]) )
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	if (i>=8 || !bh || j>=8192) //找不到空闲块
		return 0;
	
	if (set_bit(j,bh->b_data))
		panic("new_block: bit already set");
	bh->b_dirt = 1;
	j += i*8192 + sb->s_firstdatazone-1;
	if (j >= sb->s_nzones)
		return 0;
	
	//在高速缓冲中为该设备上指定的逻辑块号取得一个缓冲号，并返回缓冲块头指针。因为刚取得的逻辑块
	//其引用次数一定为1(getblk()中会设置），因此若不为1则停机。最后将新逻辑块清零，并设置其已
	//更新标志和已修改标志。然后释放对应缓冲块，返回逻辑块号
	if (!(bh=getblk(dev,j)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)  //缓冲块的引用次数一定为1
		panic("new block: count is != 1");
	clear_block(bh->b_data);
	bh->b_uptodate = 1; //已更新
	bh->b_dirt = 1;     //还没有写到设备上
	brelse(bh);
	return j;
}

/* 释放指定的i节点 */
void free_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;

	if (!inode)
		return;
	if (!inode->i_dev) {
		memset(inode,0,sizeof(*inode));
		return;
	}
	if (inode->i_count>1) {
		printk("trying to free inode with count=%d\n",inode->i_count);
		panic("free_inode");
	}
	if (inode->i_nlinks)
		panic("trying to free inode with links");
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode on nonexistent device");
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");
	if (!(bh=sb->s_imap[inode->i_num>>13]))
		panic("nonexistent imap in superblock");
	if (clear_bit(inode->i_num&8191,bh->b_data))
		printk("free_inode: bit already cleared.\n\r");
	bh->b_dirt = 1;
	memset(inode,0,sizeof(*inode));
}

/* 为设备dev建立一个新i节点。初始化并返回该新i节点的指针 */
//在内存i节点表中获取一个空闲i节点表项，并从i节点位图中找一个空闲i节点。
struct m_inode * new_inode(int dev)
{
	struct m_inode * inode;
	struct super_block * sb;
	struct buffer_head * bh;
	int i,j;

	if (!(inode=get_empty_inode()))
		return NULL;
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if (  (bh=sb->s_imap[i]) )
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {
		iput(inode);
		return NULL;
	}
	if (set_bit(j,bh->b_data))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;
	inode->i_count=1;
	inode->i_nlinks=1;
	inode->i_dev=dev;
	inode->i_uid=current->euid;
	inode->i_gid=current->egid;
	inode->i_dirt=1;
	inode->i_num = j + i*8192;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	return inode;
}
