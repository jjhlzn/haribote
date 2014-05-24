#include "bootpack.h"
#include "memory.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000


static void prepare_page_dir_and_page_table();
static void map_kernel_page(unsigned int addr_start, int page_count);
static void map_user_page(unsigned int addr_start);
static void oom();
void print_page_tables();

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
	if( p == 0 )
		map_user_page(address);
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
	//0x00a00000-0x00b00000为高速缓冲
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

/****准备页目录和页表***
 ***先把线性地址映射成物理地址
 ***将内核所在的线性地址映射到物理地址
*/
static void prepare_page_dir_and_page_table()
{
	int i;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	//清空1024个页目录项
	for(i=0; i<1024; i++){
		page_dir_base_addr[i] = 0;
	}
	
	unsigned int kernel_pages = 0x00b10000 / 0x1000;
	//unsigned int kernel_pages = 0xFFe00000 / 0x1000;
	end_addr_mapped = kernel_pages * 4 * 1024 -1;
	map_kernel_page(0x00000000, kernel_pages);  //映射内核空间
    int vram_pages = 4 * 1024 * 1024 / 0x1000;
	map_kernel_page(0xe0000000, vram_pages);  //映射显存空间
	
	//页表最后一项指向它自己
	page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

/** 分配一页物理页  */
static unsigned int alloc_free_page()
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
		//debug("alloc a new page, addr = 0x%08.8x", i * 4 * 1024);
		return i * 4 * 1024;
	}
	else
		return NO_FREE_PAGE_ADDR;
}

static void map_kernel_page(unsigned int addr_start, int page_count)
{
	int i, j, index = 0;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	
	unsigned int dir_start_index = addr_start >> 22; //页目录开始项
	int dir_count = (page_count + 1023) / 1024 ; //页目录项个数
	
	//占用物理页
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for( i = dir_start_index * 1024; i < dir_start_index * 1024 + page_count; i++)
		page_bit_map[i] = 1;
	
	//设置页目录项和页表项
	for( i = dir_start_index; i < dir_start_index + dir_count; i++ ){
		int *the_page_table = page_dir_base_addr + (i + 1) * 1024;
		page_dir_base_addr[i] = ((int)the_page_table & 0xFFFFFC00) | 0x7; //设置页目录项的页表物理地址
		int base_addr =  i * 4 * 1024 * 1024;
		for(j = 0; j < 1024; j++){
			if(index >= page_count) //内核页已经映射完毕
				break;
			the_page_table[j] = ( (base_addr + j * 4 * 1024) & 0xFFFFFC00 ) | 0x7;
			index++;
		}
	}
}

////获取该线性地址的页表地址
static  int get_page_table_laddr(unsigned int addr_start)
{
	int offset = (addr_start >> 12 & 0x3FF) * 4;           //页表的线性地址的最低12位
	int page_offset = (addr_start >> 22) << 12;            //页表的线性地址的中间10位
	int page_dir_offset = 0x3FF << 22;
	//debug("pdir_offset=%d,page_offset=%d, offset=%d", (u32)page_dir_offset, page_offset, offset);
	return (page_dir_offset + page_offset + offset); //页表的线性地址
}



////打印页表映射 laddr->paddr
void print_page_tables()
{
	u32 laddr = 0;
	for(laddr=0; laddr < 0x00d10000; laddr += 0x1000){
		
		int page_table_laddr = get_page_table_laddr(laddr);
		if( *(int *)page_table_laddr & 0x01 ){
			int page_base_paddr = *(int *)page_table_laddr & 0xFFFFFFF000;
			if(laddr != (u32)page_base_paddr)
				debug("%08.8x --> %08.8x",laddr,(u32)page_base_paddr);
			//debug("%08.8x --> %08.8x",laddr,(u32)page_base_paddr);
		}else{
			//debug("%08.8x --> ",laddr);
		}
	}
}


static void map_user_page(unsigned int addr_start)
{
	//debug("laddr = 0x%08.8x",addr_start); 
	//这里应该使用页表的线性地址，而不应该是物理地址。不过对于PAGE_DIR_ADDR线性地址刚好是物理地址。
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;  
	unsigned int dir_index = addr_start >> 22; //页目录开始项
	
	int *pagetable_item = NULL;
	if(page_dir_base_addr[dir_index] == 0){  //页表没有分配
		unsigned int page = alloc_free_page();
		if(page == NO_FREE_PAGE_ADDR)
			oom();
		page_dir_base_addr[dir_index] = ( page & 0xFFFFFC00 ) | 0x7; //设置页目录项的页表物理地址
		debug("paddr of page table = 0x%08.8x", page);
		debug("allocate page dir");
	}
	//现在需要修改页表
	pagetable_item = (int *)get_page_table_laddr(addr_start); //页表的线性地址
	debug("laddr of page table item = 0x%08.8x", (int)pagetable_item);
		
	unsigned int page = alloc_free_page();
	if(page == NO_FREE_PAGE_ADDR)
		oom();
	//page -= 0x2000;
	*pagetable_item =  page | 0x7;
	
	debug("map ladd[%08.8x] to paddr[%08.8x]",addr_start,page);
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



