#include "bootpack.h"
#include <stdio.h>
#include <string.h>
#include <asm/segment.h>

extern struct SHEET  *log_win;
extern struct FIFO32 *log_fifo_buffer;

extern int log_ready;
extern struct LogBufferMgr *log_buf_mgr;
/* bmp.nasm */
struct DLL_STRPICENV {	/* 64KB */
	int work[64 * 1024 / 4];
};

struct RGB {
	unsigned char b, g, r, t;
};

int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);
PRIVATE unsigned char rgb2pal(int r, int g, int b, int x, int y);



PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256], buf2[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	sprintf(buf2,"%c !!panic!! %s", MAG_CH_PANIC, buf);
	print_on_screen(buf2);

	/* should never arrive here */
	//ud2();
	for(;;);
}


PUBLIC void printTSSInfo(struct TSS32 *src)
{
	debug("----------------TSS begin---------------------------");
	debug("es = %d, cs = %d", src->es, src->cs);
	debug("ss = %d, ds = %d", src->ss, src->ds);
	debug("fs = %d, gs = %d", src->fs, src->gs);
	debug("backlink = %d",src->backlink);
	debug("esp0 = %d, ss0 = %d", src->esp0, src->ss0);
	debug("esp1 = %d, ss1 = %d", src->esp1, src->ss1);
	debug("esp2 = %d, ss2 = %d", src->esp2, src->ss2);
	debug("eip = %d, esp= %d", src->eip, src->esp);
	debug("cr3 = %d",src->cr3);
	debug("----------------TSS end  ---------------------------");
}

//是否是异步log
int is_async_log = 0;
int debug_running = 0;

////输出到控制台窗口的调试日志
void debug(const char *fmt, ...)
{
	if(!log_ready)
		return;
	
	va_list arg = (va_list)((char *)(&fmt) + 4);
	/* 同步log */
	if(!is_async_log){  
		char buf[512];
		int len = vsprintf(buf,fmt,arg);
		buf[len++] = '\n';
		buf[len++] = 0;
		int wait_count = 100000;
		while(debug_running && wait_count--){
			//print_on_screen3("other debug is running");
			//nothing
		}
		if(wait_count == 0){
			panic("debug wait timeout: %s",buf);
		}
		debug_running = 1;
		cons_putstr0(log_win->task->cons,buf);
		debug_running = 0;
		return;
	}
	
	//return;
	static int invoke = 0; /* 为了检测debug是否已经被嵌套调用了 */
	invoke++;
	
	/* 申请日志的缓冲 */
	cli();
	char * buf=get_log_buf(log_buf_mgr);
	sti();
	if(buf == NULL){
		print_on_screen3("no log buf");
		invoke--;
		return;
	}
	
	
	int len = vsprintf(buf,fmt,arg);
	buf[len++] = '\n';
	buf[len++] = 0;
	
	if(strlen(buf) + 2 > LOG_ENTRY_SIZE){
		panic("log is too log: %s", buf);
	}
	
	if(invoke > 10){
		panic("debug may invoke recursively: %s",buf);
	}
	
	if(is_async_log){
		if(log_win != 0){
			io_cli();
			//print_on_screen3("utls: add log process[%d,%s]",log_fifo_buffer->task->pid,log_fifo_buffer->task->name);
			int retval = fifo32_put(log_fifo_buffer,(int)buf);
			if(retval == -1){  /* fifo已满 */
				//释放日志缓冲
				put_log_buf(log_buf_mgr,buf);
			}
			io_sti();
		}else{
			/* 释放日志缓冲 */
			io_cli();
			put_log_buf(log_buf_mgr,buf);
			io_sti();
		}
	}

	invoke--;
	return;
}

PUBLIC void spin(char * func_name)
{
	debug("\nspinning in %s ...\n", func_name);
	while (1) {}
}


PUBLIC void string_memory(u8 *mem, int size, char *buf){
	int i = 0;
	for(i = 0; i < size; i++){
		sprintf(buf+strlen(buf),"%2.2x ",(u8)mem[i]);
	}
}


PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	debug("%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
		  MAG_CH_ASSERT,
		  exp, file, base_file, line);

	/**
	 * If assertion fails in a TASK, the system will halt before
	 * printl() returns. If it happens in a USER PROC, printl() will
	 * return like a common routine and arrive here. 
	 * @see sys_printx()
	 * 
	 * We use a forever loop to prevent the proc from going on:
	 */
	spin("assertion_failure()");

	/* should never arrive here */
	//__asm__ __volatile__("ud2");
}



void print_on_screen2(char *msg, int x, int y)
{
	struct BOOTINFO *bootinfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_848484, x, y, x + strlen(msg)*8, y+16);
	putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x, y, COL8_000000, msg);	
}

void print_on_screen(char *msg)
{
	struct BOOTINFO *bootinfo = (struct BOOTINFO *) ADR_BOOTINFO;
	int x0 = 10, y0 = 200;
	static int invoke_count = 0;
	boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_848484, x0, y0 + invoke_count * 16, x0 + strlen(msg)*8, y0 + invoke_count * 16 + 16);
	putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x0, y0 + invoke_count * 16, COL8_000000, msg);	
	invoke_count++;
	
	//没有空间记录日志，则刷新日志，从头开始显示日志
	if(invoke_count == 35){
		invoke_count = 0;
		boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_008484, x0, y0, x0 + 1024, y0 + 768);
		putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x0, y0 + invoke_count * 16, COL8_000000, msg);
	}
}

