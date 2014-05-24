/* 僐儞僜乕儖娭學 */

#include "bootpack.h"
#include "window.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include <fcntl.h>
#include "linkedlist.h"
#include "elf.h"

int do_rdwt(MESSAGE * msg,struct TASK *pcaller);
void print_identify_info(u16* hdinfo, char* str);
u16* hd_identify(int drive);
void partition(int device, int style);
static void console_loop(struct TASK *task, int memtotal, char *last_cmdline);
static void init_cons_buf(struct CONSOLE *cons);
static void buf_putchar(struct CONSOLE *cons, int chr, char move);
static void buf_newline(struct CONSOLE *cons);
static void cons_newline2(struct CONSOLE *cons);
void cons_key_up(struct CONSOLE *cons);
void cmd_testhd();
void cons_key_down(struct CONSOLE *cons);
static void cons_key_down0(struct CONSOLE *cons, int lines);
static void cons_scroll_buttom(struct CONSOLE *cons);
void cmd_cp0(struct CONSOLE *cons, char *cmdline, int *fat);
void print_page_tables();

void update_scroll_bar(struct CONSOLE *cons);

int *fat;
struct FIFO32* log_fifo_buffer = 0;
extern struct SHTCTL *SHEET_CTRL;

static struct CONSOLE *cons_debug;
static int lineNo = 1;

char *log_buf;
int log_ready = 0;
struct LogBufferMgr *log_buf_mgr = NULL;

int has_console_buf = 0;

void log_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int i;
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[CONSOLE_WIDTH_COLS];
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);
    
	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;
	init_cons_buf(&cons);
	//if (cons.sht != 0) {
	//	cons.timer = timer_alloc();
	//	timer_init(cons.timer, &task->fifo, 1);
	//	timer_settime(cons.timer, 50);
	//}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 枹巊梡儅乕僋 */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 擔杮岅僼僅儞僩僼傽僀儖傪撉傒崬傔偨偐丠 */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;
	
	
	//初始化log缓冲区
	int char_count = 4096;
	int *log_fifo_buf = (int *)memman_alloc_4k(memman,4 * char_count);
	fifo32_init(log_fifo_buffer,char_count,log_fifo_buf, task);
	struct LogBufferMgr logBufMgr;
	init_logmgr(&logBufMgr);
	log_buf_mgr = &logBufMgr;
	
	/* 日志准备好 */
	log_ready = 1;
	for (;;) {
		io_cli();
		if (fifo32_status(log_fifo_buffer) == 0) {
			//print_on_screen("go to sleep");
			task_sleep(task);
			io_sti();
		} else{
			//char ch;
			char *buf = (char *)fifo32_get(log_fifo_buffer);
			io_sti();

			cons_putstr0(&cons,buf);
			
			/* 释放日志缓冲 */
			io_cli();
			put_log_buf(log_buf_mgr,buf);
			io_sti();
		}
	}
}

void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int i;
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	cons_debug = &cons;
	lineNo = 1;
	struct FILEHANDLE fhandle[8];
	char cmdline[CONSOLE_WIDTH_COLS];
	char last_cmdline[CONSOLE_WIDTH_COLS];
	sprintf(last_cmdline,"");
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	
	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;
	init_cons_buf(&cons);
	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 枹巊梡儅乕僋 */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 擔杮岅僼僅儞僩僼傽僀儖傪撉傒崬傔偨偐丠 */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* 僾儘儞僾僩昞帵 */
	cons_putchar(&cons, '>', 1);
	
	console_loop(task,memtotal,last_cmdline);
}


////初始化控制台的显示缓冲
static void init_cons_buf(struct CONSOLE *cons)
{	
	struct SHEET *sht_buf = sheet_alloc(SHEET_CTRL);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, CONSOLE_BUF_WIDTH * CONSOLE_BUF_HEIGHT);
	memset(buf,0,CONSOLE_BUF_WIDTH * CONSOLE_BUF_HEIGHT);
	sheet_setbuf(sht_buf, buf, CONSOLE_BUF_WIDTH, CONSOLE_BUF_HEIGHT, -1);
	cons->sht_buf = sht_buf;
	//cons->sht_buf = cons->sht;
	cons->buf_x =  8;
	cons->buf_y = 28;
	cons->buf_cur_y = cons->buf_y;
}


