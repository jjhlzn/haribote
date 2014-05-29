#include "bootpack.h"
#include "memory.h"
#include <string.h>

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000


static void prepare_for_open_page();
static void get_empty_page(unsigned int addr_start);
static void oom();
static void un_wp_page(unsigned long * table_entry);
static void copy_page(unsigned long from_paddr, unsigned long to_paddr);
static void do_wp_page(unsigned long error_code,unsigned long address);
static void do_no_page(unsigned long error_code,unsigned long address);

void print_page_tables();
static u32 put_page(unsigned int page, unsigned int address);
char *mem_map = (char *)PAGE_BIT_MAP_ADDR;

////刷新页变换高速缓冲宏函数
//为了提高地址转换的效率，CPU将最近使用的页表数据存放在芯片中高速缓冲中(TLB)。在修改过页表信息后，
//就需要重新刷新该缓冲区。这里使用重新加载页目录基址寄存器CR3的方法来进行刷新。下面eax = PAGE_DIR_ADDR，是页目录的地址
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (PAGE_DIR_ADDR))

/**
  用于计算计算机内存大小。方法：通过不断的测试来获取计算机内存大小。
*/
int memtotal;
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386偐丄486埲崀側偺偐偺妋擣 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386偱偼AC=1偵偟偰傕帺摦偱0偵栠偭偰偟傑偆 */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* 僉儍僢僔儏嬛巭 */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* 僉儍僢僔儏嫋壜 */
		store_cr0(cr0);
	}

	return i;
}


void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* 可用信息数目 */
	man->maxfrees = 0;		/* 用于观察可用状况：frees的最大值 */
	man->lostsize = 0;		/* 释放失败的内存大小总和 */
	man->losts = 0;			/* 释放失败次数 */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* 报告空余内存大小的合计 */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 内存分配 */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 找到了足够大的内存 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* 如果free[i]变成了0，就减掉一条可用信息 */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; 
				}
			}
			return a;
		}
	}
	return 0; /* 没有可用空间 */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放内存 */
{
	int i, j;
	 /* 为便于归纳内存，将free[]按照addr的顺序排列 */    
	/* 所以，先决定应该放在哪里 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 可以预前面的可用内存归纳到一起 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 后面也有 */
				if (addr + size == man->free[i].addr) {
					/* 也可以与后面的可用内存归纳到一起 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]删除*/
					/* free[i]变成0后归纳到前面去 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 结构体赋值 */
					}
				}
			}
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面归纳到一起*/
	if (i < man->frees) {
		/* 后面还有*/
		if (addr + size == man->free[i].addr) {
			/* 可以与后面的内容归纳到一起 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面归纳到一起，也不能与后面归纳到一起 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，向后移动，腾出一点可用空间 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功完成 */
	}
	/* 不能往后移动 */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失败 */
}

unsigned int 
memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	//print_on_screen3("memman_alloc_4k: size = %d",size);
	a = memman_alloc(man, size);
	return a;
}

int 
memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}


/***********************************分页相关**************************************************/
int NO_PAGE_EXP_COUNT = 0;
unsigned long _error_code = 0, _address = 0;
/*  处理Page Fault */
void do_page_fault(unsigned long error_code, unsigned long address) 
{
	NO_PAGE_EXP_COUNT++;
	_error_code = error_code;
	_address = address;
	int p = error_code & 0x01;
	int w_r = (error_code & 0x02) >> 1;
	int u_s = (error_code & 0x04) >> 2;
    debug("do_no_page(%d): code=%d, addr=%d(0x%08.8x)",NO_PAGE_EXP_COUNT,error_code, address, address);
	debug("P = %d, W/R = %d, U/S = %d",p,w_r,u_s);
	if( p == 0 )
		do_no_page(error_code, address);
	else if ( p == 1 && w_r == 1)
		do_wp_page(error_code, address);
	debug("-----------------------------------------");
}


void do_no_page(unsigned long error_code, unsigned long address) 
{
	get_empty_page(address);
}

////初始化内存, 测试内存大小，将内存划分成物理页, 分配和映射内核使用的物理页
void 
mem_init()
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//检测内存
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	//0x00400000-0x00900000放页表，0x00900000-0x00a00000存放分页是否空闲
	//0x00a00000-0x00b00000为高速缓冲 (1MB)
	memman_free(memman, 0x00b00000, 0x00b00000 + 16 * 1024 * 1024); /* 16M供内核分配 */ 
	
	prepare_for_open_page();
	open_page();
}

