/* 儊儌儕娭學 */

#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/**
  用于计算计算机内存大小。方法：通过不断的测试来获取计算机内存大小。
*/
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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}


/***********************************分页相关**************************************************/

/****准备页目录和页表***
 ***先把线性地址映射成物理地址
*/
int PAGE_DIR_ADDR = 0x00400000; 
void prepare_page_dir_and_page_table()
{
	
	
	//int mem_size = 32 * 1024 * 1024; //写死成32M
	int i, j;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	//清空1024个页目录项
	for(i=0; i<1024; i++){
		page_dir_base_addr[i] = 0;
	}
	
	//一个页表可以维护1024个页表(4M)，16M内存只需要4个页表
	
	//设置页表
	for(i = 0; i < 1024; i++){
		int *the_page_table = page_dir_base_addr + (i + 1) * 1024;
		page_dir_base_addr[i] = ((int)the_page_table & 0xFFFFFC00) | 0x3;
		int base_addr = 0x00000000 + i * 4 * 1024 * 1024;
		for(j=0; j < 1024; j++){
			the_page_table[j] = ( (base_addr + j * 4 * 1024) & 0xFFFFFC00 ) | 0x3;
		}
	}
}

int test_page(unsigned x)
{
	int *page_dir_base_addr = PAGE_DIR_ADDR;
	int page_dir_index = x >> 22;
	debug("page_dir_index = %d", page_dir_index);
	unsigned int page_add =  *(page_dir_base_addr + page_dir_index) & 0xFFFFF000;
	debug("page_add = %8.8x", page_add);
	int page_index = x >> 12 & 0x000003FF;
	debug("page_index = %d", page_index);
	int *add = (*((int *)page_add + page_index)  & 0xFFFFF000) + (x & 0x00000FFF);
	debug("Page(0x%08.8x) = 0x%08.8x",x,(int) add);
	return (int) add;
}

void print_page_config()
{
	int i, j;
	int *page_dir_base_addr = PAGE_DIR_ADDR;
	//清空1024个页目录项
	for(i=0; i<10; i++){
		debug("0x%08.8x",page_dir_base_addr[i]);
	}
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
	
	debug("cr3 = 0x%08.8x",get_cr3());
	debug("cr0 = 0x%08.8x",get_cr0());
}


