
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
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0)); 
			
			return 0;
		} else if (appsiz >= sizeof(Elf32_Ehdr) && strncmp(p + 1, "ELF", 3) == 0 ) {
			//Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)p;
			//debug_Elf32_Ehd(elf_hdr);
			
			//int i;
			
			////加载字符串表
			//Elf32_Shdr* str_section = (Elf32_Shdr*)(p + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize);
			//char *str_contents = (char *)(p + str_section->sh_offset);
			
			//for (i = 0; i<elf_hdr->e_shnum; i++) {
			//	Elf32_Shdr *elf_shdr = (Elf32_Shdr*)(p + elf_hdr->e_shoff + i * elf_hdr->e_shentsize);
			//	char *sh_name = str_contents+elf_shdr->sh_name;
			//	if(strlen(sh_name) == 0)
			//		continue;
			//	debug("name = %s",sh_name);
			//}
			
			//u8 *cod_seg, *data_seg;
			//cod_seg  = (u8 *)memman_alloc_4k(memman, 1024*64); //64K
			//data_seg = (u8 *)memman_alloc_4k(memman, 1024*64); //64K
			////加载数据段、代码段
			//for (i=0; i<elf_hdr->e_phnum; i++){
			//	Elf32_Phdr *elf_phdr = (Elf32_Phdr *)(p + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
			//	//debug("p_type = %d",elf_phdr->p_type);
			//	debug_Elf32_Phdr(elf_phdr);
			//	
			//	if(elf_phdr->p_type == PT_LOAD){
			//		if ( elf_phdr->p_flags & 0x04 ){ //可以执行
			//			phys_copy(cod_seg, p + elf_phdr->p_offset,elf_phdr->filesz);
			//		}else{
			//			phys_copy(data_seg, p + elf_phdr->p_offset, elf_phdr->filesz);
			//		}
			//	}
			//}
			
			//start_app(elf_hdr->e_entry, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0)); 
			
		} else {
			//cons_putstr0(cons, ".hrb or .elf file format error.\n");
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
