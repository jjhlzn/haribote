/* 儊儌儕娭學 */

#include "bootpack.h"
#include "memory.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000


static void prepare_page_dir_and_page_table();
static void map_kernel(unsigned int addr_start, int page_count);

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
	//open_page();
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
	
	
	unsigned int kernel_pages = 0x00e00000 / 0x1000;
	//unsigned int kernel_pages = 0xFFe00000 / 0x1000;
	end_addr_mapped = kernel_pages * 4 * 1024 -1;
	map_kernel(0x00000000, kernel_pages);  //映射内核空间
    int vram_pages = 4 * 1024 * 1024 / 0x1000;
	map_kernel(0xe0000000, vram_pages);  //映射显存空间
	
	//页表最后一项指向它自己
	page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}


static void map_kernel(unsigned int addr_start, int page_count)
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

/**   分配一页物理页  */
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

static void oom(){
	panic("out of memory!");
}


static void map_user(unsigned int addr_start)
{
	//debug("laddr = 0x%08.8x",addr_start); 
	//这里应该使用页表的线性地址，而不应该是物理地址。不过对于PAGE_DIR_ADDR线性地址刚好是物理地址。
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;  
	unsigned int dir_index = addr_start >> 22; //页目录开始项
	
	//debug("page_dir_base_addr[%d] = 0x%08.8x", dir_index, page_dir_base_addr[dir_index]);
	int *pagetable_item = NULL;
	if(page_dir_base_addr[dir_index] == 0){  //页表没有分配
		unsigned int page = alloc_free_page();
		if(page == NO_FREE_PAGE_ADDR)
			oom();
		page_dir_base_addr[dir_index] = ( page & 0xFFFFFC00 ) | 0x7; //设置页目录项的页表物理地址
		//debug("paddr of page table = 0x%08.8x", page);
		debug("allocate page dir");
	}
	//现在需要修改页表
	int offset = (addr_start >> 12 & 0x3FF) * 4;   //页表的线性地址的最低12位
	int page_offset = (addr_start >> 22) << 12;            //页表的线性地址的中间10位
	int page_dir_offset = 0x3FF << 22;
	//debug("page_dir_offset = %d, page_offset = %d, offset = %d", page_dir_offset, page_offset, offset);
	pagetable_item = (int *)(page_dir_offset + page_offset + offset); //页表的线性地址
	//debug("laddr of page table item = 0x%08.8x", (int)pagetable_item);
		
	unsigned int page = alloc_free_page();
	if(page == NO_FREE_PAGE_ADDR)
		oom();
	*pagetable_item =  page | 0x7;
}

/*  处理Page Fault */
void do_no_page(unsigned long error_code, unsigned long address) 
{
     //debug("-----do_no_page----: error_code = %d, address = %d",error_code, address);
	//unsigned int laddr = address & 0xFFC00000;
	//map_kernel(laddr, 1024);
	map_user(address);
}







//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
	//prepare_page_dir_and_page_table();
	//int i, j;
	//int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	//清空1024个页目录项
	//for(i=0; i<10; i++){
	//	debug("0x%08.8x",page_dir_base_addr[i]);
	//}
	//设置页表
	//for(i = 0; i < 4; i++){
	//	int *the_page_table = page_dir_base_addr + (i + 1) * 1024;
	//	for(j=0; j < 1024; j++){
	//		debug("0x%08.8x",the_page_table[j]);
	//	}
	//}
	//for(i = 0; i<16 * 1024 * 1024 ; i++){
	//	int result = test_page(i);
	//	if(result != i)
	//		debug("find fault");
	//}
	//test_page(0x00600010);
	//test_page(0x00000000);
	//test_page(0x00001000);
	//test_page(0x00400000);
	//test_page(16 * 1024 * 1024-1);
	//test_page(4 * 1024 * 1024 * 1024-1);
	test_page(0xe0000000);
	test_page(0xe0001000);
	
	//debug("mapped addr: 0x%08.8x -- 0x%08.8x", start_addr_mapped, end_addr_mapped);
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



