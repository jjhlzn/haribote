
#include "bootpack.h"
#include "elf.h"

/**
 * Perform the exec() system call
 * @return Zero if successful, otherwise -1
**/
int do_exec(const char *name, char *argv[], int *fat, int *regs_push_by_interrupt)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET  *sht;

	/* 查找文件在磁盘中的信息 */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	
	if(finfo == 0){
		debug("can't find file[%s]",name);
		return -1;
	}

	if (finfo != 0) {
		/* 加载文件信息 */
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat); //代码段
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			
			//释放之前段内存(Data & Code)
			free_mem(task);
			
			q = (char *) memman_alloc_4k(memman, segsiz); //分配数据段
			task->ds_base = (int) q;
			task->cs_base = (int) p;
			set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			
			debug("start app: %s",name);
			debug("code segment: size = %d, add = %d",appsiz,(int)p);
			debug("data segment: size = %d, add = %d",segsiz,(int)q);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
		
			//跳到用户态去执行用户程序, 
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0),0,0); 
			
			return 0;
		} else if (appsiz >= sizeof(Elf32_Ehdr) && strncmp(p + 1, "ELF", 3) == 0 ) {
			
			Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)p;
			debug_Elf32_Ehd(elf_hdr);
			int i;
			
			//加载字符串表
			Elf32_Shdr* str_section = (Elf32_Shdr*)(p + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize);
			char *str_contents = (char *)(p + str_section->sh_offset);
			
			for (i = 0; i<elf_hdr->e_shnum; i++) {
				Elf32_Shdr *elf_shdr = (Elf32_Shdr*)(p + elf_hdr->e_shoff + i * elf_hdr->e_shentsize);
				char *sh_name = str_contents+elf_shdr->sh_name;
				if(strlen(sh_name) == 0)
					continue;
				debug("name = %s",sh_name);
			}
			
			u8 *cod_seg;
			cod_seg =  (u8 *)memman_alloc_4k(memman, 1024*64); //TODO: 代码固定在64K
			//data_seg = (u8 *)memman_alloc_4k(memman, 1024*64); //TODO: 数据段固定在64K
			task->ds_base = (int) cod_seg;  //代码和数据段用同一个段
			task->cs_base = (int) cod_seg;
			
			esp = 1024 * 64 - 4;
			debug("esp = %d",esp);
			//准备好argc,argv
			char **p = argv;
			
			int PROC_ORIGIN_STACK = 500;
			char arg_stack0[PROC_ORIGIN_STACK+4];
			int *tmp = arg_stack0;
			int stack_len = 4;
			int str_count = 0;
			while(*p++){
				assert(stack_len + 2 * sizeof(char *) < PROC_ORIGIN_STACK);
				stack_len += sizeof(char *);
				str_count++;
			}
			*tmp = str_count;
			
			char *arg_stack = arg_stack0 + 4;
			*((int *)(&arg_stack[stack_len])) = 0;
			stack_len += sizeof(char *);
			
			char ** q = (char**)arg_stack;
			for(p = argv; *p != 0; p++){
				*q++ = &arg_stack[stack_len];
				
				assert(stack_len + strlen(*p) + 1 < PROC_ORIGIN_STACK);
				strcpy(&arg_stack[stack_len], *p);
				stack_len += strlen(*p);
				arg_stack[stack_len] = 0;
				stack_len++;
			}
			
			phys_copy(esp-stack_len, arg_stack0, stack_len);
			esp = esp-stack_len;
			
			set_segmdesc(task->ldt + 0, 1024*64 - 1, (int) cod_seg, AR_CODE32_ER + 0x60); //代码和数据段其实指向同一个空间
			set_segmdesc(task->ldt + 1, 1024*64 - 1, (int) cod_seg, AR_DATA32_RW + 0x60);
			
			//加载数据段、代码段
			for (i=0; i<elf_hdr->e_phnum; i++){
				Elf32_Phdr *elf_phdr = (Elf32_Phdr *)(p + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
				if(elf_phdr->p_type == PT_LOAD){
					//debug("see PT_LOAD section");
					phys_copy(cod_seg +(int)elf_phdr->p_vaddr, p + elf_phdr->p_offset, elf_phdr->p_filesz);
				}
			}
			
			
			
			start_app(elf_hdr->e_entry, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0),0,0);  
			
		} else {
			return -1;
		}
	}
	return -1;
}


static void debug_Elf32_Ehd(Elf32_Ehdr* elf_hdr)
{
	debug("-------------------Elf32 header-------------------");
	debug("e_ident = %s", elf_hdr->e_ident);
	debug("e_type = %d",elf_hdr->e_type);
	debug("e_machine = %d",elf_hdr->e_machine);
	debug("e_version = %d",elf_hdr->e_version);
	debug("e_entry = %d",elf_hdr->e_entry);
	debug("e_phoff = %d",elf_hdr->e_phoff);
	debug("e_shoff = %d",elf_hdr->e_shoff);
	debug("e_flags = %d",elf_hdr->e_flags);
	debug("e_ehsize = %d",elf_hdr->e_ehsize);
	debug("e_phentsize = %d",elf_hdr->e_phentsize);
	debug("e_phnum = %d",elf_hdr->e_phnum);
	debug("e_shentsize = %d",elf_hdr->e_shentsize);
	debug("e_shnum = %d",elf_hdr->e_shnum);
	debug("e_shstrndx = %d",elf_hdr->e_shstrndx);
	debug("--------------------------------------------------");
}

PRIVATE void free_mem(struct TASK *task)
{
	debug("-----------free memory-------------------");
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task->ldt;
	
	/* Text segment */
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	memman_free_4k(memman, codeBase, codeLimit);
	debug("free text segment: add = %d, size = %d", codeBase, codeLimit);
	
	/* Data segment */
	int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	memman_free_4k(memman, dataBase, dataLimit);
	debug("free data segment: add = %d, size = %d", dataBase, dataLimit);
	
	debug("-----------------------------------------");
	
	/* TODO: CONSOLE */
}
