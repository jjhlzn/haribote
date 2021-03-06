#include "bootpack.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
//#include "hd.h"
#include "fs.h"
#include "linkedlist.h"
#include "elf.h"
#include <sys/stat.h>
#include <haribote/sys.h>
#include <termios.h>
#include <asm/segment.h>

extern int sys_open();
extern int* fat;

extern fn_ptr sys_call_table[];
static int _sys_stdio_ioctl(u32 fd, u32 cmd, u32 arg);

int *linux_api2(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			   int fs, int gs,
			   int es, int ds,
			   int eip, int cs, int eflags, int user_esp, int user_ss)
{
	struct TASK *task = task_now();
	debug("syscall(%d): eax = %d, ebx=%d, ecx=%d, edx = %d", task->pid, eax, ebx, ecx, edx);
	int ds_base = task->ds_base;
	int *reg = &eax + 1 + 9;
	
	if(eax == 4 && (ebx==1 || ebx == 2)){
		char *buf = (char *)ecx;
		int len = edx;
		if(len <= 0)
			return 0;
		char msg[2048];
		get_str_userspace1(buf,len,msg);
		msg[len] = 0;
		debug("pid[%d]: here1", task->pid);
		cons_putstr0(task->cons,msg);
		debug("pid[%d]: here2", task->pid);
		debug("msg = [%s]",msg);
		reg[7] = len;
		return 0;
	}
	
	if(eax == 3 && ebx == 0){
		char *buf = (char *)ecx + ds_base;
	    int len = edx;
		reg[7] = read_from_keyboard(buf,len);;
		return 0;
	}
	
	if (eax == 1) {    //exit
		//假如当前进程是通过fork调用创建的，那么可以直接结束这个任务
		if(task->forked == 1){
			debug("pocess[%d, forked] die!", task->pid);
			do_exit(task,0);
		}else{
			return &(task->tss.esp0);
		}
		return 0;
	}
	
	if(eax == 2){
		struct TSS32 tss;
		tss.backlink = 0;
		//tss.esp0 = task->tss.esp0;  
		tss.ss0 = 1 * 8;   //内核栈选择子
		tss.esp1 = 0;
		tss.ss1 = 0;
		tss.esp2 = 0;
		tss.ss2 = 0;
		tss.cr3 = task->tss.cr3;
		tss.eip = eip;
		tss.eflags = eflags;
		tss.eax = 0;
		tss.ecx = ecx;
		tss.edx = edx;
		tss.ebx = ebx;
		tss.esp = user_esp;
		tss.ebp = ebp;
		tss.esi = esi;
		tss.edi = edi;
		tss.es = es;
		tss.cs = cs;
		tss.ss = user_ss;
		tss.ds = ds;
		tss.fs = fs;
		tss.gs = gs;
		debug("eip = %d", eip);
		debug("ss = %d, ds = %d, cs = %d", user_ss, ds, cs);
		struct TASK * new_task = do_fork_elf(&tss);
		debug("has create child process[%d]",new_task->pid);
		reg[7] = new_task->pid;
		debug("new_task->tss.eip = %d",new_task->tss.eip);
		task_add(new_task);
		return 0;
	}
	
	if(eax == 7){  //wait 
		//debug("addr = %d",ebx);
		int* add_status = (int *)(ds_base+ecx);
		//debug("eax = %d, ebx = %d, ecx = %d, edx = %d", eax, ebx,ecx,edx);
		//debug("exit_status = %d", *add_status);
		int child_pid = do_wait(task, add_status);
		debug("exit_status = %d", *add_status);
		reg[7] = child_pid;
		return 0;
	}
	
	if(eax == 11) { //execve
		char *path =  (char *)(ds_base + ebx); 
		char **argv = (char **)(ds_base + ecx);
		
		debug("path = %s", path);
		int *regs_push_by_interrupt = &user_ss + 1 + 8;
		reg[7] = do_exec(path+1, argv, fat, regs_push_by_interrupt); //例如 /hello 要变成 hello
		return 0;
	}
	
	if(eax == 54)  //ioctl
	{
		//if(ebx == 0 || ebx == 1 || ebx == 2){   //STDOUT, STDERR 不带缓冲 
		//	reg[7] = _sys_stdio_ioctl(ebx,ecx,edx);
		//}else{   //其他都带缓冲
		//	reg[7] = -1;
		//}
		_sys_stdio_ioctl(ebx,ecx,edx);
		reg[7] = 0;
		return 0;
	}
	
	int nr_sys_calls = 21;
	
	if(eax < 0 || (eax > nr_sys_calls - 1)){
		debug("syscall number is invalid: %d",eax);
		reg[7] = -1;
		return 0;
	}
	
	if(sys_call_table[eax]){
		debug("sys_call_table[%d]()",eax);
		reg[7] = sys_call_table[eax](ebx,ecx,edx,esi,edi);
	}else{
		debug("syscall is not implemented: %d",eax);
		debug("sys_call_table[%d] = %d", eax, (int)sys_call_table[eax]);
		reg[7] = -1;
	}
	return 0;
}

