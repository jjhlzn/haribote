#include "bootpack.h"
#include "memory.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000


static void prepare_page_dir_and_page_table();
static void get_empty_page(unsigned int addr_start);
static void oom();
void print_page_tables();
unsigned int get_and_map_cont_free_page(u32 addr, int n);
static unsigned int get_cont_free_pages(int n);
static u32 put_page(unsigned int page, unsigned int addr_start);

/**
  用于计算计算机内存大小。方法：通过不断的测试来获取计算机内存大小。
*/
int memtotal;
/**  addr_start必须4M对其 */
int start_addr_mapped = 0;
int end_addr_mapped = 0;
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
{
	return memman_alloc_4k(man,size);
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放内存 */
{
	return 0;
}

////分配连续页数的物理内存
//参数：man - 无用， size - 需要内存的字节数
unsigned int 
memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	man = (struct MEMMAN *)NULL; //unuserd code
	unsigned int page;
	size = (size + 0xfff) & 0xfffff000;
	page = get_cont_free_pages(size/4096);
	return page;
}

int 
memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	//int i;
	//size = (size + 0xfff) & 0xfffff000;
	//i = memman_free(man, addr, size);
	//return i;
	return 0;
}

int NO_PAGE_EXP_COUNT = 0;
/***********************************分页相关**************************************************/
/*  处理Page Fault */
void do_no_page(unsigned long error_code, unsigned long address) 
{
	NO_PAGE_EXP_COUNT++;
	int p = error_code & 0x01;
	int w_r = (error_code & 0x02) >> 1;
	int u_s = (error_code & 0x04) >> 2;
    debug("do_no_page(%d): code=%d, addr=%d(0x%08.8x)",NO_PAGE_EXP_COUNT,error_code, address,address);
	debug("P = %d, W/R = %d, U/S = %d",p,w_r,u_s);
	//unsigned int laddr = address & 0xFFC00000;
	//map_kernel(laddr, 1024);
	if( !p ){
		get_empty_page(address);
	}
	debug("-----------------------------------------");
	//while(1);
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
	memman_free(memman, 0x00b00000, memtotal - 0x00b00000); 
	
	int mem_pages = memtotal / (4 * 1024);
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	int i;
	for(i = 0; i < mem_pages; i++){
		page_bit_map[i] = 0;
	}
	prepare_page_dir_and_page_table();
	open_page();
}

////准备页目录和页表
static void 
prepare_page_dir_and_page_table()
{
	int i;
	u32 laddr;
	
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	
	//清空1024个页目录项
	for(i=0; i<1024; i++)
		page_dir_base_addr[i] = 0;
		
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < 64 * 1024 * 1024 / 0x1000; i++)
		page_bit_map[i] = 0;
	
	/*  低端的线性地址只有内核会用到的，对于内核用到的地址，线性地址=物理地址 */
	for(i=0; i<16; i++) 
		page_dir_base_addr[i] = (((int)page_dir_base_addr + 4096 * (i+1)) & 0xFFFFFC00) | 0x7;
	page_dir_base_addr[0xe0000000/0x400000] = (( (int)page_dir_base_addr + 4096 * (16+1)) & 0xFFFFFC00) | 0x7;
	int *page_table = (int *)PAGE_DIR_ADDR + 1024;
	for(laddr = 0, i= 0; laddr < 0x4000000; laddr += 0x1000, i++)
		page_table[i] = (laddr & 0xFFFFFC00) | 0x7;
	/* 显存 */
	for(laddr = 0xe0000000; laddr < 0xe0000000 + 768 * 0x1000; laddr += 0x1000) //从3G开始
		page_table[i] = (laddr & 0xFFFFFC00) | 0x7;
	
	/* 设置已经使用的内存 */
	for( i = 0; i < 0x00b00000 / 0x1000; i++ ){
		page_bit_map[i] = 1;
	}
}

/** 分配一页物理页  */
static unsigned int get_free_page()
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
		return 0;
}

////分配物理上连续的n页内存
//参数：内存页数
static unsigned int get_cont_free_pages(int n)
{
	if (!n)
		debug("WARN: alloc 0 pages");
	int i, cont_pages =0, start_index = 0, has_free_pages = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for( i = 0; i < get_count_of_total_pages(); i++ ){
		if(!page_bit_map[i]){
			if(has_free_pages){
				cont_pages++;
			}else{
				has_free_pages = 1;
				cont_pages = 1;
				start_index = i;
			}
			if( n == cont_pages )
				break;
		}else{
			has_free_pages = 0;
		}
	}
	if( has_free_pages ){
		for(i=start_index; i<n; i++)
			page_bit_map[i] = 1;
		return start_index * 4 * 1024;
	}
	else
		return 0;
}

////分配n页连续物理内存，并把它映射到线性地址
//参数：addr -- 线性地址， n --  需要分配的内存页数
//返回：物理地址
unsigned int get_and_map_cont_free_page(u32 addr, int n)
{
	u32 page = get_cont_free_pages(n);
	if(!page)
		oom();
	int i;
	for(i = 0; i<n; i++)
		put_page(page + i * 0x1000,addr + i * 0x1000);
	return page;
}

static u32 put_page(unsigned int page, unsigned int address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */

	//if (page < LOW_MEM || page >= HIGH_MEMORY)
	//	printk("Trying to put page %p at %p\n",page,address);
	//if (mem_map[(page-LOW_MEM)>>12] != 1)
	//	printk("mem_map disagrees with %p at %p\n",page,address);
	page_table = (unsigned long *) ((address>>20) & 0xffc);
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

int test_page(unsigned x)
{
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	int page_dir_index = x >> 22;
	debug("page_dir_index = %d", page_dir_index);
	unsigned int page_add =  *(page_dir_base_addr + page_dir_index) & 0xFFFFF000;
	debug("page_add = %8.8x", page_add);
	int page_index = x >> 12 & 0x000003FF;
	debug("page_index = %d", page_index);
	int *add = (int *)((*((int *)page_add + page_index)  & 0xFFFFF000) + (x & 0x00000FFF));
	debug("Page(0x%08.8x) = 0x%08.8x",x,(int) add);
	return (int) add;
}

void print_page_config()
{
	test_page(0xe0000000);
	test_page(0xe0001000);

	debug("mapped addr: %d -- %d", start_addr_mapped, end_addr_mapped);
	
	debug("total_pages = %d", get_count_of_total_pages());
	debug("free_pages = %d",get_count_of_free_pages());
	debug("used_pages = %d",get_count_of_used_pages());
	
	debug("cr3 = 0x%08.8x",get_cr3());
	debug("cr0 = 0x%08.8x",get_cr0());
	
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	int cyls = binfo->cyls;
	debug("cyls = %d",cyls);
}

static void oom(){
	panic("out of memory!");
}



