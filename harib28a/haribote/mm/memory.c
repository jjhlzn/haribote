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

////ˢ��ҳ�任���ٻ���꺯��
//Ϊ����ߵ�ַת����Ч�ʣ�CPU�����ʹ�õ�ҳ�����ݴ����оƬ�и��ٻ�����(TLB)�����޸Ĺ�ҳ����Ϣ��
//����Ҫ����ˢ�¸û�����������ʹ�����¼���ҳĿ¼��ַ�Ĵ���CR3�ķ���������ˢ�¡�����eax = PAGE_DIR_ADDR����ҳĿ¼�ĵ�ַ
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (PAGE_DIR_ADDR))

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


/***********************************��ҳ���**************************************************/
int NO_PAGE_EXP_COUNT = 0;
unsigned long _error_code = 0, _address = 0;
/*  ����Page Fault */
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

////��ʼ���ڴ�, �����ڴ��С�����ڴ滮�ֳ�����ҳ, �����ӳ���ں�ʹ�õ�����ҳ
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
	
	/* ��ʼ������ҳ״̬ */
	for(i = 0; i < TOTAL_MEM / 0x1000; i++)
		mem_map[i] = 0;
	
	/* ��ʼ��1024��ҳĿ¼�� */
	for(i=0; i<1024; i++)
		page_dir_base_addr[i] = 0;
	
	/*  �Ͷ˵����Ե�ַֻ���ں˻��õ��ģ������ں��õ��ĵ�ַ�����Ե�ַ=�����ַ */
	for(i=0; i<16; i++) 
		page_dir_base_addr[i] = (((int)page_dir_base_addr + 4096 * (i+1)) & 0xFFFFFC00) | 0x7;
	page_dir_base_addr[0xe0000000/0x400000] = (( (int)page_dir_base_addr + 4096 * 17) & 0xFFFFFC00) | 0x7;
	int *page_table = (int *)PAGE_DIR_ADDR + 1024;
	/* ӳ�������ڴ� */
	for(laddr = 0, i= 0; laddr < TOTAL_MEM; laddr += 0x1000, i++)
		page_table[i] =  laddr | 0x7;
	/* ӳ���Դ� */
	for(laddr = 0xe0000000; laddr < 0xe0000000 + 192 * 0x1000; laddr += 0x1000, i++) //��3G��ʼ
		page_table[i] = laddr | 0x7;
	
	/* �����Ѿ�ʹ�õ��ڴ�(�ں˵Ĵ��룬���ݣ����ں˷���Ĵ洢�ռ�) */
	for( i = 0; i < LOW_MEM/ 0x1000; i++ )
		mem_map[i] = USED;
	
	//ҳ�����һ��ָ�����Լ�
	//page_dir_base_addr[1023] = PAGE_DIR_ADDR | 0x7;
}

////����һҳ����ҳ
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

////����������������nҳ�ڴ�,����4M����
//������n -- �ֽ���
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

////����nҳ���������ڴ棬������ӳ�䵽���Ե�ַ
//������addr -- ���Ե�ַ�� n --  ��Ҫ������ڴ�ҳ��
//���أ������ַ
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

////�������ڴ�ӳ�䵽���Ե�ַ��
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

////����һҳ�����ڴ棬���Ұ���ҳ�ڴ�ӳ�䵽���Ե�ַladdr��
//������laddr -- ���Ե�ַ
static void 
get_empty_page(unsigned int laddr)
{
	u32 page;
	if (  (page=get_free_page()) == NO_FREE_PAGE_ADDR
	   || !put_page(page,laddr) ){
		oom();
	}
}

////�ͷ������ַΪaddr����һ���ڴ棬���������free_page_tableʹ��
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

//// ����ָ�������Ե�ַ���޳���ҳ����������ͷŶ�Ӧ�ڴ�ҳ��ָ�����ڴ�鲢���ñ�����С�
//������from -- ��ʼ���Ի���ַ�� size -- �ͷŵ��ֽڳ���
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
	dir = (unsigned long *) (( (from>>20)  & 0xffc) + PAGE_DIR_ADDR); //ҳĿ¼���ַ
	debug("dir = 0x%x, size = %d",dir,size);
	for ( ; size-->0 ; dir++) {
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *) (0xfffff000 & *dir); //ҳ���ַ(�����ַ��
		for (nr=0 ; nr<1024 ; nr++) {
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table); //�ͷŸ�ҳ
			*pg_table = 0;
			pg_table++;
		}
		free_page(0xfffff000 & *dir);
		*dir = 0;
	}
	invalidate();
	return 0;
}

////����ҳĿ¼��ҳ����
//ע��1�����ﲢ�������κ��ڴ�� - �ڴ��ĵ�ַ��Ҫ��4Mb�ı���������һ��ҳĿ¼�����Ӧ���ڴ�
//���ȣ�����Ϊ���������ʹ�����ܼ򵥡�����������������fork()ʹ�á�
//ע��2: ���ڸ����ں˿ռ�
//����ָ�����Ե�ַ�ͳ����ڴ��Ӧ��ҳĿ¼���ҳ����Ӷ������Ƶ�ҳĿ¼��ҳ���Ӧ��ԭ�����ڴ�
//ҳ����������ҳ��ӳ�������ʹ�á�����ʱ����������ҳ���������ҳ��ԭ�����ڴ��ڴ�����������
//�˺��������̣������̺����ӽ��̣��������ڴ�����ֱ����һ������ִ��д����ʱ���ں˲Ż�Ϊд����
//���̷����µ��ڴ�Ҳ��дʱ���ƻ��ƣ�
//������from��to - ���Ե�ַ�� size - ��Ҫ���Ƶ��ڴ泤�ȣ���λ���ֽڣ���
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

////ȡ��д����ҳ�溯���������쳣�жϹ�����д�����쳣�Ĵ���дʱ���ƣ�
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

////����һҳ�ڴ�
//������from_paddr - ����Դ�����ַ��to_paddr -- ����Ŀ�������ַ
static void
copy_page(unsigned long from_paddr, unsigned long to_paddr)
{
	unsigned long *from = (unsigned long *)from_paddr, *to = (unsigned long *)to_paddr;
	int i;
	for(i = 0; i < 0x1000 / sizeof(unsigned long); i++){
		*(from++) = *(to++);
	}
}

////ִ��д����ҳ�洦��������Page Fault�쳣��������Ϊ��ҳд�������𣬾ͻ���øú���
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
										  *((unsigned long *) (((address>>20) &0xffc) + PAGE_DIR_ADDR) ))); //ҳ�����ַ
	un_wp_page(table_entry);

}

/*------------------------------------------��������-----------------------------------------------*/
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



