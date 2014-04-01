#include "bootpack.h"

/*****************************************************************************
 *				  va2la
 *****************************************************************************/
/**
 * <Ring 0~1> Virtual addr --> Linear addr.
 * 
 * @param pid  PID of the proc whose address is to be calculated.
 * @param va   Virtual address.
 * 
 * @return The linear address for the given virtual address.
 *****************************************************************************/
PUBLIC void* va2la(int pid, void* va)
{
	//struct proc* p = &proc_table[pid];

	//u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	//u32 la = seg_base + (u32)va;

	////if (pid < NR_TASKS + NR_PROCS) {
	////	assert(la == (u32)va);
	////}

	//return (void*)la;
	
	return va;
}


