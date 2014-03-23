/*************************************************************************//**
 *****************************************************************************
 * @file   forkexit.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Tue May  6 00:37:15 2008
 *****************************************************************************
 *****************************************************************************/

#include "bootpack.h"
#include <string.h>
#include "fs.h"

PRIVATE void copyTSS(struct TSS32 *dst, struct TSS32 *src);
//PRIVATE void cleanup(struct proc * proc);

PRIVATE void copyTSS(struct TSS32 *dst, struct TSS32 *src){
	dst->backlink = src->backlink;
    dst->esp0 = src->esp0;
	dst->ss0 = src->ss0;
	dst->esp1 = src->esp1;
	dst->ss1 = src->ss1;
	dst->esp2 = src->esp2;
	dst->ss2 = src->ss2;
	dst->cr3 = src->cr3;
	
	dst->eip = src->eip;
	dst->eflags = src->eflags;
	dst->eax = src->eax;
	dst->ecx = src->ecx;
	dst->edx = src->edx;
	dst->ebx = src->ebx;
	dst->esp = src->esp;
	dst->ebp = src->ebp;
	dst->esi = src->esi;
	dst->edi = src->edi;
	
	dst->es = src->es;
	dst->cs = src->cs;
	dst->ss = src->ss;
	dst->ds = src->ds;
	dst->fs = src->fs;
	dst->gs = src->gs;

	
	
	
}

/*****************************************************************************
 *                                do_fork
 *****************************************************************************/
/**
 * Perform the fork() syscall.
 * 
 * @return  Zero if success, otherwise -1.
 *****************************************************************************/
PUBLIC struct TASK* do_fork(struct TASK *task_parent, struct TSS32 *tss)
{
	//printTSSInfo(&(task_parent->tss));
	debug("task_parent->pid = %d",task_parent->pid);
	/* find a free slot in proc_table */
	//创建一个新任务
	struct TASK *new_task = task_alloc();
	
	if (new_task == 0) {/* no free slot */
		debug("no free task struct");
		return 0;
	}
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	/* duplicate the process table */
	copyTSS(&(new_task->tss),tss);
	new_task->tss.ss0 = task_parent->tss.ss0;
	new_task->tss.esp0 = task_parent->tss.esp0;
	
	new_task->level = task_parent->level;
	new_task->priority = task_parent->priority;
	new_task->fhandle = task_parent->fhandle;
	new_task->fat = task_parent->fat;
	//使用和task_parnent同一个控制台
	//new_task->cons  = -1;
	new_task->cons = task_parent->cons;
	//struct CONSOLE *new_console = (struct CONSOLE *)memman_alloc(memman,sizeof(struct CONSOLE));

	/* duplicate the process: T, D & S */
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task_parent->ldt;

	/* Text segment */
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	u8 * code_seg = (u8 *)memman_alloc_4k(memman, codeLimit);
	debug("text segment size = %d, add = %d",codeLimit, codeBase);
	set_segmdesc(new_task->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy(code_seg,codeBase,codeLimit);
	new_task->cs_base = code_seg;
	
	/* Data segment */
	int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	u8 *data_seg = (u8 *)memman_alloc_4k(memman, dataLimit);
	debug("data segment size = %d, src base add = %d",dataLimit,dataBase);
	set_segmdesc(new_task->ldt + 1, dataLimit - 1, (int) data_seg, AR_DATA32_RW + 0x60);
	phys_copy(data_seg,dataBase,dataLimit);
	new_task->ds_base = data_seg;
	
	
	//struct file_desc **filp_parent = task_parent->filp, **filp_new = new_task->filp;
	//int i;
	//for (i = 0; i < NR_FILES; i++) {
	//	filp_new[i] = filp_parent[i];
	//	if (filp_new[i]) {
	//		filp_new[i]->fd_cnt++;
	//		filp_new[i]->fd_inode->i_cnt++;
	//	}
	//}

	return new_task;
}