////分页前的初始化功能，准备页目录和页表
static void 
prepare_for_open_page()
{
	int i;
	u32 laddr;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	
	/* 初始化物理页状态 */
	for(i = 0; i < TOTAL_MEM / 0x1000; i++)
		mem_map[i] = 0;
	
	/* 初始化1024个页目录项 */
	for(i=0; i<1024; i++)
		page_dir_base_addr[i] = 0;
	
	/*  低端的线性地址只有内核会用到的，对于内核用到的地址，线性地址=物理地址 */
	for(i=0; i<16; i++) 
		page_dir_base_addr[i] = (((int)page_dir_base_addr + 4096 * (i+1)) & 0xFFFFFC00) | 0x7;
	page_dir_base_addr[0xe0000000/0x400000] = (( (int)page_dir_base_addr + 4096 * 17) & 0xFFFFFC00) | 0x7;
	int *page_table = (int *)PAGE_DIR_ADDR + 1024;
	/* 映射物理内存 */
	for(laddr = 0, i= 0; laddr < TOTAL_MEM; laddr += 0x1000, i++)
		page_table[i] =  laddr | 0x7;
	/* 映射显存 */
	for(laddr = 0xe0000000; laddr < 0xe0000000 + 192 * 0x1000; laddr += 0x1000, i++) //从3G开始
		page_table[i] = laddr | 0x7;
	
	/* 设置已经使用的内存(内核的代码，数据＋供内核分配的存储空间) */
	for( i = 0; i < LOW_MEM/ 0x1000; i++ )
		mem_map[i] = USED;
	
	//页表最后一项指向它自己
	//page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

////分配一页物理页
static unsigned int 
get_free_page()
{
	int i;
	int has_free_page = 0;
	char *mem_map = (char *)PAGE_BIT_MAP_ADDR;
	for( i = 0; i < get_count_of_total_pages(); i++ ){
		if(mem_map[i] == 0){
			has_free_page = 1;
			break;
		}
	}
	if( has_free_page ){
		mem_map[i]++;
		unsigned int page = i * 4 * 1024;
		memset((void *)page, 0, 0x1000);
		return page;
	}
	else
		return NO_FREE_PAGE_ADDR;
}

////分配物理上连续的n页内存,必须4M对齐
//参数：n -- 字节数
static unsigned int 
get_free_pages(int size)
{
	if( size & 0x3fffff )
		panic("get_cont_free_pages: size shoulbe align with 4M, but size = 0x%x", size);
	
	int n = size >> 22;
	if( !n )
		panic("size can't be zero");
	
	int i,j, start_index = 0, has_free_pages = 0;
	for( i = 0; i < get_count_of_total_pages(); ){
		if(!mem_map[i]){
			has_free_pages = 1;
			start_index = i;
			for(j=0; j<1024*size; j++){
				if(!mem_map[j]){
					continue;
				}else{
					has_free_pages = 0;
					break;
				}
			}
			if(has_free_pages)
				break;
			else
				i += 1024;
		}else{
			i += 1024;
		}
	}
	if( has_free_pages ){
		debug("alloc %d pages", n * 1024);
		int tmp_index = start_index;
		for(i=0; i<n * 1024; i++)
			mem_map[tmp_index++] = 1;
		unsigned int page = start_index * 4 * 1024;
		memset((void *)page, 0, size);
		return page;
	}
	else {
		oom();
		return 0;
	}
}

////分配n页连续物理内存，并把它映射到线性地址
//参数：addr -- 线性地址， n --  需要分配的内存页数
//返回：物理地址
unsigned int 
get_and_map_free_pages(u32 addr, int size)
{
	u32 page = get_free_pages(size);
	debug("page = 0x%x",page);
	if(!page)
		oom();
	int i;
    int n = size >> 12;
	for(i = 0; i<n; i++)
		put_page(page + i * 0x1000,addr + i * 0x1000);
	return page;
}

////将物理内存映射到线性地址上
static u32 
put_page(unsigned int page, unsigned int address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */

	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n",page,address);
	if (mem_map[page>>12] != 1)
		printk("mem_map disagrees with %p at %p\n",page,address);
	page_table = (unsigned long *) (((address>>20) & 0xffc) + PAGE_DIR_ADDR);
	if ((*page_table)&1)
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		if (!(tmp=get_free_page()))
			return 0;
		*page_table = tmp|7;
		page_table = (unsigned long *) tmp;
	}
	page_table[(address>>12) & 0x3ff] = page | 7;
	/* no need for invalidate */
	return page;
}

////分配一页物理内存，并且把这页内存映射到线性地址laddr处
//参数：laddr -- 线性地址
static void 
get_empty_page(unsigned int laddr)
{
	u32 page;
	if (  (page=get_free_page()) == NO_FREE_PAGE_ADDR
	   || !put_page(page,laddr) ){
		oom();
	}
}

////释放物理地址为addr处的一个内存，这个函数被free_page_table使用
void 
free_page(unsigned long addr)
{
	if (addr < LOW_MEM) return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	//addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--) return;
	mem_map[addr]=0;
	panic("trying to free free page");
}

//// 根据指定的线性地址和限长（页表个数），释放对应内存页表指定的内存块并闲置表项空闲。
//参数：from -- 起始线性基地址， size -- 释放的字节长度
int 
free_page_tables(unsigned long from,unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir, nr;

	if (from & 0x3fffff)
		panic("free_page_tables called with wrong alignment");
	if (!from)
		panic("Trying to free up swapper memory space");
	size = (size + 0x3fffff) >> 22;
	dir = (unsigned long *) (( (from>>20)  & 0xffc) + PAGE_DIR_ADDR); //页目录项地址
	debug("dir = 0x%x, size = %d",dir,size);
	for ( ; size-->0 ; dir++) {
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *) (0xfffff000 & *dir); //页表地址(物理地址）
		for (nr=0 ; nr<1024 ; nr++) {
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table); //释放该页
			*pg_table = 0;
			pg_table++;
		}
		free_page(0xfffff000 & *dir);
		*dir = 0;
	}
	invalidate();
	return 0;
}

