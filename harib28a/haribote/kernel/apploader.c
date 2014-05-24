#include "bootpack.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
//#include "hd.h"
#include "fs.h"
#include "linkedlist.h"
#include "elf.h"
#include <fcntl.h>

 void load_hrb(char *p, int appsiz, struct Node *list);
 void load_elf(char *p, struct Node *list);

PRIVATE char *get_next_arg(char *cmdline, int *skip);
static char * get_file_from_hd(char *name);
int sys_open(const char * filename,int flag,int mode);
int sys_read(unsigned int fd,char * buf,int count);
int prepare_args(u8* cod_seg, unsigned int data_limit, struct Node *list);

static char * 
get_file_from_hd(char *name)
{
	int fd = sys_open(name,O_RDONLY,0);
	if(fd<0){
		debug("can't find in hd");
		return NULL;
	}
	int filesz = 100 * 1024;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *p = (char *)memman_alloc_4k(memman, filesz);
	int n = sys_read(fd, p, filesz);
	if( n < 0 ){
		debug("read failed!");
		return NULL;
	}
	return p;
}

int load_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p = NULL;
	int i, appsiz;
	char *origin_cmdline = cmdline;

	/* 获取程序文件名 */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; 
	
	//提取参数，每个参数最多只能有1024长度。
	struct Node *list = NULL;
	int count = 0;
	while(1){
		int skip = 0;
		char *arg = get_next_arg(cmdline, &skip);
		if(arg == NULL)
			break;
		else{
			struct Node *node = CreateNode(arg);
			if(list == NULL){
				list = node;
			}else{
				Append(list,node);
			}
			count++;
			cmdline += skip;
		}
	}
	
	/* 查找文件在磁盘中的信息 */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		
		if(finfo == NULL){
			debug("cmdline = %s",origin_cmdline);
			p = get_file_from_hd(origin_cmdline);
		}
		
		if( p == NULL){
			/* 加入.hrb后缀再试试 */
			name[i    ] = '.';
			name[i + 1] = 'H';
			name[i + 2] = 'R';
			name[i + 3] = 'B';
			name[i + 4] = 0;
			finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
		}
	} 

	if (finfo != 0) {
		/* 加载文件信息 */
		appsiz = finfo->size;
		int filesz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat); //代码段
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			load_hrb(p, appsiz, list);
		} else if (appsiz >= sizeof(Elf32_Ehdr) && strncmp(p + 1, "ELF", 3) == 0 && p[0] == 0x7F ) {
			load_elf(p, list);
		} else {
			cons_putstr0(cons, ".hrb or .elf file format error.\n");
		}
		memman_free_4k(memman, (int) p, filesz);
		cons_newline(cons);
		return 1;
	}else if((finfo == 0 && p != NULL)){
		load_elf(p, list);
		cons_newline(cons);
		return 1;
	}else{
		debug("can't find file[%s]",name);
	}
	
	return 0;
}

 void load_hrb(char *p, int appsiz, struct Node *list)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	char  *q;
	struct SHTCTL *shtctl;
	struct SHEET *sht;
	
	int i, segsiz, datsiz, esp, dathrb;
    segsiz = *((int *) (p + 0x0000));
    esp    = *((int *) (p + 0x000c));
	datsiz = *((int *) (p + 0x0010));
	dathrb = *((int *) (p + 0x0014));
	//debug("esp = %d",esp);
	q = (char *) memman_alloc_4k(memman, segsiz); //分配数据段
	task->ds_base = (int) q;
	task->cs_base = (int) p;
	set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
	set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
	
	debug("start app: %s", (char *)list->data);
	debug("code segment: size = %d, add = %d",appsiz,(int)p);
	debug("data segment: size = %d, add = %d",segsiz,(int)q);
	for (i = 0; i < datsiz; i++) {
		q[esp + i] = p[dathrb + i];
	}
	start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0), 0, 0); 

	shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	for (i = 0; i < MAX_SHEETS; i++) {
		sht = &(shtctl->sheets0[i]);
		if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
			/* 傾僾儕偑奐偒偭傁側偟偵偟偨壓偠偒傪敪尒 */
			sheet_free(sht);	/* 暵偠傞 */
		}
	}
	for (i = 0; i < 8; i++) {	/* 僋儘乕僘偟偰側偄僼傽僀儖傪僋儘乕僘 */
		if (task->fhandle[i].buf != 0) {
			memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
			task->fhandle[i].buf = 0;
		}
	}
	timer_cancelall(&task->fifo);
	memman_free_4k(memman, (int) q, segsiz);
	task->langbyte1 = 0;
}


 void load_elf(char *p, struct Node *list)
{
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	int esp,i;
	
	Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)p;
	//debug_Elf32_Ehd(elf_hdr);

	int data_limit = 1024 * 100; //TODO: 代码固定尺寸
	debug("data_limit = %d(0x%08.8x)",data_limit,data_limit);
	u8 *cod_seg =  (u8 *)memman_alloc_4k(memman, data_limit); 
	debug("cod_seg = %d(0x%08.8x)", (u32)cod_seg, (u32)cod_seg);
	
	task->ds_base = (int) cod_seg;  //代码和数据段用同一个段
	task->cs_base = (int) cod_seg;
	
	set_segmdesc(task->ldt + 0, data_limit - 1, (int) cod_seg, AR_CODE32_ER + 0x60); //代码和数据段其实指向同一个空间
	set_segmdesc(task->ldt + 1, data_limit - 1, (int) cod_seg, AR_DATA32_RW + 0x60);
	
	//加载数据段、代码段
	for (i=0; i<elf_hdr->e_phnum; i++){
		Elf32_Phdr *elf_phdr = (Elf32_Phdr *)(p + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
		//debug("p_type = %d",elf_phdr->p_type);
		if(elf_phdr->p_type == PT_LOAD){
			if( (u32)elf_phdr->p_vaddr > 32 * 1024 * 1024){
				debug("WARN: virtual addr > 32MB, ignore the section");
			}else{
				//debug("copy to addr %d(0x%08.8x)", (unsigned int)elf_phdr->p_vaddr,(unsigned int)elf_phdr->p_vaddr);
				phys_copy(cod_seg +(int)elf_phdr->p_vaddr, p + elf_phdr->p_offset, elf_phdr->p_filesz);
				//debug("copy PT_LOAD finished");
			}
		}
	}
	
	int argc = GetSize(list);
	esp = prepare_args(cod_seg, data_limit, list);
	//esp = data_limit - 512;
	
	debug("invoke start_app_elf");
	
	/* cs = 0 * 8 + 4表示第0个ldt */
	start_app_elf((int)elf_hdr->e_entry, 0 * 8 + 4, esp-4, 1 * 8 + 4, &(task->tss.esp0), argc, esp); 
}

