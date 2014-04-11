#include "bootpack.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include "hd.h"
#include "fs.h"
#include "linkedlist.h"
#include "elf.h"

int *linux_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			 int fs, int gs,
			 int es, int ds,
			 int eip, int cs, int eflags, int user_esp, int user_ss)
{
	struct TASK *task = task_now();
	debug("invoke system API: eax = %d, pid = %d", eax, task->pid);
	int ds_base = task->ds_base;
	int cs_base = task->cs_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1 + 9;	/* eax偺師偺斣抧 */
	/* 曐懚偺偨傔偺PUSHAD傪嫮堷偵彂偒姺偊傞 */
	/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
	/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	if (eax == 1) {
		//假如当前进程是通过fork调用创建的，那么可以直接结束这个任务
		if(task->forked == 1){
			debug("pocess[%d, forked] die!", task->pid);
			do_exit(task,0);
		}else{
			return &(task->tss.esp0);
		}
	}else if (eax == 4) {
		int fd = ebx;
		char *buf = (char *)(ecx+ds_base);
		int len = edx;
		
		debug("ebx = %d, ecx = %d, edx = %d",ebx, ecx, edx);
		
		//构建写文件内容的参数
		MESSAGE msg;
		msg.FD = fd;
		msg.BUF = buf;
		msg.CNT = len;
		msg.type = WRITE;
		
		reg[7] = do_rdwt(&msg,task);
		
		debug("write contets(%s) to fd(%d)",buf, fd);
	}
	
	return 0;
}
