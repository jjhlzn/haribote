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
PRIVATE void free_mem(struct TASK *task);


//int sys_exit(int errcode)
//{
//	struct TASK *task = current;
//	if(task->forked == 1){
//		debug("pocess[%d, forked] die!", task->pid);
//		do_exit(task,0);
//	}else{
//		return &(task->tss.esp0);
//	}
//	return 0;
//}
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
	/* find a free slot in proc_table */
	//创建一个新任务
	//debug("do_fork");
	struct TASK *new_task = task_alloc();
	
	if (new_task == 0) {/* no free slot */
		debug("no free task struct");
		return 0;
	}
	new_task->forked = 1;
	new_task->parent_pid = task_parent->pid;
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	/* duplicate the process table */
	copyTSS(&(new_task->tss),tss);
	//debug("here1");
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	//debug("here2");
	new_task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	//debug("here3");
	new_task->tss.esp0 = new_task->cons_stack + 64 * 1024 - 12;

	//debug("new_task->tss.esp0 + 4 = %d", (int)(new_task->tss.esp0) + 4);
	*((int *) (new_task->tss.esp0 + 4)) = (int) task_parent->cons->sht;
	//debug("here4");
	*((int *) (new_task->tss.esp0 + 8)) = 32 * 1024 * 1024;
	//debug("here5");
	fifo32_init(&new_task->fifo, 128, cons_fifo, new_task);
	
	new_task->level = task_parent->level;
	new_task->priority = task_parent->priority;
	new_task->fhandle = task_parent->fhandle;
	new_task->fat = task_parent->fat;
	//使用和task_parnent同一个控制台
	new_task->cons = task_parent->cons;

	/* duplicate the process: T, D & S */
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task_parent->ldt;

	
	/* Text segment */
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	u8 * code_seg = (u8 *)memman_alloc_4k(memman, codeLimit);
	//debug("text size = %d, src_addr = %d, dest_addr = %d",codeLimit, codeBase, (int)code_seg);
	set_segmdesc(new_task->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy((void *)code_seg,(void *)codeBase,codeLimit);
	new_task->cs_base = (int)code_seg;
	
	/* Data segment */
	int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	u8 *data_seg = (u8 *)memman_alloc_4k(memman, dataLimit);
	//debug("data size = %d, src_addr = %d, dest_addr = %d",dataLimit,dataBase, (int)data_seg);
	set_segmdesc(new_task->ldt + 1, dataLimit - 1, (int) data_seg, AR_DATA32_RW + 0x60);
	phys_copy((void *)data_seg,(void *)dataBase,dataLimit);
	new_task->ds_base = (int)data_seg;
	
	
	struct file **filp_parent = task_parent->filp, **filp_new = new_task->filp;
	int i;
	for (i = 0; i < NR_FILES; i++) {
		filp_new[i] = filp_parent[i];
		if (filp_new[i]) {
			filp_new[i]->f_count++;
			filp_new[i]->f_inode->i_count++;
		}
	}
	return new_task;
}


