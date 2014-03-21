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
	//dst->backline = src->backline;
    //dst->esp0 = src->esp0;
	
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
PUBLIC struct TASK* do_fork(struct TASK *task_parent)
{
	/* find a free slot in proc_table */
	//struct proc* p = proc_table;
	//int i;
	//for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++)
	//	if (p->p_flags == FREE_SLOT)
	//		break;
	//创建一个新任务
	struct TASK *new_task = task_alloc();
	
	if (new_task == 0) {/* no free slot */
		debug("no free task struct");
		return 0;
	}
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	/* duplicate the process table */
	//int pid = mm_msg.source;
	//u16 child_ldt_sel = p->ldt_sel;
	//*p = proc_table[pid];
	//p->ldt_sel = child_ldt_sel;
	//p->p_parent = pid;
	//sprintf(p->name, "%s_%d", proc_table[pid].name, child_pid);
	//int oldldtr = new_task->tss.ldtr;
	//new_task->tss = task_parent->tss;
	//new_task->tss.ldtr = oldldtr;
	copyTSS(new_task,task_parent);
	
	new_task->level = task_parent->level;
	new_task->priority = task_parent->priority;
	new_task->fhandle = task_parent->fhandle;
	new_task->fat = task_parent->fat;
	//使用和task_parnent同一个控制台
	new_task->cons  = -1;
	//new_task->cons = task_parent->cons;

	/* duplicate the process: T, D & S */
	//struct descriptor * ppd;
	
	
	struct SEGMENT_DESCRIPTOR *pldt = (struct SEGMENT_DESCRIPTOR *)&task_parent->ldt;

	/* Text segment */
	//ppd = &proc_table[pid].ldts[INDEX_LDT_C];
	debug("limit_high = %d, limit_low = %d", pldt[0].limit_high, pldt[0].limit_low);
	debug("text segment size = %d, add = %d",DESCRIPTOR_LIMIT(pldt[0]), DESCRIPTOR_BASE(pldt[0]));
	//int tmp = pldt[0].limit_high;
	//debug("DESCRIPTOR_LIMIT(pldt[0]) = %d",    ((((int)pldt[0].limit_high & 0x0F) << 16 ) + pldt[0].limit_low)   );
	
	int codeLimit = DESCRIPTOR_LIMIT(pldt[0]), codeBase = DESCRIPTOR_BASE(pldt[0]);
	u8 * code_seg = (u8 *)memman_alloc_4k(memman, codeLimit);
	set_segmdesc(new_task->ldt + 0, codeLimit - 1, (int) code_seg, AR_CODE32_ER + 0x60);
	phys_copy(code_seg,codeBase,codeLimit);
	u8 *baseAdd = (int *)codeBase;
	//debug("src data[0] = %d,src data[1] = %d,src data[2] = %d,src data[3] = %d",baseAdd[0],baseAdd[1],baseAdd[2],baseAdd[3]);
	//debug("dst data[0] = %d,dst data[0] = %d,dst data[0] = %d,dst data[0] = %d",code_seg[0],code_seg[1],code_seg[2],code_seg[3]);
	new_task->cs_base = code_seg;
	
	
					
	/* base of T-seg, in bytes */
	//int caller_T_base  = reassembly(ppd->base_high, 24,
	//				ppd->base_mid,  16,
	//				ppd->base_low);
	/* limit of T-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	//int caller_T_limit = reassembly(0, 0,
	//				(ppd->limit_high_attr2 & 0xF), 16,
	//				ppd->limit_low);
	/* size of T-seg, in bytes */
	//int caller_T_size  = ((caller_T_limit + 1) *
	//		      ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
	//		       4096 : 1));

	/* Data & Stack segments */
	//ppd = &proc_table[pid].ldts[INDEX_LDT_RW];
	
	int dataLimit = DESCRIPTOR_LIMIT(pldt[1]), dataBase = DESCRIPTOR_BASE(pldt[1]);
	u8 *data_seg = (u8 *)memman_alloc_4k(memman, dataLimit);
	debug("data segment size = %d, src base add = %d",dataLimit,dataBase);
	set_segmdesc(new_task->ldt + 1, dataLimit - 1, (int) data_seg, AR_DATA32_RW + 0x60);
	phys_copy(code_seg,dataBase,dataLimit);
	new_task->ds_base = data_seg;
	
	
	/* base of D&S-seg, in bytes */
	//int caller_D_S_base  = reassembly(ppd->base_high, 24,
	//				  ppd->base_mid,  16,
	//				  ppd->base_low);
	/* limit of D&S-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	//int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16,
	//				  0, 0,
	//				  ppd->limit_low);
	/* size of D&S-seg, in bytes */
	//int caller_D_S_size  = ((caller_T_limit + 1) *
	//			((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
	//			 4096 : 1));

	/* we don't separate T, D & S segments, so we have: */
	//assert((caller_T_base  == caller_D_S_base ) &&
	//       (caller_T_limit == caller_D_S_limit) &&
	//       (caller_T_size  == caller_D_S_size ));

	/* base of child proc, T, D & S segments share the same space,
	   so we allocate memory just once */
	//int child_base = alloc_mem(child_pid, caller_T_size);
	/* int child_limit = caller_T_limit; */
	//printl("{MM} 0x%x <- 0x%x (0x%x bytes)\n",
	//       child_base, caller_T_base, caller_T_size);
	/* child is a copy of the parent */
	//phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);

	/* child's LDT */
	//init_desc(&p->ldts[INDEX_LDT_C],
	//	  child_base,
	//	  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
	//	  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	//init_desc(&p->ldts[INDEX_LDT_RW],
	//	  child_base,
	//	  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
	//	  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	/* tell FS, see fs_fork() */
	//MESSAGE msg2fs;
	//msg2fs.type = FORK;
	//msg2fs.PID = child_pid;
	//send_recv(BOTH, TASK_FS, &msg2fs);
	
	struct file_desc **filp_parent = task_parent->filp, **filp_new = new_task->filp;
	int i;
	for (i = 0; i < NR_FILES; i++) {
		filp_new[i] = filp_parent[i];
		if (filp_new[i]) {
			filp_new[i]->fd_cnt++;
			filp_new[i]->fd_inode->i_cnt++;
		}
	}

	/* child PID will be returned to the parent proc */
	//mm_msg.PID = child_pid;

	/* birth of the child */
	//MESSAGE m;
	//m.type = SYSCALL_RET;
	//m.RETVAL = 0;
	//m.PID = 0;
	//send_recv(SEND, child_pid, &m);

	return new_task;
}