int *linux_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			 int fs, int gs,
			 int es, int ds,
			 int eip, int cs, int eflags, int user_esp, int user_ss)
{
	struct TASK *task = task_now();
	debug("syscall: eax = %d, pid = %d, eip = %d", eax, task->pid, eip);
	//debug("ds = %d, ss = %d, esp = %d, cs = %d, eflags = %d", ds, user_ss, user_esp, cs, eflags);
	int ds_base = task->ds_base;
	int *reg = &eax + 1 + 9;	/* eax偺師偺斣抧 */
	/* reg寄存器数组 */
	/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
	/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */ 

	if (eax == 1) {    //exit
		//假如当前进程是通过fork调用创建的，那么可以直接结束这个任务
		if(task->forked == 1){
			debug("pocess[%d, forked] die!", task->pid);
			do_exit(task,0);
		}else{
			return &(task->tss.esp0);
		}
	}else if(eax == 2){
		struct TSS32 tss;
		tss.backlink = 0;
		//tss.esp0 = task->tss.esp0;  
		tss.ss0 = 1 * 8;   //内核栈选择子
		tss.esp1 = 0;
		tss.ss1 = 0;
		tss.esp2 = 0;
		tss.ss2 = 0;
		tss.cr3 = task->tss.cr3;
		tss.eip = eip;
		tss.eflags = eflags;
		tss.eax = 0;
		tss.ecx = ecx;
		tss.edx = edx;
		tss.ebx = ebx;
		tss.esp = user_esp;
		tss.ebp = ebp;
		tss.esi = esi;
		tss.edi = edi;
		tss.es = es;
		tss.cs = cs;
		tss.ss = user_ss;
		tss.ds = ds;
		tss.fs = fs;
		tss.gs = gs;
		debug("eip = %d", eip);
		debug("ss = %d, ds = %d, cs = %d", user_ss, ds, cs);
		struct TASK * new_task = do_fork_elf(&tss);
		debug("has create child process[%d]",new_task->pid);
		reg[7] = new_task->pid;
		task_add(new_task);
	}else if(eax == 3){  //read file
		//int fd = ebx;
		//char *buf = (char *)(ecx+ds_base);
		//int len = edx;
		
		//MESSAGE msg;
		//msg.FD = fd;
		//msg.BUF = buf;
		//msg.CNT = len;
		//msg.type = READ;
		
		//reg[7] = do_rdwt(&msg,task);
		//if(reg[7] != -1)
		//	buf[reg[7]] = 0; //设置结尾符
		
		//debug("read %d bytes [%s]", reg[7], buf);
	}else if (eax == 4) {  //write file
		//int fd = ebx;
		//char *buf = (char *)(ecx+ds_base);
		//int len = edx;
		////debug("ebx = %d, ecx = %d, edx = %d",ebx, ecx, edx);
		
		////构建写文件内容的参数
		//MESSAGE msg;
		//msg.FD = fd;
		//msg.BUF = buf;
		//msg.CNT = len;
		//msg.type = WRITE;
		
		//reg[7] = do_rdwt(&msg,task);
		//debug("write %d bytes [%s] to fd(%d)",len,buf, fd);
	}else if(eax == 5){   // open file
		//debug("ebx = %d, ecx = %d, edx = %d", ebx, ecx, edx);
		//char *pathname = (char *) ebx + ds_base;
		//int flags = ecx;
		//debug("open fd: pathname = %s, flags = %d",pathname,flags);
		//int fd = do_open(pathname,flags,task);
		//debug("open fd(%d)", fd);
		//reg[7] = fd;
	}else if(eax == 6){  //close file
		//int fd = ebx;
		//debug("close fd(%d)",fd);
		//reg[7] = do_close(fd,task);
	}else if(eax == 7){  //wait 
		//debug("addr = %d",ebx);
		int* add_status = (int *)(ds_base+ecx);
		debug("eax = %d, ebx = %d, ecx = %d, edx = %d", eax, ebx,ecx,edx);
		//debug("exit_status = %d", *add_status);
		int child_pid = do_wait(task, add_status);
		debug("exit_status = %d", *add_status);
		reg[7] = child_pid;
	}else if(eax == 8){  //create file
		//char *pathname = (char *)ebx+ds_base;
		//int fd = do_open(pathname,O_CREATE,task);
		//debug("create file %s fd[%d]",pathname,task);
		//reg[4] = fd;
	}else if(eax == 20){  //getpid
		debug("pid = %d",task->pid);
		reg[7] = task->pid;
	}
	return 0;
}