void print_on_screen3(char *fmt, ...)
{
	int i;
	char buf[512];
	va_list arg = (va_list)((char *)(&fmt) + 4);
	i = vsprintf(buf,fmt,arg);
	
	print_on_screen(buf);
}


void debug_userspace1(char *str, int len)
{
	char msg[1024];
	memset(msg,0,1024);
	int i = 0, ch;
	while( (ch=get_fs_byte(str++)) && i < 1024 && i < len){
		msg[i++] = ch;
	}
	debug("str on userspace:[%s]",msg);
	return;
}

void debug_userspace(char *str)
{
	char msg[1024];
	memset(msg,0,1024);
	int i = 0, ch;
	while( (ch=get_fs_byte(str++)) && i < 1024){
		msg[i++] = ch;
	}
	debug("str on userspace:[%s]",msg);
	return;
}

void get_str_userspace(char *str, char *buf)
{
	int i = 0, ch;
	while( (ch=get_fs_byte(str++)) && i < 1024){
		buf[i++] = ch;
	}
	buf[i] = 0;
	return;
}

void get_str_userspace1(char *str, int len, char *buf)
{
	int i = 0, ch;
	while( (ch=get_fs_byte(str++)) && i < len && i < 1024){
		buf[i++] = ch;
	}
	buf[i] = 0;
	return;
}


void load_background_pic(char* buf_back, int *fat)
{
	struct DLL_STRPICENV env;
	char * filebuf, *p;
	char filename[20] ;
	sprintf(filename, "night.bmp");
	p = filename;
	
	int  i, j, fsize, info[8];
	struct RGB picbuf[300*300], *q;	 //这里的空间不能太大，否则内核的栈不够用了
	//picbuf = memman_alloc((struct MEMMAN *) MEMMAN_ADDR, 1024 * 768 * sizeof(struct RGB));
	if(picbuf == 0)
	{
		return;
	}
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

	char strbuf[50];
	sprintf(strbuf,p);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200-16, 200+8*50, 200+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200-16, COL8_000000, strbuf);		
	
	//查找文件
	struct FILEINFO *finfo = file_search(p,(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224); 
	if( finfo ==0 ) //文件找不到
		return;

	sprintf(strbuf,"found %s, clustno = %d, size = %d", filename, finfo->clustno, finfo->size);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200, 200+8*50, 200+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200, COL8_000000, strbuf);					
	
	fsize = finfo->size;
	//加载文件内容
	filebuf = file_loadfile2(finfo->clustno,&fsize,fat);
	
	/* 检查文件类型*/
	if (info_BMP(&env, info, fsize, filebuf) == 0) {
		/* 不是BMP */
		if (info_JPEG(&env, info, fsize, filebuf) == 0) {
			/*  不是JPEG */
			//api_putstr0("file type unknown.\n");
			//api_end();
			memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, (int)filebuf, fsize);
			return;
		}
	}
	sprintf(strbuf,"load file contents, x = %d, y = %d, info[0] = %d",info[2],info[3],info[0]);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200+16, 200+8*50, 200+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200+16, COL8_000000, strbuf);			
	
	/* 上面其中一个info函数调用成功的话，info中包含以下信息 */
	/*	info[0] : 文件类型 (1:BMP, 2:JPEG) */
	/*	info[1] : 颜色数信息 */
	/*	info[2] : xsize */
	/*	info[3] : ysize */

	if (info[2] > 500 || info[3] > 400) {
		//error("picture too large.\n");
		memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, (int)filebuf, fsize);
		return;
	}

	if (info[0] == 1) {
		i = decode0_BMP (&env, fsize, filebuf, 4, (char *) picbuf, 0);
	} else {
		i = decode0_JPEG(&env, fsize, filebuf, 4, (char *) picbuf, 0);
	}
	
	sprintf(strbuf,"parse image, i = %d",i);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200+16+16, 200+8*50, 200+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200+16+16, COL8_000000, strbuf);	

	if (i != 0) {
		//error("decode error.\n");
		memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, (int)filebuf, fsize);
		return;
	}
	
	buf_back = buf_back + binfo->scrnx * ( (binfo->scrny - 24) / 2 - info[3] / 2);

	for (i = 0; i < info[3]; i++) {
		p = buf_back + i * binfo->scrnx + binfo->scrnx / 2 - info[2] / 2;
		q = picbuf + i * info[2];
		for (j = 0; j < info[2]; j++) {
			p[j] = rgb2pal(q[j].r, q[j].g, q[j].b, j, i);
		}
	}
	
	memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, (int)filebuf, fsize);
}


PRIVATE unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	x &= 1; /* 嬼悢偐婏悢偐 */
	y &= 1;
	i = table[x + y * 2];	/* 拞娫怓傪嶌傞偨傔偺掕悢 */
	r = (r * 21) / 256;	/* 偙傟偱 0乣20 偵側傞 */
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;	/* 偙傟偱 0乣5 偵側傞 */
	g = (g + i) / 4;
	b = (b + i) / 4;
	return 16 + r + g * 6 + b * 36;
}



