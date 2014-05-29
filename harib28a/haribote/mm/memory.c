#include "bootpack.h"
#include "memory.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000


static void prepare_for_open_page();
//static void map_kernel_page(unsigned int addr_start, int page_count);
static void get_empty_page(unsigned int addr_start);
static void oom();
void print_page_tables();

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

int NO_PAGE_EXP_COUNT = 0;
unsigned long _error_code = 0,_address = 0;
/***********************************分页相关**************************************************/
/*  处理Page Fault */
void do_no_page(unsigned long error_code, unsigned long address) 
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
		get_empty_page(address);
	debug("-----------------------------------------");
}

//初始化内存, 测试内存大小，将内存划分成物理页, 分配和映射内核使用的物理页
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
	
	//清空1024个页目录项
	for(i=0; i<1024; i++)
		page_dir_base_addr[i] = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < 0x04000000 / 0x1000; i++)
		page_bit_map[i] = 0;
	
	/*  低端的线性地址只有内核会用到的，对于内核用到的地址，线性地址=物理地址 */
	for(i=0; i<16; i++) 
		page_dir_base_addr[i] = (((int)page_dir_base_addr + 4096 * (i+1)) & 0xFFFFFC00) | 0x7;
	page_dir_base_addr[0xe0000000/0x400000] = (( (int)page_dir_base_addr + 4096 * 17) & 0xFFFFFC00) | 0x7;
	int *page_table = (int *)PAGE_DIR_ADDR + 1024;
	/* 映射物理内存 */
	for(laddr = 0, i= 0; laddr < 0x04000000; laddr += 0x1000, i++)
		page_table[i] =  laddr | 0x7;
	/* 映射显存 */
	for(laddr = 0xe0000000; laddr < 0xe0000000 + 192 * 0x1000; laddr += 0x1000, i++) //从3G开始
		page_table[i] = laddr | 0x7;
	
	/* 设置已经使用的内存(内核的代码，数据＋供内核分配的存储空间) */
	for( i = 0; i < (0x00b00000 + 0x01000000)/ 0x1000; i++ )
		page_bit_map[i] = 1;
	
	//页表最后一项指向它自己
	page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

////分配一页物理页
static unsigned int 
get_free_page()
{
	int i;
	int has_free_page = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for( i = 0; i < get_count_of_total_pages(); i++ ){
		if(page_bit_map[i] == 0){
			has_free_page = 1;
			break;
		}
	}
	if( has_free_page ){
		page_bit_map[i] = 1;
		return i * 4 * 1024;
	}
	else
		return NO_FREE_PAGE_ADDR;
}

////将物理内存映射到线性地址上
static u32 
put_page(unsigned int page, unsigned int address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */

	//if (page < LOW_MEM || page >= HIGH_MEMORY)
	//	printk("Trying to put page %p at %p\n",page,address);
	//if (mem_map[(page-LOW_MEM)>>12] != 1)
	//	printk("mem_map disagrees with %p at %p\n",page,address);
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


static void get_empty_page(unsigned int addr_start)
{
	u32 page;
	if (  (page=get_free_page()) == NO_FREE_PAGE_ADDR
	   || !put_page(page,addr_start) ){
		oom();
	}
}

/*------------------------------------------辅助函数-----------------------------------------------*/
int get_count_of_free_pages(){
	int i;
	int count_of_free_pages = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < get_count_of_total_pages(); i++){
		if(page_bit_map[i] == 0)
			count_of_free_pages++;
	}
	return count_of_free_pages;
}

int get_count_of_total_pages(){
	return memtotal / (4 * 1024);
}

int get_count_of_used_pages(){
	int i;
	int count_of_pages = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < get_count_of_total_pages(); i++){
		if(page_bit_map[i] == 1)
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
	
	debug("NO_PAGE_EXP_COUNT = %d, error_code = %d, address = %d(0x%x)",NO_PAGE_EXP_COUNT, _error_code, _address, _address);
}

static void oom(){
	panic("out of memory!");
}



