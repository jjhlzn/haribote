#include "bootpack.h"
#include "elf.h"
#include <string.h>
#include "linkedlist.h"

/**
 * Perform the exec() system call
 * @return Zero if successful, otherwise -1
**/
PRIVATE void free_mem(struct TASK *task);
//static void debug_Elf32_Ehd(Elf32_Ehdr* elf_hdr);
int prepare_args(u8* cod_seg, unsigned int data_limit, struct Node *list);

int do_exec(char *name, char *argv[], int *fat, int *regs_push_by_interrupt)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;

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
		
		if (appsiz >= sizeof(Elf32_Ehdr) && strncmp(p + 1, "ELF", 3) == 0 ) {
			
			free_mem(task);
			
			struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
			struct TASK *task = task_now();
			int esp;
			
			Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)p;
			//debug_Elf32_Ehd(elf_hdr);
			
			int i;
			
			//加载字符串表
			Elf32_Shdr* str_section = (Elf32_Shdr*)(p + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize);
			char *str_contents = (char *)(p + str_section->sh_offset);
			
			for (i = 0; i<elf_hdr->e_shnum; i++) {
				Elf32_Shdr *elf_shdr = (Elf32_Shdr*)(p + elf_hdr->e_shoff + i * elf_hdr->e_shentsize);
				char *sh_name = str_contents+elf_shdr->sh_name;
				if(strlen(sh_name) == 0)
					continue;
				//debug("name = %s",sh_name);
			}
			
			int data_limit = 1024 * 512;
			u8 *cod_seg =  (u8 *)memman_alloc_4k(memman, data_limit); //TODO: 代码固定尺寸
			task->ds_base = (int) cod_seg;  //代码和数据段用同一个段
			task->cs_base = (int) cod_seg;
			
			set_segmdesc(task->ldt + 0, data_limit - 1, (int) cod_seg, AR_CODE32_ER + 0x60); //代码和数据段其实指向同一个空间
			set_segmdesc(task->ldt + 1, data_limit - 1, (int) cod_seg, AR_DATA32_RW + 0x60);
			//加载数据段、代码段
			//debug("elf_hdr->e_phnum = %d",elf_hdr->e_phnum);
			for (i=0; i<elf_hdr->e_phnum; i++){
				Elf32_Phdr *elf_phdr = (Elf32_Phdr *)(p + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
				//debug("p_type = %d",elf_phdr->p_type);
				if(elf_phdr->p_type == PT_LOAD){
					phys_copy(cod_seg +(int)elf_phdr->p_vaddr, p + elf_phdr->p_offset, elf_phdr->p_filesz);
				}
			}
			
			//提取参数，每个参数最多只能有1024长度。
			struct Node *list = NULL;
			int count = 0;
			while(1){
				char *arg = argv[count];
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
				}
			}
	
			int argc = GetSize(list);
			esp = prepare_args(cod_seg, data_limit, list);
			
			start_app_elf((int)elf_hdr->e_entry, 0 * 8 + 4, esp-4, 1 * 8 + 4, &(task->tss.esp0), argc, esp); 
			
		} else {
			return -1;
		}
	}
	return -1;
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