static void console_loop(struct TASK *task, int memtotal, char *last_cmdline)
{
	struct CONSOLE *cons = task->cons;
	char *cmdline = task->cmdline;
	int i;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* 光标用定时器 */
				if (i != 0) {
					timer_init(cons->timer, &task->fifo, 0); /* 下次置位0 */
					if (cons->cur_c >= 0) {
						cons->cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons->timer, &task->fifo, 1); /* 下次置位1 */
					if (cons->cur_c >= 0) {
						cons->cur_c = COL8_000000;
					}
				}
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* 光标ON */
				cons->cur_c = COL8_FFFFFF;  //白色
			}
			if (i == 3) {	/* 光标OFF */
				if (cons->sht != 0) {
					boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000,       //黑色
							 cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
				}
				cons->cur_c = -1;
			}
			if (i == 4) {	/* 僐儞僜乕儖偺乽亊乿儃僞儞僋儕僢僋 */
				cmd_exit(cons, fat);
			}
			if (256 <= i && i <= 511) { /* 键盘数据 */
				//debug("i = %d",i);
				//if (i ==  56  + 256 ) {//向上方向键
				//	cons->cur_x = 16;
				//	sprintf(cmdline,last_cmdline);
				//	cons_putstr0(cons,cmdline);
				//	continue;
				//}
				if (i == 8 + 256) {  //backsapce
					if (cons->cur_x > 16) {
						/* 将当前的字符变为空格 */
						cons_putchar(cons, ' ', 0);
						cons->cur_x -= 8;
						cons->buf_x -= 8;
					}
				} else if (i == 10 + 256) {  //回车
					cons_putchar(cons, ' ', 0);
					cmdline[cons->cur_x / 8 - 2] = 0;
					cons_newline(cons);
					cons_runcmd(cmdline, cons, fat, memtotal);	/* 执行命令应用程序 */
					sprintf(last_cmdline,cmdline);
					if (cons->sht == 0) {
						cmd_exit(cons, fat);
					}
					/* 打印提示符 */
					cons_putchar(cons, '>', 1);
				} else {
					/* 在没有到达边界之前 */
					if (cons->cur_x < CONSOLE_CONTENT_WIDTH) {
						/* 显示输入的字符 */
						cmdline[cons->cur_x / 8 - 2] = i - 256;
						cons_putchar(cons, i - 256, 1);
					}
				}
			}
			if ( i >= 0x80000000 ){
				//debug("i = %x", i);
			}
			
			/* 僇乕僜儖嵞昞帵 */
			if (cons->sht != 0) {
				if(cons->buf_y == cons->buf_cur_y){
					//print_on_screen3("yes,here");
					if (cons->cur_c >= 0) {
						boxfill8(cons->sht->buf, cons->sht->bxsize, cons->cur_c, 
								 cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
					}
					sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 8, cons->cur_y + 16);
				}
			}
		}
	}
}

////往控制台中输入字符
//参数：cons -- 控制台结构，chr -- 显示的字符，move -- 是否移动当前字符
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	buf_putchar(cons,chr,move);
	
	char s[2];
	s[0] = chr;
	s[1] = 0;
	
	if (s[0] == 0x09) {	   /* TAB */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + CONSOLE_CONTENT_WIDTH) { //到达行末尾
				cons_newline2(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32偱妱傝愗傟偨傜break */
			}
		}
	} else if (s[0] == 0x0a) {	/* 换行 */
		cons_newline2(cons);
	} else if (s[0] == 0x0d) {	/* 回车 */
		/* nothing */
	} else {	/* 常规字符 */
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {  //移动当前位置
			cons->cur_x += 8;
			if (cons->cur_x == 8 + CONSOLE_CONTENT_WIDTH) {
				cons_newline2(cons);
			}
		}
	}
	
	return;
}