//PRIVATE void debug_Elf32_Ehd(Elf32_Ehdr* elf_hdr)
//{
//	debug("-------------------Elf32 header-------------------");
//	debug("e_ident = %s", elf_hdr->e_ident);
//	debug("e_type = %d",elf_hdr->e_type);
//	debug("e_machine = %d",elf_hdr->e_machine);
//	debug("e_version = %d",elf_hdr->e_version);
//	debug("e_entry = %d",elf_hdr->e_entry);
//	debug("e_phoff = %d",elf_hdr->e_phoff);
//	debug("e_shoff = %d",elf_hdr->e_shoff);
//	debug("e_flags = %d",elf_hdr->e_flags);
//	debug("e_ehsize = %d",elf_hdr->e_ehsize);
//	debug("e_phentsize = %d",elf_hdr->e_phentsize);
//	debug("e_phnum = %d",elf_hdr->e_phnum);
//	debug("e_shentsize = %d",elf_hdr->e_shentsize);
//	debug("e_shnum = %d",elf_hdr->e_shnum);
//	debug("e_shstrndx = %d",elf_hdr->e_shstrndx);
//	debug("--------------------------------------------------");
//}
//PRIVATE void debug_Elf32_Phdr(Elf32_Phdr *phdr)
//{
//	debug("-----------------Program header-------------------");
//	debug("p_type = %d", phdr->p_type);
//	debug("p_offset = %d", phdr->p_offset);
//	debug("p_vaddr = %d", phdr->p_vaddr);
//	debug("p_paddr = %d", phdr->p_paddr);
//	debug("p_filesz = %d", phdr->p_filesz);
//	debug("p_memsz = %d", phdr->p_memsz);
//	debug("p_flags = %d", phdr->p_flags);
//	debug("p_align = %d", phdr->p_align);
//	debug("--------------------------------------------------");
//}

//PRIVATE void debug_Elf32_Shdr(Elf32_Shdr *phdr)
//{
//	debug("-----------------Section header-------------------");
//	debug("sh_name = %s", phdr->sh_name);
//	debug("sh_type = %d", phdr->sh_type);
//	debug("sh_flags = %d", phdr->sh_flags);
//	debug("sh_addr = %d", phdr->sh_addr);
//	debug("sh_offset = %d", phdr->sh_offset);
//	debug("sh_size = %d", phdr->sh_size);
//	debug("sh_link = %d", phdr->sh_link);
//	debug("sh_info = %d", phdr->sh_info);
//	debug("sh_addralign = %d", phdr->sh_addralign);
//	debug("sh_entsize = %d", phdr->sh_entsize);
//	debug("--------------------------------------------------");
//}

