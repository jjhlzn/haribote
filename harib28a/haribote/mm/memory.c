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
  ���ڼ��������ڴ��С��������ͨ�����ϵĲ�������ȡ������ڴ��С��
*/
int memtotal;
/**  addr_start����4M���� */
int start_addr_mapped = 0;
int end_addr_mapped = 0;
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386���A486�ȍ~�Ȃ̂��̊m�F */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386�ł�AC=1�ɂ��Ă�������0�ɖ߂��Ă��܂� */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* �L���b�V���֎~ */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* �L���b�V������ */
		store_cr0(cr0);
	}

	return i;
}


void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* ������Ϣ��Ŀ */
	man->maxfrees = 0;		/* ���ڹ۲����״����frees�����ֵ */
	man->lostsize = 0;		/* �ͷ�ʧ�ܵ��ڴ��С�ܺ� */
	man->losts = 0;			/* �ͷ�ʧ�ܴ��� */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* ��������ڴ��С�ĺϼ� */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* �ڴ���� */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* �ҵ����㹻����ڴ� */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* ���free[i]�����0���ͼ���һ��������Ϣ */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; 
				}
			}
			return a;
		}
	}
	return 0; /* û�п��ÿռ� */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* �ͷ��ڴ� */
{
	int i, j;
	 /* Ϊ���ڹ����ڴ棬��free[]����addr��˳������ */    
	/* ���ԣ��Ⱦ���Ӧ�÷������� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* ǰ���п����ڴ� */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* ����Ԥǰ��Ŀ����ڴ���ɵ�һ�� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* ����Ҳ�� */
				if (addr + size == man->free[i].addr) {
					/* Ҳ���������Ŀ����ڴ���ɵ�һ�� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]ɾ��*/
					/* free[i]���0����ɵ�ǰ��ȥ */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* �ṹ�帳ֵ */
					}
				}
			}
			return 0; /* �ɹ���� */
		}
	}
	/* ������ǰ����ɵ�һ��*/
	if (i < man->frees) {
		/* ���滹��*/
		if (addr + size == man->free[i].addr) {
			/* �������������ݹ��ɵ�һ�� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �ɹ���� */
		}
	}
	/* ������ǰ����ɵ�һ��Ҳ�����������ɵ�һ�� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]֮��ģ�����ƶ����ڳ�һ����ÿռ� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* �������ֵ */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �ɹ���� */
	}
	/* ���������ƶ� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ʧ�� */
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
/***********************************��ҳ���**************************************************/
/*  ����Page Fault */
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

//��ʼ���ڴ�, �����ڴ��С�����ڴ滮�ֳ�����ҳ, �����ӳ���ں�ʹ�õ�����ҳ
void 
mem_init()
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//����ڴ�
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	//0x00400000-0x00900000��ҳ��0x00900000-0x00a00000��ŷ�ҳ�Ƿ����
	//0x00a00000-0x00b00000Ϊ���ٻ���
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

/****׼��ҳĿ¼��ҳ��***
 ***�Ȱ����Ե�ַӳ��������ַ
 ***���ں����ڵ����Ե�ַӳ�䵽�����ַ
*/
static void prepare_page_dir_and_page_table()
{
	int i;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	//���1024��ҳĿ¼��
	for(i=0; i<1024; i++){
		page_dir_base_addr[i] = 0;
	}
	
	unsigned int kernel_pages = 0x00b10000 / 0x1000;
	//unsigned int kernel_pages = 0xFFe00000 / 0x1000;
	end_addr_mapped = kernel_pages * 4 * 1024 -1;
	map_kernel_page(0x00000000, kernel_pages);  //ӳ���ں˿ռ�
    int vram_pages = 4 * 1024 * 1024 / 0x1000;
	map_kernel_page(0xe0000000, vram_pages);  //ӳ���Դ�ռ�
	
	//ҳ�����һ��ָ�����Լ�
	page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

/** ����һҳ����ҳ  */
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
	
	unsigned int dir_start_index = addr_start >> 22; //ҳĿ¼��ʼ��
	int dir_count = (page_count + 1023) / 1024 ; //ҳĿ¼�����
	
	//ռ������ҳ
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for( i = dir_start_index * 1024; i < dir_start_index * 1024 + page_count; i++)
		page_bit_map[i] = 1;
	
	//����ҳĿ¼���ҳ����
	for( i = dir_start_index; i < dir_start_index + dir_count; i++ ){
		int *the_page_table = page_dir_base_addr + (i + 1) * 1024;
		page_dir_base_addr[i] = ((int)the_page_table & 0xFFFFFC00) | 0x7; //����ҳĿ¼���ҳ�������ַ
		int base_addr =  i * 4 * 1024 * 1024;
		for(j = 0; j < 1024; j++){
			if(index >= page_count) //�ں�ҳ�Ѿ�ӳ�����
				break;
			the_page_table[j] = ( (base_addr + j * 4 * 1024) & 0xFFFFFC00 ) | 0x7;
			index++;
		}
	}
}

////��ȡ�����Ե�ַ��ҳ���ַ
static  int get_page_table_laddr(unsigned int addr_start)
{
	int offset = (addr_start >> 12 & 0x3FF) * 4;           //ҳ������Ե�ַ�����12λ
	int page_offset = (addr_start >> 22) << 12;            //ҳ������Ե�ַ���м�10λ
	int page_dir_offset = 0x3FF << 22;
	//debug("pdir_offset=%d,page_offset=%d, offset=%d", (u32)page_dir_offset, page_offset, offset);
	return (page_dir_offset + page_offset + offset); //ҳ������Ե�ַ
}



////��ӡҳ��ӳ�� laddr->paddr
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
	//����Ӧ��ʹ��ҳ������Ե�ַ������Ӧ���������ַ����������PAGE_DIR_ADDR���Ե�ַ�պ��������ַ��
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;  
	unsigned int dir_index = addr_start >> 22; //ҳĿ¼��ʼ��
	
	int *pagetable_item = NULL;
	if(page_dir_base_addr[dir_index] == 0){  //ҳ��û�з���
		unsigned int page = alloc_free_page();
		if(page == NO_FREE_PAGE_ADDR)
			oom();
		page_dir_base_addr[dir_index] = ( page & 0xFFFFFC00 ) | 0x7; //����ҳĿ¼���ҳ�������ַ
		debug("paddr of page table = 0x%08.8x", page);
		debug("allocate page dir");
	}
	//������Ҫ�޸�ҳ��
	pagetable_item = (int *)get_page_table_laddr(addr_start); //ҳ������Ե�ַ
	debug("laddr of page table item = 0x%08.8x", (int)pagetable_item);
		
	unsigned int page = alloc_free_page();
	if(page == NO_FREE_PAGE_ADDR)
		oom();
	//page -= 0x2000;
	*pagetable_item =  page | 0x7;
	
	debug("map ladd[%08.8x] to paddr[%08.8x]",addr_start,page);
}

/*------------------------------------------��������-----------------------------------------------*/
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