int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			 int fs, int gs,
			 int es, int ds,
			 int eip, int cs, int eflags, int user_esp, int user_ss)
{
	//debug("eip = %d, cs = %d", eip, cs & 0xffff);
	//debug("eflags = %d, user_esp = %d, user_ss = %d", eflags, user_esp, user_ss & 0xffff);
	//debug("es = %d, ds = %d", es & 0xffff, ds & 0xffff);
	//debug("fs = %d, gs = %d", fs & 0xffff, gs & 0xffff);
	//debug("ds = %d, ss = %d, esp = %d, cs = %d, eflags = %d", ds, user_ss, user_esp, cs, eflags);
	struct TASK *task = task_now();
	debug("syscall: edx = %d, pid = %d, eip = %d", edx, task->pid, eip);
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	//struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1 + 9;	/* eax偺師偺斣抧 */
	/* 曐懚偺偨傔偺PUSHAD傪嫮堷偵彂偒姺偊傞 */
	/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
	/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;


	if (edx == 1) {
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {
		char * msg = (char *) ebx + ds_base;
		//debug("ebx = %d, ds_base =%d",ebx, ds_base);
		//debug("dispay msg: [%s]",msg);
		
		//char msg1[500];
		//int i;
		//for(i=0; i<500; i++)
		//	msg1[i] = 0;
		//string_memory(ebx + ds_base, 32, msg1);
		//debug(msg1);
		cons_putstr0(cons, msg);
	} else if (edx == 3) {
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {
		//假如当前进程是通过fork调用创建的，那么可以直接结束这个任务
		if(task->forked == 1){
			debug("pocess[%d, forked] die!", task->pid);
			do_exit(task,0);
		}else{
			return &(task->tss.esp0);
		}
	} else if (edx == 5) {
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		sheet_updown(sht, shtctl->top); /* 崱偺儅僂僗偲摨偠崅偝偵側傞傛偆偵巜掕丗 儅僂僗偼偙偺忋偵側傞 */
		reg[7] = (int) sht;
	} else if (edx == 6) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;	/* 16僶僀僩扨埵偵 */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {               //api_malloc
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 分配16的倍数的空间 */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16僶僀僩扨埵偵愗傝忋偘 */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {
		sht = (struct SHEET *) ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
	} else if (edx == 13) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			if (eax > esi) {
				i = eax;
				eax = esi;
				esi = i;
			}
			if (ecx > edi) {
				i = ecx;
				ecx = edi;
				edi = i;
			}
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {
		sheet_free((struct SHEET *) ebx);
	} else if (edx == 15) {
		//reg[7] = read_from_keyboard(task,eax);
		panic("don't support");
		return 0;
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	/* 帺摦僉儍儞僙儖桳岠 */
	} else if (edx == 17) {
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {
		timer_settime((struct TIMER *) ebx, eax);
	} else if (edx == 19) {
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	} else if (edx == 21) {
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char *) ebx + ds_base,
								(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int) fh;
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
		}
	} else if (edx == 22) {
		fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
	} else if (edx == 23) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (edx == 24) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
	} else if (edx == 25) {
		fh = (struct FILEHANDLE *) eax;
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	} else if (edx == 26) {
		i = 0;
		for (;;) {
			*((char *) ebx + ds_base + i) =  task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
	} else if (edx == 27) {
		reg[7] = task->langmode;
	} else if (edx == 28) {
		struct MOUSE_DEC mdec;
		mdec.phase = 0; 
		int mx, my, btn;
		mx =0; my = 0, btn =0;
		task->sendMouse = 1;
		struct MOUSE_INFO * minfo = (struct MOUSE_INFO*)(ebp + ds_base);
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		
		debug("invoke 28 minfo->flag = %d", minfo->flag);
		
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	
				} else {
					io_sti();
					task->sendMouse = 0;
					break;
				}
			}
			i = fifo32_get(&task->fifo);
			debug("i = %x", i);
			io_sti();
			
			if (i >= 0x80000000) { /* 鼠标数据 */
				
				mx = ((unsigned int)i << 1) >> 18;
				my = ((unsigned int)i << 15) >> 18;
				btn = i & 0x00000007;
				
				minfo->x = mx;
				minfo->y = my;
				minfo->btn = btn;
				minfo->flag = 1;
				task->sendMouse = 0;
				break;
			}
		}
	} else if (edx == 29) { //api_open
		char *pathname = (char *) eax + ds_base;
		int flags = ebx;
		debug("open fd: pathname = %s, flags = %d",pathname,flags);
		int fd = sys_open((char *)eax, flags, S_IRWXU);
		//int fd = do_open(pathname,flags,task);
		debug("open fd(%d)", fd);
		reg[7] = fd;
	} else if(edx == 30){   //close file
		//int fd = eax;
		//debug("close fd(%d)",fd);
		//do_close(fd,task);
	} else if(edx == 31){    //read file
		int fd = eax;
		char *buf = (char *)ebx;
		int len = ebp;
		reg[7]= sys_read(fd, (char *)ebx, len);
		
		char contents[2048];
		get_str_userspace(buf,contents);
		debug("read %d bytes: [%s]",reg[7], contents);
		
	} else if(edx == 32){        //api_write
		int fd = eax;
		char *buf = (char *)ebx;
		int len = ebp;
		
		reg[7] = sys_write(fd,buf,len);
		
		char contents[2048];
		get_str_userspace1(buf,len,contents);
		debug("write %d bytes: [%s]", reg[7], contents);
	} else if(edx == 33){
		int fd = eax;
		reg[7] = task->filp[fd]->f_inode->i_size;
		debug("filesize of fd[%d] = %d",fd,reg[7]);
	} else if(edx == 34){
		struct TSS32 tss;
		tss.backlink = 0;
		//tss.esp0 = task->tss.esp0;  
		tss.ss0 = 1 * 8;   //内核栈选择子
		tss.esp1 = 0;
		tss.ss1 = 0;
		tss.esp2 = 0;
		tss.ss2 = 0;
		tss.cr3 = task->tss.cr3;
		tss.eip = eip;
		tss.eflags = eflags;
		tss.eax = 0;
		tss.ecx = ecx;
		tss.edx = edx;
		tss.ebx = ebx;
		tss.esp = user_esp;
		tss.ebp = ebp;
		tss.esi = esi;
		tss.edi = edi;
		tss.es = es;
		tss.cs = cs;
		tss.ss = user_ss;
		tss.ds = ds;
		tss.fs = fs;
		tss.gs = gs;
		debug("eip = %d", eip);
		debug("ss = %d, ds = %d, cs = %d", user_ss, ds, cs);
		struct TASK * new_task = do_fork(&tss);
		debug("has create child process[%d]",new_task->pid);
		reg[7] = new_task->pid;
		task_add(new_task);
	} else if(edx == 35){
		reg[7] = task->pid;
		//debug("pid = %d",reg[7]);
	} else if(edx == 36){   //wait
		//debug("addr = %d",ebx);
		int* add_status = (int *)(ds_base+ebx);
		debug("ds_base = %d, add_status = %d", ds_base, (int)add_status);
		debug("exit_status = %d", *add_status);
		int child_pid = do_wait(task, add_status);
		debug("exit_status = %d", *add_status);
		reg[7] = child_pid;
	} else if(edx == 37){
		char *path =  (char *)(ds_base + ebx); 
		char **argv = (char **)(ds_base + ecx);
		
		debug("path = %s", path);
		int *regs_push_by_interrupt = &user_ss + 1 + 8;
		reg[7] = do_exec(path, argv, fat, regs_push_by_interrupt);
	}
	return 0;
}
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"
struct termios termios_table[] = {

	{	ICRNL,		/* change incoming CR to NL */
		OPOST|ONLCR,	/* change outgoing NL to CRNL */
		0,
		ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
		0,		/* console termio */
		INIT_C_CC}
	
};

static int _sys_stdio_ioctl(u32 fd, u32 cmd, u32 arg)
{
	debug("_sys_stdio_ioctl");
	struct termios * termios = (struct termios *)arg;
	int i;
	//verify_area(termios, sizeof (*termios));
	for (i=0 ; i< (sizeof (*termios)) ; i++)
		put_fs_byte( ((char *)&termios_table[0])[i] , i+(char *)termios );
	return 0;
}
