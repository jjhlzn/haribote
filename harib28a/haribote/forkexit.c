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
	/* find a free slot in proc_table */
	//创建一个新任务
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
	debug("text segment size = %d, add = %d",codeLimit, codeBase);
	set_segmdesc(new_task->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy((void *)code_seg,(void *)codeBase,codeLimit);
	new_task->cs_base = (int)code_seg;
	
	/* Data segment */
	int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	u8 *data_seg = (u8 *)memman_alloc_4k(memman, dataLimit);
	debug("data segment size = %d, src base add = %d",dataLimit,dataBase);
	set_segmdesc(new_task->ldt + 1, dataLimit - 1, (int) data_seg, AR_DATA32_RW + 0x60);
	phys_copy((void *)data_seg,(void *)dataBase,dataLimit);
	new_task->ds_base = (int)data_seg;
	
	
	struct file_desc **filp_parent = task_parent->filp, **filp_new = new_task->filp;
	int i;
	for (i = 0; i < NR_FILES; i++) {
		filp_new[i] = filp_parent[i];
		if (filp_new[i]) {
			filp_new[i]->fd_cnt++;
			filp_new[i]->fd_inode->i_cnt++;
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
	int parent_pid = p->parent_pid;

	/* 释放该进程打开的文件 */
	

	/* 释放该进程的内存 */
	free_mem(pid);

	/* 保存该进程 */
	p->exit_status = status;
 
	struct TASK  *parent_task = get_task(p->parent_pid);
	if (parent_task.flags == TASK_STATUS_WAITING) { /* parent is waiting */
		//proc_table[parent_pid].p_flags = TASK_STATUS_RUNNING;
		
		//移
		task_remove(task);
		
		//父进程可以进入运行状态
		task_add(parent_task);
	}
	else { /* parent is not waiting */
		proc_table[pid].p_flags |= HANGING;
	}

	/* if the proc has any child, make INIT the new parent */
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
		if (proc_table[i].p_parent == pid) { /* is a child */
			proc_table[i].p_parent = INIT;
			if ((proc_table[INIT].p_flags & WAITING) && (proc_table[i].p_flags & HANGING)) {
				proc_table[INIT].p_flags &= ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
	
	
	//假如当前进程是通过fork调用创建的，那么可以直接结束这个任务
	if(task->forked == 1){
		debug("pocess[%d, forked] die!", task->pid);
		for (;;) {  //为什么要一个无限循环
			task_sleep(task);
		}
	}else{
		return &(task->tss.esp0);
	}
}

/*****************************************************************************
 *                                cleanup
 *****************************************************************************/
/**
 * Do the last jobs to clean up a proc thoroughly:
 *     - Send proc's parent a message to unblock it, and
 *     - release proc's proc_table[] entry
 * 
 * @param proc  Process to clean up.
 *****************************************************************************/
PRIVATE void cleanup(struct proc * proc)
{
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(proc);
	msg2parent.STATUS = proc->exit_status;
	send_recv(SEND, proc->p_parent, &msg2parent);

	proc->p_flags = FREE_SLOT;
}