int prepare_args(u8* cod_seg, unsigned int data_limit, struct Node *list)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char msg[200];
	int esp = -1;
	
	//封装成argv
	int count = GetSize(list); //参数个数
	char **argv = (char **)memman_alloc(memman,sizeof(char **) * (count+1)); //argv参数数组
	//准备argv数组
	int i = 0;
	while(list != NULL){
		debug("arg = %s",(char *)list->data);
		argv[i] =  (char *)list->data;
		list = list->next;
		i++;
	}
	argv[i] = 0;

	char **p_argv = argv;
	int PROC_ORIGIN_STACK = 512;
	char arg_stack[PROC_ORIGIN_STACK];

	//分配argv空间
	
	int stack_len = 0;
	
	//放置argc
	//*((int *)(&arg_stack[stack_len])) = count;
	//stack_len += 4;
	
	int argc = 0;
	while(*p_argv++){
		assert(stack_len + 2 * sizeof(char *) < PROC_ORIGIN_STACK);
		stack_len += sizeof(char *);
		argc++;
	}
	debug("argc = %d",argc);
	
	
	*((int *)(&arg_stack[stack_len])) = 0;
	stack_len += sizeof(char *);
	
	for(p_argv = argv; *p_argv != 0; p_argv++){
		assert(stack_len + strlen(*p_argv) + 1 < PROC_ORIGIN_STACK);
		strcpy(&arg_stack[stack_len], *p_argv);
		//debug("*p_argv = %s",*p_argv);
		stack_len += strlen(*p_argv);
		arg_stack[stack_len] = 0;
		stack_len++;
	}
	
	debug("here1");
	
	//释放准备参数时的内存
	memman_free(memman, (u32)argv, sizeof(char **) * (count+1));
	struct Node *tmp = NULL;
	while(list != NULL){
		tmp = list;
		list = list->next;
		memman_free_4k(memman, (u32)tmp->data, 1024);
		FreeNode(list);
	}
	
	debug("here2");
	
    esp = data_limit - PROC_ORIGIN_STACK;

	phys_copy(cod_seg+esp, arg_stack, stack_len);
	
	//debug("stack_len = %d",stack_len);
	u8 *stack = (u8 *)(cod_seg+esp);
	char *argv_contents = (char *)(stack + (argc + 1) * 4);
	for(i = 0; i<argc; i++){
		debug("argv_contents = %d",argv_contents);
		*((char **)stack) = (char *)((int)argv_contents - (int)cod_seg);  //等级argv[i]的地址
		argv_contents += strlen(argv_contents) + 1;
		stack += 4;
	}
	
	debug("here3");
	
	stack = (u8 *)(cod_seg+esp);
	string_memory(cod_seg+esp-4, stack_len, msg);
	//debug(msg);
	
	//检查栈中的内容
	argv_contents = (char *)(stack + (argc + 1) * 4);
	for(i = 0; i<argc; i++){
		//debug("argv[%d] = %s",i,argv_contents);
		//debug("&argv[0] = %d", (int)argv_contents - (int)cod_seg);
		//debug("stack[0] = %d", *((int *)stack));
		argv_contents += strlen(argv_contents) + 1;
		stack += 4;
	}
	
	//debug("argc = %d, argv = %d", argc, esp);
	//char ** pp = (char **)(cod_seg+esp);
	//debug("argv[0] = %d", (int)(pp[0]));
	//debug("tt = %d",*((int *)esp));
	
	debug("esp = 0x%08.8x", esp);
	
	return esp;
}

PRIVATE char *get_next_arg(char *cmdline, int *skip)
{
	//debug("1 = %s",cmdline);
	int i=0;
	while(cmdline[i] == ' ') i++;
	//debug("2 = %s",cmdline+i);
	
	if(cmdline[i] != 0){ //不是结尾处
		//移到结尾，或者移到下个参数前的空格处
		struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
		char *arg = (char *)memman_alloc_4k(memman,1024);
		int j = 0;
		while(cmdline[i] != ' ' && cmdline[i] != 0){
			arg[j] = cmdline[i];
			++i;
			++j;
		}
		arg[j] = 0;
		//debug("3 = %s",cmdline+i);
		*skip = i;
		return arg;
	}else{
		return NULL;
	}
}

