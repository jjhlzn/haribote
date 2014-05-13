/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

#include <stdarg.h>
 
#include "bootpack.h"
#include "sys/types.h"
#include "fs.h"

//extern int end;
//struct buffer_head * start_buffer = (struct buffer_head *) &end; //TODO: 高速缓冲开始地址
struct buffer_head * start_buffer = (struct buffer_head *) 0x00a00000;
struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;

static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock){
		debug("about to sleep");
		sleep_on(&bh->b_wait);
	}
	sti();
}

/* 设备数据同步 */
//同步设备和内存高速缓冲中的数据， sync_inodes()定义在inode.c中
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;
	
	//首先调用i节点同步函数，把内存i节点表中所有修改过的i节点写入高速缓冲中。然后扫描所有高速
	//缓冲区，对已被修改的缓冲块产生写盘请求，将缓冲中数据写入盘中，做到高速缓冲中的数据与设备
	//中的同步。
	sync_inodes();		/* write out inodes into buffers */
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}


/*  对指定设备进行高速缓冲数据与设备上数据的同步操作  */
//QUESTION: 为什么要两次写入数据块  ??
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	sync_inodes();
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

//使指定设备在高速缓冲区中的数据无效
void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev)
			bh->b_uptodate = bh->b_dirt = 0;
	}
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 */
//TODO: 关于软盘的操作
void check_disk_change(int dev)
{
	int i;

	if (MAJOR(dev) != 2)
		return;
	panic("only suport hd");
	//if (!floppy_change(dev & 0x03))
	//	return;
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);
	invalidate_buffers(dev);
}

#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

/*  从hash队列中空闲缓冲队列中移走缓冲块 */
//hash队列是双向链表结构，空闲缓冲队列是双向循环链表结构
static inline void remove_from_queues(struct buffer_head * bh)
{
/* remove from hash-queue */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* remove from free list */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)
		free_list = bh->b_next_free;
}

static inline void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	if(bh->b_next)
		bh->b_next->b_prev = bh;
}

static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

	for (;;) {
		if (!(bh=find_buffer(dev,block)))
			return NULL;
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		bh->b_count--;
	}
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
/*  取高速缓冲中指定的缓冲块 */
//检查指定（设备号和块号）的缓冲区是否已经在高速缓冲中。如果指定块已经在高速缓冲中，则返回
//对应缓冲区头指针退出；如果不在，就需要在高速缓冲中设置一个对应设备号和块号的新项。返回相应
//缓冲区头指针
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	if ( (bh = get_hash_table(dev,block)) )
		return bh;
	tmp = free_list;
	do {
		if (tmp->b_count)
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {
			bh = tmp;
			if (!BADNESS(tmp))
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list);
	
	//如果循环检查所有缓冲块都正在被使用（所有缓冲块的头部引用计数都>0)中，则睡眠等待有空闲
	//缓冲块可用。当有空闲块可用时本进程会被明确地唤醒。然后我们就跳转到函数开始处重新查找空闲
	//缓冲块
	if (!bh) {
		sleep_on(&buffer_wait);
		goto repeat;
	}
	
	//如果跑到这里，说明已经找到一个空闲的缓冲区块。于是先等待该缓冲区解锁（如果已经被上锁的话）。
	wait_on_buffer(bh);
	if (bh->b_count)
		goto repeat;
	
	//如果该缓冲区已被修改，则将数据写盘，并再次等待缓冲区解锁。
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);
		wait_on_buffer(bh);
		if (bh->b_count)
			goto repeat;
	}
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	//当进程为了等待该缓冲块而睡眠时，其他进程可能已经将该缓冲块进入高速缓冲中，因此我们也要对此进行检查
	if (find_buffer(dev,block))
		goto repeat;
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	
	//到了这里，最终我们知道该缓冲块是指定参数的唯一一块，而且目前还没有被占用（b_count=0)
	//也没有被上锁(b_lock=0)，而且是干净的(b_dirt=0)
	//于是我们占用此缓冲块。置引用计数为1，复位修改标志和有效标志
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_uptodate=0;
	//从hash队列和空闲块链表中移出该缓冲区头，让该缓冲区用于指定设备和其上的指定块。然后根据
	//此新的设备号和块号重新插入空闲链表和hash队列新位置处。并最终返回缓冲头指针。
	remove_from_queues(bh);
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);
	return bh;
}

/*  释放指定缓冲块 */
//等待缓冲块解锁，然后引用计数递减1，并明确
void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
//从设备上读取指定的数据块并返回含有数据的缓冲区。如果指定的块不存在则返回NULL。
struct buffer_head * bread(int dev,int block)
{
	//debug("bread1");
	//debug("dev = %d, block = %d",dev, block);
	struct buffer_head * bh;

	//首先在高速缓冲中申请一块缓冲块。如果返回值是NULL，则表示内核出错，停机。然后我们判断
	//其中是否已有可用数据。如果该缓冲块中数据是有效的（已更新的）可以直接使用，则返回。
	if (!(bh=getblk(dev,block)))
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)
		return bh;
	//debug("bread2");
	ll_rw_block(READ,bh);
	//debug("about to wait on buffer");
	wait_on_buffer(bh);
	//debug("wait end");
	//debug("bh->b_uptodate = %d",bh->b_uptodate);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return NULL;
}

/* 复制内存块 */
//从from地址复制一块(1024字节）数据到to位置
//TODO
//CHANGED
#define COPYBLK(from,to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) )

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 */
/* 读设备上一个页面（4个缓冲块）的内容到指定内存地址处 */
//bread_page一次读四个缓冲块数据到内存指定的地址处。它是一个完整的函数
//参数address是保存页面数据的地址：dev是指定的设备号；b[4]是含有4个设备数据块号的数据。该函数仅
//用于mm/memory.c文件中的do_no_page()函数中
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];
	int i;

	for (i=0 ; i<4 ; i++)
		if (b[i]) {
			if ( (bh[i] = getblk(dev,b[i])) )
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ,bh[i]);
		} else
			bh[i] = NULL;
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
		if (bh[i]) {
			wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
				COPYBLK((unsigned long) bh[i]->b_data,address);
			brelse(bh[i]);
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
//从指定设备读取指定的一些块
//函数参数个数课表，是一系列指定的块号，成功是返回第1块的缓冲块头指针，否则返回NULL
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	va_start(args,first);
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	while ((first=va_arg(args,int))>=0) {
		tmp=getblk(dev,first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA,tmp);
			tmp->b_count--;
		}
	}
	va_end(args);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return (NULL);
}

/**     缓冲区初始化函数         **/
//参数buffer_end是缓冲区内存末端 
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer;
	void * b;
	int i;

	//确定实际的缓冲区末端地址
	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;
	
	//缓冲区初始化，初始化后，形成一个空闲循环双向链表
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
		h->b_dev = 0;
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;
		h->b_prev_free = h-1;
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;
		if (b == (void *) 0x100000) //跳过640KB~1MB的内存空间
			b = (void *) 0xA0000;
	}
	h--;  //让h指向最后一个有效缓冲块头
	free_list = start_buffer;
	free_list->b_prev_free = h;
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)
		hash_table[i]=NULL;
}	