PUBLIC struct TASK* do_fork_elf(struct TASK *task_parent, struct TSS32 *tss)
{
	/* find a free slot in proc_table */
	//创建一个新任务
	//debug("do_fork");
	struct TASK *new_task = task_alloc();
	
	if (new_task == 0) {/* no free slot */
		debug("no free task struct");
		return 0;
	}
	new_task->forked = 1;
	new_task->parent_pid = task_parent->pid;
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	/* duplicate the process table */
	copyTSS(&(new_task->tss),tss);

	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	new_task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	new_task->tss.esp0 = new_task->cons_stack + 64 * 1024 - 12;

	//debug("new_task->tss.esp0 + 4 = %d", (int)(new_task->tss.esp0) + 4);
	*((int *) (new_task->tss.esp0 + 4)) = (int) task_parent->cons->sht;
	*((int *) (new_task->tss.esp0 + 8)) = 32 * 1024 * 1024;
	fifo32_init(&new_task->fifo, 128, cons_fifo, new_task);
	
	new_task->level = task_parent->level;
	new_task->priority = task_parent->priority;
	new_task->fhandle = task_parent->fhandle;
	new_task->fat = task_parent->fat;
	//使用和task_parnent同一个控制台
	new_task->cons = task_parent->cons;

	/* duplicate the process: T, D & S */
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task_parent->ldt;

	
	/* Text segment */
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	u8 * code_seg = (u8 *)memman_alloc_4k(memman, codeLimit);
	//debug("text size = %d, src = %d, dest = %d",codeLimit, codeBase, (int)code_seg);
	set_segmdesc(new_task->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy((void *)code_seg,(void *)codeBase, codeLimit);
	new_task->cs_base = (int)code_seg;
	
	/* Data segment */
	set_segmdesc(new_task->ldt + 1, codeLimit - 1, (int) code_seg, AR_DATA32_RW + 0x60);
	new_task->ds_base = (int)code_seg;
	
	struct file **filp_parent = task_parent->filp, **filp_new = new_task->filp;
	int i;
	for (i = 0; i < NR_FILES; i++) {
		filp_new[i] = filp_parent[i];
		if (filp_new[i]) {
			filp_new[i]->f_count++;
			filp_new[i]->f_inode->i_count++;
		}
	}
	return new_task;
}


/*****************************************************************************
 *                                do_exit
 *****************************************************************************/
/**
 * Perform the exit() syscall.
 *
 * If proc A calls exit(), then MM will do the following in this routine:
 *     <1> inform FS so that the fd-related things will be cleaned up
 *     <2> free A's memory
 *     <3> set A.exit_status, which is for the parent
 *     <4> depends on parent's status. if parent (say P) is:
 *           (1) WAITING
 *                 - clean P's WAITING bit, and
 *                 - send P a message to unblock it
 *                 - release A's proc_table[] slot
 *           (2) not WAITING
 *                 - set A's HANGING bit
 *     <5> iterate proc_table[], if proc B is found as A's child, then:
 *           (1) make INIT the new parent of B, and
 *           (2) if INIT is WAITING and B is HANGING, then:
 *                 - clean INIT's WAITING bit, and
 *                 - send INIT a message to unblock it
 *                 - release B's proc_table[] slot
 *               else
 *                 if INIT is WAITING but B is not HANGING, then
 *                     - B will call exit()
 *                 if B is HANGING but INIT is not WAITING, then
 *                     - INIT will call wait()
 *
 * TERMs:
 *     - HANGING: everything except the proc_table entry has been cleaned up.
 *     - WAITING: a proc has at least one child, and it is waiting for the
 *                child(ren) to exit()
 *     - zombie: say P has a child A, A will become a zombie if
 *         - A exit(), and
 *         - P does not wait(), neither does it exit(). that is to say, P just
 *           keeps running without terminating itself or its child
 * 
 * @param status  Exiting status for parent.
 * 
 *****************************************************************************/
PUBLIC void do_exit(struct TASK *p, int status)
{
	int i;
	int pid = p->pid;

	/* 关闭该进程打开的文件 */
	debug("close files");
	struct file **filp = p->filp;
	for (i = 0; i < NR_FILES; i++) {
		if (filp[i]) {
			filp[i]->f_count++;
			filp[i]->f_inode->i_count++;
		}
	}

	

	/* 保存该进程的退出状态 */
	p->exit_status = status;
	debug("set exit_status of process[%d] = %d",p->pid, status);
	
	/* 如果当前进程有任何子进程, 那么让该进程的父进程为idle*/
	struct TASK *idle = get_task(1);
	for (i = 0; i < MAX_TASKS; i++) {
		struct TASK *tmp = get_task(i);
		if (tmp->parent_pid == pid) { /* is a child */
			//debug("I entered!!!!!!!");
			tmp->parent_pid = 1;  //TODO, 把idle的进程号写死为1
			if ( idle->flags == TASK_STATUS_WAITING && tmp->flags == TASK_STATUS_HANGING) {
				idle->wait_return_task = tmp;
				task_add(idle);
				tmp->flags = TASK_STATUS_UNUSED;
			}
		}
	}
 
	struct TASK  *parent_task = get_task(p->parent_pid);
	if (parent_task->flags == TASK_STATUS_WAITING) { /* parent is waiting */
		debug("process[%d] will go to UNUSED status!", p->pid);
		//父进程可以进入运行状态
		parent_task->wait_return_task = p;
		task_add(parent_task);
		/* 释放该进程的内存 */
		free_mem(p);
		task_exit(p, TASK_STATUS_UNUSED);
		
	}
	else { /* parent is not waiting */
		debug("process[%d] will go to HANGING status!", p->pid);
		/* 释放该进程的内存 */
		free_mem(p);
		task_exit(p, TASK_STATUS_HANGING);
	}
}




/*****************************************************************************
 *                                do_wait
 *****************************************************************************/
/**
 * Perform the wait() syscall.
 *
 * If proc P calls wait(), then MM will do the following in this routine:
 *     <1> iterate proc_table[],
 *         if proc A is found as P's child and it is HANGING
 *           - reply to P (cleanup() will send P a messageto unblock it)
 *           - release A's proc_table[] entry
 *           - return (MM will go on with the next message loop)
 *     <2> if no child of P is HANGING
 *           - set P's WAITING bit
 *     <3> if P has no child at all
 *           - reply to P with error
 *     <4> return (MM will go on with the next message loop)
 *
 *****************************************************************************/
PUBLIC int do_wait(struct TASK *task, int *status)
{
	int pid = task->pid;
	
	int i;
	int children = 0;

	for (i = 0; i < MAX_TASKS; i++) {
		struct TASK *tmp_task = get_task(i);
		if (tmp_task->parent_pid == pid) {
			children++;
			if (tmp_task->flags == TASK_STATUS_HANGING) {
				*status = tmp_task->exit_status;
				debug("tmp_task->exit_status = %d",tmp_task->exit_status);
				tmp_task->flags = TASK_STATUS_UNUSED;
				
				return tmp_task->pid;
			}
		}
	}

	if (children) {
		/* has children, but no child is HANGING */
		debug("process[%d] will go to wait status!",task->pid);
		task_wait(task);
		debug("process[%d] recover from wait status", task->pid);
		
		struct TASK *wait_return_task = task->wait_return_task;
		*status = wait_return_task->exit_status;
		debug("wait_return_task->exit_status = %d",wait_return_task->exit_status);
		return wait_return_task->pid;
	}
	else {
		panic("this task has no children!");
		return -1;
	}
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
	
	/* stack: TODO 这时候还不能释放自己的栈 */
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	debug("free console stack: add = %d, size = %d", task->cons_stack, 64 * 1024);
	
	/* cons fifo：TODO 这时候能不能释放*/
	memman_free_4k(memman, (u32)task->fifo.buf, 128*4);
	debug("free FIFO buf: add = %d, size = %d", task->fifo.buf, 128 * 4);
	
	debug("-----------------------------------------");
	
	/* TODO: CONSOLE */
}


