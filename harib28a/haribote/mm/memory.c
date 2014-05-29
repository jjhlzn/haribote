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
  ���ڼ��������ڴ��С��������ͨ�����ϵĲ�������ȡ������ڴ��С��
*/
int memtotal;
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
unsigned long _error_code = 0,_address = 0;
/***********************************��ҳ���**************************************************/
/*  ����Page Fault */
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
	//0x00a00000-0x00b00000Ϊ���ٻ��� (1MB)
	memman_free(memman, 0x00b00000, 0x00b00000 + 16 * 1024 * 1024); /* 16M���ں˷��� */ 
	
	prepare_for_open_page();
	open_page();
}

////��ҳǰ�ĳ�ʼ�����ܣ�׼��ҳĿ¼��ҳ��
static void 
prepare_for_open_page()
{
	int i;
	u32 laddr;
	int *page_dir_base_addr = (int *)PAGE_DIR_ADDR;
	
	//���1024��ҳĿ¼��
	for(i=0; i<1024; i++)
		page_dir_base_addr[i] = 0;
	char *page_bit_map = (char *)PAGE_BIT_MAP_ADDR;
	for(i = 0; i < 0x04000000 / 0x1000; i++)
		page_bit_map[i] = 0;
	
	/*  �Ͷ˵����Ե�ַֻ���ں˻��õ��ģ������ں��õ��ĵ�ַ�����Ե�ַ=�����ַ */
	for(i=0; i<16; i++) 
		page_dir_base_addr[i] = (((int)page_dir_base_addr + 4096 * (i+1)) & 0xFFFFFC00) | 0x7;
	page_dir_base_addr[0xe0000000/0x400000] = (( (int)page_dir_base_addr + 4096 * 17) & 0xFFFFFC00) | 0x7;
	int *page_table = (int *)PAGE_DIR_ADDR + 1024;
	/* ӳ�������ڴ� */
	for(laddr = 0, i= 0; laddr < 0x04000000; laddr += 0x1000, i++)
		page_table[i] =  laddr | 0x7;
	/* ӳ���Դ� */
	for(laddr = 0xe0000000; laddr < 0xe0000000 + 192 * 0x1000; laddr += 0x1000, i++) //��3G��ʼ
		page_table[i] = laddr | 0x7;
	
	/* �����Ѿ�ʹ�õ��ڴ�(�ں˵Ĵ��룬���ݣ����ں˷���Ĵ洢�ռ�) */
	for( i = 0; i < (0x00b00000 + 0x01000000)/ 0x1000; i++ )
		page_bit_map[i] = 1;
	
	//ҳ�����һ��ָ�����Լ�
	page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

////����һҳ����ҳ
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

////�������ڴ�ӳ�䵽���Ե�ַ��
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