////复制页目录和页表项
//注意1：这里并不复制任何内存块 - 内存块的地址需要是4Mb的倍数（正好一个页目录表项对应的内存
//长度），因为这样处理可使函数很简单。不管怎样，它仅被fork()使用。
//注意2: 关于复制内核空间
//复制指定线性地址和长度内存对应的页目录项和页表项，从而被复制的页目录和页表对应的原物理内存
//页面区被两套页表映射而共享使用。复制时，需申请新页面来存放新页表，原物理内存内存区将被共享。
//此后两个进程（父进程和其子进程）将共享内存区，直到有一个进程执行写操作时，内核才会为写操作
//进程分配新的内存也（写时复制机制）
//参数：from、to - 线性地址， size - 需要复制的内存长度（单位：字节），
int 
copy_page_tables(unsigned long from,unsigned long to,long size)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long nr;

	if ((from&0x3fffff) || (to&0x3fffff))
		panic("copy_page_tables called with wrong alignment");
	from_dir = (unsigned long *) (((from>>20) & 0xffc) + PAGE_DIR_ADDR); 
	to_dir = (unsigned long *) (((to>>20) & 0xffc) + PAGE_DIR_ADDR);
	size = ((unsigned) (size+0x3fffff)) >> 22;
	for( ; size-->0 ; from_dir++,to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exist, to = 0x%x, size = 0x%x", to, size);
		if (!(1 & *from_dir))
			continue;
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
		if (!(to_page_table = (unsigned long *) get_free_page())){
			oom();       //add by jjh
			return -1;	/* Out of memory, see freeing */
		}
		*to_dir = ((unsigned long) to_page_table) | 7;
		nr = (from==0)?0xA0:1024;
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
			this_page = *from_page_table;
			if (!(1 & this_page))
				continue;
			this_page &= ~2;
			*to_page_table = this_page;
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;
				this_page -= LOW_MEM;
				this_page >>= 12;
				mem_map[this_page]++;
			}
		}
	}
	invalidate();
	return 0;
}

////取消写保护页面函数。用于异常中断过程中写保护异常的处理（写时复制）
static void 
un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page,new_page;

	old_page = 0xfffff000 & *table_entry;
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {
		*table_entry |= 2;
		invalidate();
		return;
	}
	if (!(new_page=get_free_page()))
		oom();
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	*table_entry = new_page | 7;
	invalidate();
	copy_page(old_page,new_page);
}	

////拷贝一页内存
//参数：from_paddr - 拷贝源物理地址，to_paddr -- 拷贝目的物理地址
static void
copy_page(unsigned long from_paddr, unsigned long to_paddr)
{
	unsigned long *from = (unsigned long *)from_paddr, *to = (unsigned long *)to_paddr;
	int i;
	for(i = 0; i < 0x1000 / sizeof(unsigned long); i++){
		*(from++) = *(to++);
	}
}

////执行写保护页面处理，当发生Page Fault异常，并且因为有页写保护引起，就会调用该函数
static void 
do_wp_page(unsigned long error_code,unsigned long address)
{
#if 0
	/* we cannot do this yet: the estdio library writes to code space */
	/* stupid, stupid. I really want the libc.a from GNU */
	//if (CODE_SPACE(address))
	//	do_exit(SIGSEGV);
#endif
	unsigned long * table_entry = (unsigned long *)
								  (((address>>10) & 0xffc) + (0xfffff000 &
										  *((unsigned long *) (((address>>20) &0xffc) + PAGE_DIR_ADDR) ))); //页表项地址
	un_wp_page(table_entry);

}

/*------------------------------------------辅助函数-----------------------------------------------*/
int get_count_of_free_pages(){
	int i;
	int count_of_free_pages = 0;
	char *mem_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < get_count_of_total_pages(); i++){
		if(mem_map[i] == 0)
			count_of_free_pages++;
	}
	return count_of_free_pages;
}

int get_count_of_total_pages(){
	return memtotal / 0x1000;
}

int get_count_of_used_pages(){
	int i;
	int count_of_pages = 0;
	char *mem_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < get_count_of_total_pages(); i++){
		if(mem_map[i] == 1)
			count_of_pages++;
	}
	return count_of_pages;
}

void print_page_config()
{
	debug("total_pages = %d", get_count_of_total_pages());
	debug("free_pages = %d",get_count_of_free_pages());
	debug("used_pages = %d",get_count_of_used_pages());
	
	debug("cr3 = 0x%08.8x",get_cr3());
	debug("cr0 = 0x%08.8x",get_cr0());
	
	debug("NO_PAGE_EXP_COUNT = %d, error_code = %d, address = %d(0x%x)",NO_PAGE_EXP_COUNT, (u32)_error_code, _address, _address);
}

static void oom(){
	panic("out of memory!");
}



