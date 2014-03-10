/* bootpack�̃��C�� */

#include "bootpack.h"
#include "keyboard.h"
#include "keymap.h"

#include <stdio.h>

#define KEYCMD_LED		0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

struct DLL_STRPICENV {	/* 64KB */
	int work[64 * 1024 / 4];
};

struct RGB {
	unsigned char b, g, r, t;
};

/* bmp.nasm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);
unsigned char rgb2pal(int r, int g, int b, int x, int y);


void load_background_pic(char* back_buf, int *fat);
static struct BOOTINFO *bootinfo = (struct BOOTINFO *) ADR_BOOTINFO;
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
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;  //mmxΪ������ʱ�򣬴��ڲ����ڴ����ƶ�ģʽ
	struct SHEET *sht = 0, *key_win, *temp_sheet;
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];

	char strbuf[50];
	
	init_gdtidt();
	init_pic();

	
	io_sti(); /* IDT/PIC�̏��������I������̂�CPU�̊��荞�݋֎~������ */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); /* PIT��PIC1�ƃL�[�{�[�h������(11111000) */
	io_out8(PIC1_IMR, 0xaf); /* �}�E�X������(10101111), ͬʱ��Ӳ���ж�  */ 
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	sprintf(strbuf,"memory %dMB free : %dKB", memtotal / (1024 * 1024), 
			memman_total(memman) / 1024);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	task_a->langmode = 0;

	/* sht_back */
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* �����F�Ȃ� */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	
	/* sht_cons */
	key_win = open_console(shtctl, memtotal);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* ��ʒ����ɂȂ�悤�ɍ��W�v�Z */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back,  0,  0);
	sheet_slide(key_win,   32, 4);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(key_win,   1);
	sheet_updown(sht_mouse, 2);
	keywin_on(key_win);
	

	char *BIOS = (char *)0xf00;
	int *BIOS2 = (int *)0xf00;
	unsigned short cyl =   *(unsigned short *) BIOS; //1023
	unsigned char head =  *(unsigned char *) (2+BIOS); //0
	unsigned short wpcom = *(unsigned short *)(5+BIOS); //65535
	unsigned char ctl =   *(unsigned char *) (8+BIOS);  
	unsigned short lzone = *(unsigned short *) (12+BIOS);
	unsigned char sect = *(unsigned char *) (14+BIOS);    //63
	
	//��ʾ�ڴ���Ϣ 
	print_on_screen(strbuf);
	
	sprintf(strbuf,"dd1 = %8x, dd2 = %8x, dd3 = %8x, dd4 = %8x", 
		*BIOS2, *(BIOS2+1), *(BIOS2+2), *(BIOS2+3));
	print_on_screen(strbuf);
	
	sprintf(strbuf,"cyl = %u, head = %u, wpcom = %u ctl = %u, lzone = %u, sect = %u", 
		cyl, head, wpcom,
		ctl, lzone, sect);
	print_on_screen(strbuf);
	
	sprintf(strbuf,"hd = %x", binfo->hd0);
	print_on_screen(strbuf);

	/* �ŏ��ɃL�[�{�[�h��ԂƂ̐H���Ⴂ���Ȃ��悤�ɁA�ݒ肵�Ă������Ƃɂ��� */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/* nihongo.fnt�̓ǂݍ��� */
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	
	
	//���ر�ֽ
	//load_background_pic(buf_back, fat);
	//sheet_slide(sht_back,  0,  0); //ˢ�±�ֽ
	init_hd(&fifo);
	init_fs();

	finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	} else {
		nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* �t�H���g���Ȃ������̂Ŕ��p�������R�s�[ */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* �t�H���g���Ȃ������̂őS�p������0xff�Ŗ��ߐs���� */
		}
	}
	*((int *) 0x0fe8) = (int) nihongo;
	memman_free_4k(memman, (int) fat, 4 * 2880);

	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* �L�[�{�[�h�R���g���[���ɑ���f�[�^������΁A���� */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* FIFO��������ۂɂȂ����̂ŁA�ۗ����Ă���`�悪����Ύ��s���� */
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
			if (key_win != 0 && key_win->flags == 0) {	/* �E�B���h�E������ꂽ */
				if (shtctl->top == 1) {	/* �����}�E�X�Ɣw�i�����Ȃ� */
					key_win = 0;
				} else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			if (256 <= i && i <= 511) { /* �������� */
				//sprintf(strbuf,"%d",i);
				//print_on_screen(strbuf);
				
				if (i < 0x80 + 256) { /* �����ַ� */
					if (key_shift == 0) {
						//s[0] = keymap[(i - 256)*MAP_COLS];
						s[0] = keytable0[(i - 256)];
					} else {
						//s[0] = keymap[(i - 256)*MAP_COLS+1];
						s[0] = keytable1[(i - 256)];
					}
				} else {
					s[0] = 0;
				} 
				
				if ('A' <= s[0] && s[0] <= 'Z') {	/* ���͕������A���t�@�x�b�g */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* �啶�����������ɕϊ� */
					}
				}
				if (s[0] != 0 && key_win != 0) { /* ���ַ����͸���Ӧ������ */
					//sprintf(strbuf,"send to task [%d]",s[0] + 256);
					//print_on_screen(strbuf);
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				if (i == 256 + 0x0f && key_win != 0) {	/* Tab */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) {	/* ���V�t�g ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* �E�V�t�g ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* ���V�t�g OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* �E�V�t�g OFF */
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
						io_cli();	/* �������� */
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
						task_run(task, -1, 0);	/* �I���������m���ɂ�点�邽�߂ɁA�Q�Ă�����N���� */
					}
				}
				if (i == 256 + 0x3c && key_shift != 0) {	/* Shift+F2 */
					/* ��һ���¿���̨ */
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
				if (i == 256 + 0xfa) {	/* �L�[�{�[�h���f�[�^�𖳎��Ɏ󂯎���� */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* �L�[�{�[�h���f�[�^�𖳎��Ɏ󂯎��Ȃ����� */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			} else if (512 <= i && i <= 767) { /* ������� */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* ���ָ���ƶ� */
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
					
						/* ���{�^���������Ă��� */
						if (mmx < 0) {
							/* �ʏ탂�[�h�̏ꍇ */
							/* ��̉��������珇�ԂɃ}�E�X���w���Ă��鉺������T�� */
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
											mmx = mx;	/* �E�B���h�E�ړ����[�h�� */
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											/* �u�~�v�{�^���N���b�N */
											if ((sht->flags & 0x10) != 0) {		/* �A�v����������E�B���h�E���H */
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();	/* �����I���������Ƀ^�X�N���ς��ƍ��邩�� */
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											} else {	/* �R���\�[�� */
												task = sht->task;
												sheet_updown(sht, -1); /* �Ƃ肠������\���ɂ��Ă��� */
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
							/* �E�B���h�E�ړ����[�h�̏ꍇ */
							x = mx - mmx;	/* �}�E�X�̈ړ��ʂ��v�Z */
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;	/* �ړ���̍��W�ɍX�V */
						}
					} else {
						/* ���{�^���������Ă��Ȃ� */
						mmx = -1;	/* �ʏ탂�[�h�� */
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	/* ��x�m�肳���� */
							new_wx = 0x7fffffff;
						}
					}
					
					//��������ƶ����ݸ���Ӧ������
					for (j = shtctl->top - 1; j > 0; j--) {
						struct SHEET *temp_sheet2;
						temp_sheet2 = shtctl->sheets[j];
						x = mx - temp_sheet2->vx0;
						y = my - temp_sheet2->vy0;
						if (0 <= x && x < temp_sheet2->bxsize && 0 <= y && y < temp_sheet2->bysize) {
							if(temp_sheet2->task !=0 && temp_sheet2->task != task_a && temp_sheet2->task->sendMouse){
								//��ӡ����ǰ
								//sprintf(strbuf,"send mouse info to task [%d], top = %d", j, shtctl->top);
								//boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 200,200, 200+8*36, 200+16);
								//putfonts8_asc(binfo->vram, binfo->scrnx, 200, 200, COL8_000000, strbuf);
								
								/*��������Ϣ(x,y,btn)���뵽32λ��int�У����У�btn��Ҫ3λ(��ʵֻҪ3λ����
								  xΪ14λ��yΪ14λ, ���λ�������Ϣ�ı�־λ */
								int x1 = x << 17;
								int y1 = y << 3;
								unsigned int code = 0x80000000 + x1 + y1 + mdec.btn;
								fifo32_put( &(temp_sheet2->task->fifo), code);
							}
						}
					}
				}
			} else if (768 <= i && i <= 1023) {	/* �R���\�[���I������ */
				close_console(shtctl->sheets0 + (i - 768));
			} else if (1024 <= i && i <= 2023) {
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) {	/* �R���\�[����������� */
				temp_sheet = shtctl->sheets0 + (i - 2024);
				memman_free_4k(memman, (int) temp_sheet->buf, 256 * 165);
				sheet_free(temp_sheet);
			} else if( i == 3000){   //Ӳ���ж�
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3); /* �R���\�[���̃J�[�\��OFF */
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2); /* �R���\�[���̃J�[�\��ON */
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


struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, CONSOLE_WIDTH * CONSOLE_HEIGHT); //����������һ��
	sheet_setbuf(sht, buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, -1); /* �����F�Ȃ� */
	make_window8(buf, CONSOLE_WIDTH, CONSOLE_HEIGHT, "console", 0);
	make_textbox8(sht, 8, 28, CONSOLE_CONTENT_WIDTH, CONSOLE_CONENT_HEIGHT, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20;	/* �J�[�\������ */
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task->flags = 0; /* task_free(task); �̑��� */
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



unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	x &= 1; /* ��������� */
	y &= 1;
	i = table[x + y * 2];	/* ���ԐF����邽�߂̒萔 */
	r = (r * 21) / 256;	/* ����� 0�`20 �ɂȂ� */
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;	/* ����� 0�`5 �ɂȂ� */
	g = (g + i) / 4;
	b = (b + i) / 4;
	return 16 + r + g * 6 + b * 36;
}

void load_background_pic(char* buf_back, int *fat)
{
	struct DLL_STRPICENV env;
	char * filebuf, *p;
	char filename[20] ;
	sprintf(filename, "night.bmp");
	p = filename;
	
	int win, i, j, fsize, xsize, info[8];
	struct RGB picbuf[300*300], *q;	 //����Ŀռ䲻��̫�󣬷����ں˵�ջ��������
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
	
	//�����ļ�
	struct FILEINFO *finfo = file_search(p,(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224); 
	if( finfo ==0 ) //�ļ��Ҳ���
		return;

	sprintf(strbuf,"found %s, clustno = %d, size = %d", filename, finfo->clustno, finfo->size);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200, 200+8*50, 200+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200, COL8_000000, strbuf);					
	
	fsize = finfo->size;
	//�����ļ�����
	filebuf = file_loadfile2(finfo->clustno,&fsize,fat);
	
	/* ����ļ�����*/
	if (info_BMP(&env, info, fsize, filebuf) == 0) {
		/* ����BMP */
		if (info_JPEG(&env, info, fsize, filebuf) == 0) {
			/*  ����JPEG */
			//api_putstr0("file type unknown.\n");
			//api_end();
			memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, filebuf, fsize);
			return;
		}
	}
	sprintf(strbuf,"load file contents, x = %d, y = %d, info[0] = %d",info[2],info[3],info[0]);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 200+16, 200+8*50, 200+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 200+16, COL8_000000, strbuf);			
	
	/* ��������һ��info�������óɹ��Ļ���info�а���������Ϣ */
	/*	info[0] : �ļ����� (1:BMP, 2:JPEG) */
	/*	info[1] : ��ɫ����Ϣ */
	/*	info[2] : xsize */
	/*	info[3] : ysize */

	if (info[2] > 500 || info[3] > 400) {
		//error("picture too large.\n");
		memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, filebuf, fsize);
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
		memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, filebuf, fsize);
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
	
	memman_free_4k((struct MEMMAN *) MEMMAN_ADDR, filebuf, fsize);
}

void print_on_screen2(char *msg, int x, int y){
	
	boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_848484, x, y, x + strlen(msg)*8, y+16);
	putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x, y, COL8_000000, msg);	
}

