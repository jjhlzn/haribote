/* bootpack偺儊僀儞 */

#include "bootpack.h"
#include "hd.h"
#include <string.h>
#include <stdio.h>

#define KEYCMD_LED		0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);
struct SHEET *open_log_console(struct SHTCTL *shtctl, unsigned int memtotal);
 


/* bmp.nasm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);
unsigned char rgb2pal(int r, int g, int b, int x, int y);


void load_background_pic(char* back_buf, int *fat);
static struct BOOTINFO *bootinfo = (struct BOOTINFO *) ADR_BOOTINFO;
struct SHEET  *log_win = 0;
void HariMain(void)
{
	
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct SHTCTL *shtctl;
	char s[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256];
	struct SHEET *sht_back, *sht_mouse;
	struct TASK *task_a, *task;
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;  //mmx为负数的时候，窗口不处于窗口移动模式
	struct SHEET *sht = 0, *key_win, *temp_sheet;
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];
	
	//初始化gdt和idt
	init_gdtidt();
	init_pic();

	io_sti(); /* IDT/PIC偺弶婜壔偑廔傢偭偨偺偱CPU偺妱傝崬傒嬛巭傪夝彍 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); /* PIT偲PIC1偲僉乕儃乕僪傪嫋壜(11111000) */
	io_out8(PIC1_IMR, 0xaf); /* 儅僂僗傪嫋壜(10101111), 同时打开硬盘中断  */ 
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	//初始化硬盘
	init_hd(&fifo);
	
	//初始化文件系统
	init_fs();
	
	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman); //task_a是干什么用的？？？ task_a是不是代表内核所在的任务
	fifo.task = task_a;
	strcpy(task_a->name,"task_a");
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	task_a->langmode = 0;

	/* sht_back */
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 摟柧怓側偟 */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	
	/* sht_cons */
	key_win = open_console(shtctl, memtotal);
	log_win = open_log_console(shtctl,memtotal);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 夋柺拞墰偵側傞傛偆偵嵗昗寁嶼 */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back,  0,  0);
	sheet_slide(key_win,   20, 4);
	sheet_slide(log_win,   520, 4);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(log_win,   1);
	sheet_updown(key_win,   2);
	sheet_updown(sht_mouse, 3);
	keywin_on(key_win);

	char *BIOS = (char *)0xf00;
	int *BIOS2 = (int *)0xf00;
	unsigned short cyl =   *(unsigned short *) BIOS; //1023
	unsigned char head =  *(unsigned char *) (2+BIOS); //0
	unsigned short wpcom = *(unsigned short *)(5+BIOS); //65535
	unsigned char ctl =   *(unsigned char *) (8+BIOS);  
	unsigned short lzone = *(unsigned short *) (12+BIOS);
	unsigned char sect = *(unsigned char *) (14+BIOS);    //63
	
	//显示内存信息 
	debug("memory %dMB free : %dKB", memtotal / (1024 * 1024), 
			memman_total(memman) / 1024);
	debug("dd1 = %8x, dd2 = %8x, dd3 = %8x, dd4 = %8x", *BIOS2, *(BIOS2+1), *(BIOS2+2), *(BIOS2+3));
	debug("cyl = %u, head = %u, wpcom = %u ctl = %u, lzone = %u, sect = %u", 
		cyl, head, wpcom,
		ctl, lzone, sect);

	/* 嵟弶偵僉乕儃乕僪忬懺偲偺怘偄堘偄偑側偄傛偆偵丄愝掕偟偰偍偔偙偲偵偡傞 */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/* nihongo.fnt偺撉傒崬傒 */
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	
	//加载壁纸
	//load_background_pic(buf_back, fat);
	//sheet_slide(sht_back,  0,  0); //刷新壁纸
	
	

	finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	} else {
		nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* 僼僅儞僩偑側偐偭偨偺偱敿妏晹暘傪僐僺乕 */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* 僼僅儞僩偑側偐偭偨偺偱慡妏晹暘傪0xff偱杽傔恠偔偡 */
		}
	}
	*((int *) 0x0fe8) = (int) nihongo;
	memman_free_4k(memman, (int) fat, 4 * 2880);

	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* 僉乕儃乕僪僐儞僩儘乕儔偵憲傞僨乕僞偑偁傟偽丄憲傞 */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* FIFO偑偐傜偭傐偵側偭偨偺偱丄曐棷偟偰偄傞昤夋偑偁傟偽幚峴偡傞 */
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (key_win != 0 && key_win->flags == 0) {	/* 僂傿儞僪僂偑暵偠傜傟偨 */
				if (shtctl->top == 1) {	/* 傕偆儅僂僗偲攚宨偟偐側偄 */
					key_win = 0;
				} else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			if (256 <= i && i <= 511) { /* 键盘数据 */
				if (i < 0x80 + 256) { /* 将按键编码转换为字符编码 */
					if (key_shift == 0) {
						s[0] = keytable0[(i - 256)];
					} else {
						s[0] = keytable1[(i - 256)];
					}
				} else {
					s[0] = 0;
				} 
				
				if ('A' <= s[0] && s[0] <= 'Z') {	/* 根据shift键和caps键, 进行小写转换(默认是大写的) */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* 转化为小写 */
					}
				}
				if (s[0] != 0 && key_win != 0) { /* 把字符发送给相应的任务 */
					fifo32_put(&key_win->task->fifo, s[0] + 256);
					if(key_win->task->readKeyboard == 1)
						fifo32_put2(&key_win->task->ch_buf, s[0] + 256);
				}
				if (i == 256 + 0x0f && key_win != 0) {	/* Tab键 */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) {	/* 左Shift ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* 右Shift ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* 左Shift OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* 右Shift OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x3b && key_shift != 0 && key_win != 0) {	/* Shift+F1 */
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {
						cons_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli();	/* 结束任务 */
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
						task_run(task, -1, 0);	/* 廔椆張棟傪妋幚偵傗傜偣傞偨傔偵丄怮偰偄偨傜婲偙偡 */
					}
				}
				if (i == 256 + 0x3c && key_shift != 0) {	/* Shift+F2 */
					/* 打开一个新控制台 */
					if (key_win != 0) {
						keywin_off(key_win);
					}
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
				if (i == 256 + 0x57) {	/* F11 */
					sheet_updown(shtctl->sheets[1], shtctl->top - 1);
				}
				if (i == 256 + 0xfa) {	/* 僉乕儃乕僪偑僨乕僞傪柍帠偵庴偗庢偭偨 */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* 僉乕儃乕僪偑僨乕僞傪柍帠偵庴偗庢傟側偐偭偨 */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			} else if (512 <= i && i <= 767) { /* 鼠标数据 */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* 鼠标指针移动 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					new_mx = mx;
					new_my = my;
						
					if ((mdec.btn & 0x01) != 0) {
					
						/* 嵍儃僞儞傪墴偟偰偄傞 */
						if (mmx < 0) {
							/* 捠忢儌乕僪偺応崌 */
							/* 忋偺壓偠偒偐傜弴斣偵儅僂僗偑巜偟偰偄傞壓偠偒傪扵偡 */
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	/* 僂傿儞僪僂堏摦儌乕僪傊 */
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											/* 点击"x"按钮 */
											if ((sht->flags & 0x10) != 0) {		/* 该窗口是否为应用程序窗口？ */
												task = sht->task;
												
												io_cli();	/* 强制结束处理中禁止切换任务 */
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
												task_run(task, -1, 0);
												
											} else {	/* 僐儞僜乕儖 */
												task = sht->task;
												sheet_updown(sht, -1); /* 偲傝偁偊偢旕昞帵偵偟偰偍偔 */
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							/* 僂傿儞僪僂堏摦儌乕僪偺応崌 */
							x = mx - mmx;	/* 儅僂僗偺堏摦検傪寁嶼 */
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;	/* 堏摦屻偺嵗昗偵峏怴 */
						}
					} else {
						/* 嵍儃僞儞傪墴偟偰偄側偄 */
						mmx = -1;	/* 捠忢儌乕僪傊 */
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	/* 堦搙妋掕偝偣傞 */
							new_wx = 0x7fffffff;
						}
					}
					
					//发送鼠标移动数据给对应的任务
					for (j = shtctl->top - 1; j > 0; j--) {
						struct SHEET *temp_sheet2;
						temp_sheet2 = shtctl->sheets[j];
						x = mx - temp_sheet2->vx0;
						y = my - temp_sheet2->vy0;
						if (0 <= x && x < temp_sheet2->bxsize && 0 <= y && y < temp_sheet2->bysize) {
							if(temp_sheet2->task !=0 && temp_sheet2->task != task_a && temp_sheet2->task->sendMouse){
								/*将鼠标的信息(x,y,btn)编码到32位的int中，其中，btn需要3位(其实只要3位），
								  x为14位，y为14位, 最高位是鼠标信息的标志位 */
								int x1 = x << 17;
								int y1 = y << 3;
								unsigned int code = 0x80000000 + x1 + y1 + mdec.btn;
								fifo32_put( &(temp_sheet2->task->fifo), code);
							}
						}
					}
				}
			} else if (768 <= i && i <= 1023) {	/* 僐儞僜乕儖廔椆張棟 */
				close_console(shtctl->sheets0 + (i - 768));
			} else if (1024 <= i && i <= 2023) {
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) {	/* 僐儞僜乕儖偩偗傪暵偠傞 */
				temp_sheet = shtctl->sheets0 + (i - 2024);
				memman_free_4k(memman, (int) temp_sheet->buf, 256 * 165);
				sheet_free(temp_sheet);
			} else if( i == 3000){   //硬盘中断
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3); /* 僐儞僜乕儖偺僇乕僜儖OFF */
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2); /* 僐儞僜乕儖偺僇乕僜儖ON */
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *) (task->tss.esp + 4)) = (int) sht;
	*((int *) (task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}

struct TASK *open_log_task(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &log_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *) (task->tss.esp + 4)) = (int) sht;
	*((int *) (task->tss.esp + 8)) = memtotal;
	strcpy(task->name,"log");
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}


struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, CONSOLE_WIDTH * CONSOLE_HEIGHT); //长、宽都扩大一倍
	sheet_setbuf(sht, buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, -1); /* 摟柧怓側偟 */
	make_window8(buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, "console", 0);
	make_textbox8(sht, 8, 28, CONSOLE_CONTENT_WIDTH, CONSOLE_CONENT_HEIGHT, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20;	/* 僇乕僜儖偁傝 */
	return sht;
}

struct SHEET *open_log_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, CONSOLE_WIDTH * CONSOLE_HEIGHT); //长、宽都扩大一倍
	sheet_setbuf(sht, buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, -1); /* 摟柧怓側偟 */
	make_window8(buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, "log", 0);
	make_textbox8(sht, 8, 28, CONSOLE_CONTENT_WIDTH, CONSOLE_CONENT_HEIGHT, COL8_000000);
	sht->task = open_log_task(sht, memtotal);
	sht->flags |= 0x20;	/* 僇乕僜儖偁傝 */
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task->flags = 0; /* task_free(task); 偺戙傢傝 */
	return;
}

void close_console(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int) sht->buf, 256 * 165);
	sheet_free(sht);
	close_constask(task);
	return;
}







