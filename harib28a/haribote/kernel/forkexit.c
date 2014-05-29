#include "bootpack.h"
#include <string.h>
#include <fs.h>
#include <signal.h>

PRIVATE void copyTSS(struct TSS32 *dst, struct TSS32 *src);
PRIVATE void free_mem(struct TASK *task);
static void copy_task(struct TASK *dst, struct TSS32 *tss);
static void copy_mem(struct TASK *dst);
static void tell_father(int pid);


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


PUBLIC struct TASK* do_fork_elf(struct TSS32 *tss)
{
	/* 找到一个空闲的进程结构 */
	struct TASK *dst = task_alloc();
	if (dst == 0) {/* 没有空闲的进程结构 */
		panic("no free task struct");
		return 0;
	}
	
	copy_task(dst,tss);
	copy_mem(dst);
	
	int i;
	struct file *f;
	for (i = 0; i < NR_FILES; i++) {
		if ( (f=dst->filp[i]) ) {
			f->f_count++;
			f->f_inode->i_count++;
		}
	}
	return dst;
}

//////拷贝进程结构数据
static void 
copy_task(struct TASK *dst, struct TSS32 *tss)
{
	struct TASK *src = current;
	int old_pid = dst->pid;
	int old_sel = dst->sel;
	struct TSS32 old_tss32 = dst->tss;
	*dst = *src;
	dst->pid = old_pid;
	dst->sel = old_sel;
	dst->tss = old_tss32;
	
	/* 把当前进程的寄存器拷贝到新创建进程的tss中 */
	copyTSS(&(dst->tss),tss);
	
	dst->forked = 1; /* 标志为通过fork创建的 */
	dst->parent_pid = src->pid; /* 设置父进程 */
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	/* 初始keyboard按键接收缓冲*/
	int *kb_buf_fifo32 = (int *) memman_alloc_4k(memman, 128 * 4);
	fifo32_init(&(dst->ch_buf), 128, kb_buf_fifo32, dst);
	
    /* 初始化新进程的内核栈 */
	dst->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	dst->tss.esp0 = dst->cons_stack + 64 * 1024 - 12;
	
	/* 初始化新进程的中断等产生的信息队列 */
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	fifo32_init(&(dst->fifo), 128, cons_fifo, dst);
}

static void copy_mem(struct TASK *dst)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *src = current;
	
	/* duplicate the process: T, D & S */
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)src->ldt;

	/* Text segment */
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	debug("codelimit = %d", codeLimit);
	u8 * code_seg = (u8 *)memman_alloc_4k(memman, codeLimit);
	if(code_seg == 0){
		panic("no memory");
	}
	debug("text size = %d, src = %d, dest = %d", codeLimit, codeBase, (int)code_seg);
	set_segmdesc(dst->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy((void *)code_seg,(void *)codeBase, codeLimit);
	dst->cs_base = (int)code_seg;
	
	/* Data segment */
	set_segmdesc(dst->ldt + 1, codeLimit - 1, (int) code_seg, AR_DATA32_RW + 0x60);
	dst->ds_base = (int)code_seg;
}

static void tell_father(int pid)
{
	int i;

	if (pid)
		for (i = 0; i < MAX_TASKS; i++) {
			struct TASK *task = &(taskctl->tasks0[i]);
			if (task->flags == 0)
				continue;
			if (task->pid != pid)
				continue;
			task->signal |= (1<<(SIGCHLD-1));
			return;
		}
	/* if we don't find any fathers, we just release ourselves */
	/* This is not really OK. Must change it to make father 1 */
	printk("BAD BAD - no father found\n\r");
}

////退出当前任务的执行，并设置退出状态, 注意：该函数不会返回
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
 
	//struct TASK  *parent_task = get_task(p->parent_pid);
	
	/* 释放该进程的内存 */
	debug("TASK[%d]: go to HANGING status!", p->pid);
	free_mem(p);
	tell_father(p->parent_pid);
	/* 当前进程退出，并且执行任务切换 */
	task_exit(p, TASK_STATUS_HANGING);
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
	int has_childen = 0;
repeat:
	for (i = 0; i < MAX_TASKS; i++) {
		struct TASK *tmp_task = get_task(i);
		if (tmp_task->parent_pid == pid) {
			has_childen = 1;
			if (tmp_task->flags == TASK_STATUS_HANGING) {
				*status = tmp_task->exit_status;
				debug("tmp_task->exit_status = %d",tmp_task->exit_status);
				tmp_task->flags = TASK_STATUS_UNUSED;
				
				return tmp_task->pid;
			}
		}
	}

	if (has_childen) {
		/* 进入wait状态，执行进程调度 */
		task_wait();
		debug("process[%d] recover from wait status", task->pid);
		goto repeat;
		//struct TASK *wait_return_task = task->wait_return_task;
		//*status = wait_return_task->exit_status;

		//debug("wait_return_task->exit_status = %d",wait_return_task->exit_status);
		//return wait_return_task->pid;
	}
	else {
		debug("BAD BAD, this task has no children!");
		return -1;
	}
}

PRIVATE void free_mem(struct TASK *task)
{
	debug("-----------free memory-------------------");
	
	//struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	//struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task->ldt;
	
	///* Text segment */
	//int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	//memman_free_4k(memman, codeBase, codeLimit);
	//debug("free text segment: add = %d, size = %d", codeBase, codeLimit);
	
	///* Data segment */
	//int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	//memman_free_4k(memman, dataBase, dataLimit);
	//debug("free data segment: add = %d, size = %d", dataBase, dataLimit);
	
	///* stack: TODO 这时候还不能释放自己的栈 */
	//memman_free_4k(memman, task->cons_stack, 64 * 1024);
	//debug("free console stack: add = %d, size = %d", task->cons_stack, 64 * 1024);
	
	///* cons fifo：TODO 这时候能不能释放*/
	//memman_free_4k(memman, (u32)task->fifo.buf, 128*4);
	//debug("free FIFO buf: add = %d, size = %d", task->fifo.buf, 128 * 4);
	
	debug("-----------------------------------------");
	
	/* TODO: CONSOLE */
}

/**********************************************************************************************/

PUBLIC struct TASK* do_fork( struct TSS32 *tss)
{
	panic("don't support");
}