void debug(const char *fmt, ...){
	int i;
	char buf[1024];
	
	va_list arg = (va_list)((char *)(&fmt) + 4);
	
	i = vsprintf(buf,fmt,arg);
	print_on_screen(buf);
	
	return;
}

PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	debug("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	ud2();
}


void print_on_screen(char *msg){
	int x0 = 10, y0 = 200;
	static int invoke_count = 0;
	boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_848484, x0, y0 + invoke_count * 16, x0 + strlen(msg)*8, y0 + invoke_count * 16 + 16);
	putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x0, y0 + invoke_count * 16, COL8_000000, msg);	
	invoke_count++;
	
	//û�пռ��¼��־����ˢ����־����ͷ��ʼ��ʾ��־
	if(invoke_count == 35){
		invoke_count = 0;
		boxfill8(bootinfo->vram,bootinfo->scrnx, COL8_008484, x0, y0, x0 + 1024, y0 + 768);
		putfonts8_asc(bootinfo->vram, bootinfo->scrnx, x0, y0 + invoke_count * 16, COL8_000000, msg);
	}
}

PUBLIC void spin(char * func_name)
{
	char str[100];
	sprintf(str,"\nspinning in %s ...\n", func_name);
	print_on_screen(str);
	while (1) {}
}

PUBLIC char * string_memory(u8 *mem, int size, char *buf){
	int i = 0;
	for(i = 0; i < size; i++){
		sprintf(buf+strlen(buf),"%x ",mem[i]);
	}
}

PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	char str[200];
	sprintf(str,"%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
	       MAG_CH_ASSERT,
	       exp, file, base_file, line);
	print_on_screen(str);

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