static void buf_putchar(struct CONSOLE *cons, int chr, char move)
{
	/* 没有屏幕缓冲 */
	if(!has_console_buf)
		return;
	
	//有输出的情况下，先滚动屏幕到最底部
	cons_scroll_buttom(cons);
	
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {	   /* TAB */
		for (;;) {
			if (cons->sht_buf != 0) {
				putfonts8_asc_sht(cons->sht_buf, cons->buf_x, cons->buf_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->buf_x += 8;
			if (cons->buf_x == 8 + CONSOLE_BUF_CONTENT_WIDTH) { //到达行末尾
				buf_newline(cons);
			}
			if (((cons->buf_x - 8) & 0x1f) == 0) {
				break;	/* 32偱妱傝愗傟偨傜break */
			}
		}
	} else if (s[0] == 0x0a) {	/* 换行 */
		buf_newline(cons);
	} else if (s[0] == 0x0d) {	/* 回车 */
		/* nothing */
	} else {	/* 常规字符 */
		if (cons->sht_buf != 0) {
			putfonts8_asc_sht(cons->sht_buf, cons->buf_x, cons->buf_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {  //移动当前位置
			cons->buf_x += 8;
			if (cons->buf_x == 8 + CONSOLE_BUF_CONTENT_WIDTH) {
				buf_newline(cons);
			}
		}
	}
	
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	buf_newline(cons);
	cons_newline2(cons);
	return;
}
void cons_newline2(struct CONSOLE *cons)
{
	update_scroll_bar(cons);
	if(cons == cons_debug){
		lineNo++;
		//print_on_screen3("line %d",lineNo);
	}
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	if (cons->cur_y < 28 + CONSOLE_CONENT_HEIGHT -16) {
		cons->cur_y += 16; 
	}else {               //滚动控制台屏幕
		if (sheet != 0) {
			for (y = 28; y < 28 + CONSOLE_CONENT_HEIGHT - 16; y++) {
				for (x = 8; x < 8 + CONSOLE_CONTENT_WIDTH; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + CONSOLE_CONENT_HEIGHT - 16; y < 28 + CONSOLE_CONENT_HEIGHT; y++) {
				for (x = 8; x < 8 + CONSOLE_CONTENT_WIDTH; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sheet, 8, 28, 8 + CONSOLE_CONTENT_WIDTH, 28 + CONSOLE_CONENT_HEIGHT);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}
static void buf_newline(struct CONSOLE *cons)
{
	/* 没有屏幕缓冲 */
	if(!has_console_buf)
		return;
	
	//有输出的情况下，先滚动屏幕到最底部
	cons_scroll_buttom(cons);
	
	int x, y;
	struct SHEET *sheet = cons->sht_buf;
	struct TASK *task = task_now();

	if (cons->buf_y < 28 + CONSOLE_BUF_CONTENT_HEIGHT - 16) {
		cons->buf_y += 16; 
		cons->buf_cur_y += 16;
	}else {               //滚动控制台屏幕
		//print_on_screen("buf newline");
		if (sheet != 0) {
			for (y = 28; y < 28 + CONSOLE_BUF_CONTENT_HEIGHT - 16; y++) {
				for (x = 8; x < 8 + CONSOLE_BUF_CONTENT_WIDTH; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + CONSOLE_BUF_CONTENT_HEIGHT - 16; y < 28 + CONSOLE_BUF_CONTENT_HEIGHT; y++) {
				for (x = 8; x < 8 + CONSOLE_BUF_WIDTH; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
		}
	}
	cons->buf_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->buf_x = 16;
	}
	return;
}


////向上翻滚
void cons_key_up(struct CONSOLE *cons)
{
	//是否已经到达顶短
	if(cons->buf_cur_y <= 28 + CONSOLE_CONENT_HEIGHT - 16){
		//print_on_screen3("key_up:reach top, buf_cur_y = %d",cons->buf_cur_y);
		return;
	}
	//print_on_screen3("key_up: buf_cur_y = %d",cons->buf_cur_y);
	cons->buf_cur_y -= 16;
	//print_on_screen3("key_up: buf_cur_y = %d",cons->buf_cur_y);
	//把buf的数据拷贝到cons->sht中
	int x, y;
	int buf_y = cons->buf_cur_y-(CONSOLE_CONENT_HEIGHT-16); //从buf_y的y轴出开始拷贝缓冲
	struct SHEET *sheet = cons->sht;
	struct SHEET *cons_buf = cons->sht_buf;
	for (y = 28; y < 28 + CONSOLE_CONENT_HEIGHT; y++, buf_y++) {
		//print_on_screen3("copy %d",y);
		for (x = 8; x < 8 + CONSOLE_CONTENT_WIDTH; x++) {	
			sheet->buf[x + y * sheet->bxsize] = cons_buf->buf[x + buf_y * cons_buf->bxsize];
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + CONSOLE_CONTENT_WIDTH, 28 + CONSOLE_CONENT_HEIGHT);
	update_scroll_bar(cons);
}

#define MAX(x,y) ((x)>(y) ? (x) : (y))
void update_scroll_bar(struct CONSOLE *cons)
{
	int content_height = CONSOLE_CONENT_HEIGHT;
	//int bar_height = CONSOLE_SCROLL_BAR_HEIGHT;
	
	int y0 =( MAX(content_height,(cons->buf_cur_y-28+16)) - content_height) * content_height / CONSOLE_BUF_CONTENT_HEIGHT;
	//print_on_screen3("lineno = %d, lines = %d, ch = %d, bh = %d, scroll_bar_y = %d",lineNo, lines, content_height, bar_height, y0);
	int x, y;
	//重新刷新滚动条背景
	for(y = 28; y < 28 + CONSOLE_CONENT_HEIGHT + 2; y++){
		for(x = 8 + CONSOLE_CONTENT_WIDTH + 4; x < 8 + CONSOLE_CONTENT_WIDTH + 4 + CONSOLE_SCROLL_BAR_WIDTH + 2; x++){
			cons->sht->buf[x + y * cons->sht->bxsize] = COL8_C6C6C6;
		}
	}
	//make_scroll_bar(sht,8 + CONSOLE_CONTENT_WIDTH + 4, 28, CONSOLE_SCROLL_BAR_WIDTH, CONSOLE_SCROLL_BAR_HEIGHT, COL8_000000);
	make_scroll_bar(cons->sht,8 + CONSOLE_CONTENT_WIDTH + 4, 28+y0, CONSOLE_SCROLL_BAR_WIDTH, CONSOLE_SCROLL_BAR_HEIGHT, COL8_000000);
	  sheet_refresh(cons->sht,8 + CONSOLE_CONTENT_WIDTH + 4, 28, 8 + CONSOLE_CONTENT_WIDTH + 4 + CONSOLE_SCROLL_BAR_WIDTH + 2, 28 + CONSOLE_CONENT_HEIGHT);
}

////向下翻滚一行
void cons_key_down(struct CONSOLE *cons)
{
	cons_key_down0(cons,1);
}

static void cons_scroll_buttom(struct CONSOLE *cons)
{
	if(cons->buf_y != cons->buf_cur_y){
		int lines = (cons->buf_y - cons->buf_cur_y) / 16;
		assert(lines > 0);
		cons_key_down0(cons,lines);
	}
	update_scroll_bar(cons);
}

static void cons_key_down0(struct CONSOLE *cons, int lines)
{
	//char msg[100];
	//是否已经到达低断
	if(cons->buf_cur_y >= cons->buf_y){
		//sprintf(msg,"reach button: buf_cur_y = %d, buf_y = %d",cons->buf_cur_y,cons->buf_y);
		//print_on_screen(msg);
		return;
	}
	cons->buf_cur_y += 16 * lines;
	//sprintf(msg,"key_down: buf_cur_y = %d",cons->buf_cur_y);
	//print_on_screen(msg);
	//把buf的数据拷贝到cons->sht中
	int x, y;
	int buf_y = cons->buf_cur_y-(CONSOLE_CONENT_HEIGHT-16); //从buf_y的y轴出开始拷贝缓冲
	struct SHEET *sheet = cons->sht;
	struct SHEET *cons_buf = cons->sht_buf;
	for (y = 28; y < 28 + CONSOLE_CONENT_HEIGHT; y++,buf_y++) {
		for (x = 8; x < 8 + CONSOLE_CONTENT_WIDTH; x++) {
			sheet->buf[x + y * sheet->bxsize] = cons_buf->buf[x + buf_y * cons_buf->bxsize];
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + CONSOLE_CONTENT_WIDTH, 28 + CONSOLE_CONENT_HEIGHT);
	update_scroll_bar(cons);
}


void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (strcmp(cmdline, "hd") == 0 && cons->sht != 0){
		cmd_hd(cons);
	} else if (strcmp(cmdline, "testhd") == 0) {
		cmd_testhd();
	}else if (strcmp(cmdline, "hdpartition") == 0 && cons->sht != 0){
		cmd_partition(cons);
	} else if(strcmp(cmdline, "ls") == 0){
		cmd_ls(cons);
	} else if(strcmp(cmdline, "ps") == 0) {
		cmd_ps(cons);
	} else if(strcmp(cmdline, "page") == 0){
		print_page_config();
	} else if(strncmp(cmdline, "cp0 ", 4) == 0){               //从软盘映像中拷贝文件到硬盘的特定目录
	   cmd_cp0(cons,cmdline, fat);
	} else if(strcmp(cmdline, "pt") == 0){
		print_page_tables();
	}else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* 无法找到该命令和文件 */
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

int sys_open(const char * filename,int flag,int mode);
int sys_sync(void); 
int sys_close(unsigned int fd);
void
cmd_cp0(struct CONSOLE *cons, char *cmdline, int *fat)
{
	//获取文件名
	int i;
	for(i=4; i<strlen(cmdline); i++)
		if(cmdline[i] != ' ')
			break;
	char *filename = cmdline + i;
	
	//从软盘映像中查找该文件
	char src_path[100];
	memset(src_path, 0 , 100);
	sprintf(src_path,"%s",filename);
	struct FILEINFO * finfo = file_search(filename, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if(finfo == NULL){
		char msg[200];
		sprintf(msg,"can't find file %s!\n",src_path);
		cons_putstr0(cons, msg);
		return;
	}
	int appsiz = finfo->size;
	char *buf = file_loadfile2(finfo->clustno, &appsiz, fat); 
	
	//拷贝到硬盘的文件中
	char dest_path[100];
	sprintf(dest_path,"/usr/root/%s",filename);
	int fd = sys_open(dest_path, O_CREAT | O_TRUNC | O_RDWR, 0x777);
	if(fd<0){
		return;
	}
	int n = sys_write(fd, buf, finfo->size);
	if(n != finfo->size){
		debug("write failed!");
		goto err;
	}
	sys_sync();
	
err:
	sys_close(fd);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memman_free(memman,(unsigned int)finfo,sizeof(struct FILEINFO));
	memman_free(memman,(unsigned int)buf,finfo->size);
	
	char msg[200];
	sprintf(msg,"copy file %s success!\n",dest_path);
	cons_putstr0(cons, msg);
	return;
}

void cmd_partition(struct CONSOLE *cons)
{
	hd_identify(0);
	partition( 0 * (4 + 1), 0);
	char str[600];
	sprintf(str,"");
	print_hdinfo(str);
	cons_putstr0(cons, str);
}


void cmd_ls(struct CONSOLE *cons){
	//获取文件列表
	int dev = ROOT_DEV;
	debug("dev = %d",dev);
	struct FILEINFO *p_file = (struct FILEINFO*)get_all_files(dev);
	if(p_file == NULL){
		panic("can't find any files!");
	}
	char str[100];
	int size = 0;
	while(p_file -> size != -1){
		sprintf(str,"%12s   %5d\n",p_file->name,p_file->size);
		cons_putstr0(cons,str);
		p_file++;
		size++;
	}
	//释放p_file
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memman_free(memman,(unsigned int)p_file,(size+1) * sizeof(struct FILEINFO));
}


void cmd_hd(struct CONSOLE *cons)
{
	u16* hdinfo = hd_identify(0);
	//hdinfo = (u16*)0x315f6c;
	//debug("hdinfo = 0x%x",(int)hdinfo);
	//int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	//debug( "HD size: %dMB\n", sectors * 512 / 1000000);
	char str[300];
	print_identify_info((u16*)hdinfo,str);
	cons_putstr0(cons, str);
}

int sys_setup(void * BIOS);
void cmd_testhd()
{
	sys_setup(NULL);
}


void print_identify_info(u16* hdinfo, char* str)
{
	int i, k;
	char s[64];
	
	struct iden_info_ascii {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
				 {27, 40, "HD Model"} /* Model number in ASCII */ };
	
	sprintf(str,"");
	
	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		sprintf(str+strlen(str), "%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	sprintf(str+strlen(str), "LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmd_set_supported = hdinfo[83];
	sprintf(str+strlen(str), "LBA48 supported: %s\n",
	       (cmd_set_supported & 0x0400) ? "Yes" : "No");
	
	int wNumCyls = hdinfo[1];
	sprintf(str+strlen(str),"cyl: %d\n",wNumCyls);
	
	int wReserved2 = hdinfo[2];
	sprintf(str+strlen(str),"wReserved2: %d\n",wReserved2);
	
	int wNumHeads = hdinfo[3];
	sprintf(str+strlen(str),"head: %d\n", wNumHeads);
	
	int wReserved4 = hdinfo[4];
	sprintf(str+strlen(str),"wReserved4: %d\n",wReserved4);
	
	int wReserved5 = hdinfo[5];
	sprintf(str+strlen(str),"wReserved5: %d\n",wReserved5);
	
	int wNumSectorsPerTrack = hdinfo[6];
	sprintf(str+strlen(str),"sectors per track: %d\n", wNumSectorsPerTrack);
	
	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	sprintf(str+strlen(str), "HD size: %dMB\n", sectors * 512 / 1000000);
}

void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	return;
}

void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}

void cmd_dir(struct CONSOLE *cons)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();
	if (cons->sht != 0) {
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);	/* 768乣1023 */
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024乣2023 */
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	/* 僐儅儞僪儔僀儞偵擖椡偝傟偨暥帤楍傪丄堦暥帤偢偮怴偟偄僐儞僜乕儖偵擖椡 */
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	/* 僐儅儞僪儔僀儞偵擖椡偝傟偨暥帤楍傪丄堦暥帤偢偮怴偟偄僐儞僜乕儖偵擖椡 */
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}


char *TASK_STATUS_MSG[] = {
	"UNUSED",
	"SLEEP",
	"RUNNING",
	"HANGING(ZOMBIE)",
	"WAITING",
	"UNINTERRUPTIBLE"
};

void cmd_ps(struct CONSOLE *cons){
	char msg[200];
	
	sprintf(msg,"%4.4s    %10.10s    %16.16s\n", "PID", "NAME", "STATUS");
	cons_putstr0(cons, msg);
	
	struct Node *head = get_all_running_tasks();
	struct Node *tmp = NULL;
	struct TASK *task = NULL;
	
	while(head != NULL){
		task = (struct TASK *)(head->data);
		sprintf(msg, "%4.4d    %10.10s    %16.16s\n", task->pid, task->name, TASK_STATUS_MSG[task->flags]);
		//debug("pid = %d",task->pid);
		cons_putstr0(cons,msg);
		tmp = head;
		head = head->next;
		
		//释放内存
		FreeNode(tmp);
	}
	cons_newline(cons);
}



int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	return load_app(cons,fat,cmdline);
}
	   

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 堎忢廔椆偝偣傞 */
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 堎忢廔椆偝偣傞 */
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}
