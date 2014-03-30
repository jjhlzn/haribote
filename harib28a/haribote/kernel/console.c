/* �R���\�[���֌W */

#include "bootpack.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include "hd.h"
#include "fs.h"
#include "linkedlist.h"

int do_rdwt(MESSAGE * msg,struct TASK *pcaller);
void print_identify_info(u16* hdinfo, char* str);

static int *fat;

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

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* ���g�p�}�[�N */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* ���{��t�H���g�t�@�C����ǂݍ��߂����H */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else{
			i = fifo32_get(&task->fifo);
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

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* ���g�p�}�[�N */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* ���{��t�H���g�t�@�C����ǂݍ��߂����H */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* �v�����v�g�\�� */
	cons_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) { /* �J�[�\���p�^�C�} */
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0); /* ����0�� */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1); /* ����1�� */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {	/* �J�[�\��ON */
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* �J�[�\��OFF */
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {	/* �R���\�[���́u�~�v�{�^���N���b�N */
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) { /* �������� */
				//debug("i = %d",i);
				if (i ==  56  + 256 ) {//���Ϸ����
					cons.cur_x = 16;
					sprintf(cmdline,last_cmdline);
					cons_putstr0(&cons,cmdline);
					continue;
				}
				if (i == 8 + 256) {  //backsapce
					if (cons.cur_x > 16) {
						/* ����ǰ���ַ���Ϊ�ո� */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) { 
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* �R�}���h���s */
					sprintf(last_cmdline,cmdline);
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* �v�����v�g�\�� */
					cons_putchar(&cons, '>', 1);
				} else {
					/* ��ʕ��� */
					if (cons.cur_x < CONSOLE_CONTENT_WIDTH) {
						/* �ꕶ���\�����Ă���A�J�[�\����1�i�߂� */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			if ( i >= 0x80000000 ){
				debug("i = %x", i);
			}
			
			/* �J�[�\���ĕ\�� */
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	
	if (s[0] == 0x09) {	/* TAB */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			
			if (cons->cur_x == 8 + CONSOLE_CONTENT_WIDTH) {
				
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32�Ŋ���؂ꂽ��break */
			}
		}
	} else if (s[0] == 0x0a) {	/* ���� */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* �س� */
		/*  */
	} else {	/* �����ַ� */
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* move��0�̂Ƃ��̓J�[�\����i�߂Ȃ� */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + CONSOLE_CONTENT_WIDTH) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	if (cons->cur_y < 28 + CONSOLE_CONENT_HEIGHT -16) {
		cons->cur_y += 16; /* ���̍s�� */
	} else {
		/* �X�N���[�� */
		if (sheet != 0) {
			for (y = 28; y < 28 + CONSOLE_CONENT_HEIGHT -16; y++) {
				for (x = 8; x < 8 + CONSOLE_CONTENT_WIDTH; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + CONSOLE_CONENT_HEIGHT -16; y < 28 + CONSOLE_CONENT_HEIGHT; y++) {
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
	} else if (strcmp(cmdline, "hdpartition") == 0 && cons->sht != 0){
		cmd_partition(cons);
	} else if(strcmp(cmdline, "ls") == 0){
		cmd_ls(cons);
	} else if(strcmp(cmdline, "ps") == 0) {
		cmd_ps(cons);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* �޷��ҵ���������ļ� */
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

void cmd_partition(struct CONSOLE *cons)
{
	hd_identify(0);
	partition( 0 * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
	char str[600];
	sprintf(str,"");
	print_hdinfo(str);
	cons_putstr0(cons, str);
}


void cmd_ls(struct CONSOLE *cons){
	//��ȡ�ļ��б�
	int dev = ROOT_DEV;
	debug("dev = %d",dev);
	struct FILEINFO *p_file = (struct FILEINFO*)get_all_files(dev);
	char str[100];
	int size = 0;
	while(p_file -> size != -1){
		sprintf(str,"%12s   %5d\n",p_file->name,p_file->size);
		cons_putstr0(cons,str);
		p_file++;
		size++;
	}
	//�ͷ�p_file
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memman_free(memman,(unsigned int)p_file,(size+1) * sizeof(struct FILEINFO));
}


void cmd_hd(struct CONSOLE *cons)
{
	u8* hdinfo = hd_identify(0);
	char str[200];
	print_identify_info((u16*)hdinfo,str);
	cons_putstr0(cons, str);
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
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);	/* 768�`1023 */
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024�`2023 */
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
	/* �R�}���h���C���ɓ��͂��ꂽ��������A�ꕶ�����V�����R���\�[���ɓ��� */
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
	/* �R�}���h���C���ɓ��͂��ꂽ��������A�ꕶ�����V�����R���\�[���ɓ��� */
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

void cmd_ps(struct CONSOLE *cons){
	char msg[200];
	
	sprintf(msg,"%s    %s\n", "PID", "NAME");
	cons_putstr0(cons, msg);
	
	struct Node *head = get_all_running_tasks();
	struct Node *tmp = NULL;
	struct TASK *task = NULL;
	
	while(head != NULL){
		task = (struct TASK *)(head->data);
		sprintf(msg, "%d    %s\n", task->pid, task->name);
		//debug("pid = %d",task->pid);
		cons_putstr0(cons,msg);
		tmp = head;
		head = head->next;
		
		//�ͷ��ڴ�
		FreeNode(tmp);
	}
	cons_newline(cons);
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET *sht;

	/* ��ȡ�����ļ��� */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; 

	/* �����ļ��ڴ����е���Ϣ */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/* ����.hrb��׺������ */
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}
	
	if(finfo == 0){
		debug("can't find file[%s]",name);
	}

	if (finfo != 0) {
		/* �����ļ���Ϣ */
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat); //�����
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz); //�������ݶ�
			task->ds_base = (int) q;
			task->cs_base = (int) p;
			set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			
			debug("start app: %s",name);
			debug("code segment: size = %d, add = %d",appsiz,(int)p);
			debug("data segment: size = %d, add = %d",segsiz,(int)q);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0)); 

			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					/* �A�v�����J�����ςȂ��ɂ����������𔭌� */
					sheet_free(sht);	/* ���� */
				}
			}
			for (i = 0; i < 8; i++) {	/* �N���[�Y���ĂȂ��t�@�C�����N���[�Y */
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/* �t�@�C����������Ȃ������ꍇ */
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
	
	struct TASK *task = task_now();
	debug("invoke system API: edx = %d, pid = %d", edx, task->pid);
	int ds_base = task->ds_base;
	int cs_base = task->cs_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1 + 9;	/* eax�̎��̔Ԓn */
		/* �ۑ��̂��߂�PUSHAD�������ɏ��������� */
		/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
		/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	if (edx == 1) {
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {
		cons_putstr0(cons, (char *) ebx + ds_base);
	} else if (edx == 3) {
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {
		//���統ǰ������ͨ��fork���ô����ģ���ô����ֱ�ӽ����������
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
		sheet_updown(sht, shtctl->top); /* ���̃}�E�X�Ɠ��������ɂȂ�悤�Ɏw��F �}�E�X�͂��̏�ɂȂ� */
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
		ecx &= 0xfffffff0;	/* 16�o�C�g�P�ʂ� */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* ����16�ı����Ŀռ� */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16�o�C�g�P�ʂɐ؂�グ */
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
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	/* FIFO����Ȃ̂ŐQ�đ҂� */
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* �J�[�\���p�^�C�} */
				/* �A�v�����s���̓J�[�\�����o�Ȃ��̂ŁA�������͕\���p��1�𒍕����Ă��� */
				timer_init(cons->timer, &task->fifo, 1); /* ����1�� */
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* �J�[�\��ON */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* �J�[�\��OFF */
				cons->cur_c = -1;
			}
			if (i == 4) {	/* �R���\�[����������� */
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);	/* 2024�`2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256 && i<512) { /* �L�[�{�[�h�f�[�^�i�^�X�NA�o�R�j�Ȃ� */
				reg[7] = i - 256;
				return 0;
			}
		}
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	/* �����L�����Z���L�� */
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
	
			if (i >= 0x80000000) { /* ������� */
				
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
		int fd = do_open(pathname,flags,task);
		debug("open fd(%d)", fd);
		reg[7] = fd;
	} else if(edx == 30){
		int fd = eax;
		debug("close fd(%d)",fd);
		do_close(fd,task);
	} else if(edx == 31){
		int fd = eax;
		char *buf = (char *)(ebx+ds_base);
		int len = ebp;
		
		MESSAGE msg;
		msg.FD = fd;
		msg.BUF = buf;
		msg.CNT = len;
		msg.type = READ;
		
		reg[7] = do_rdwt(&msg,task);
		buf[reg[7]] = 0; //���ý�β��
		
		debug("read contents = [%s]",buf);
		
	} else if(edx == 32){        //api_write
		int fd = eax;
		char *buf = (char *)(ebx+ds_base);
		int len = ebp;
		
		//����д�ļ����ݵĲ���
		MESSAGE msg;
		msg.FD = fd;
		msg.BUF = buf;
		msg.CNT = len;
		msg.type = WRITE;
		
		reg[7] = do_rdwt(&msg,task);
		
		debug("write contets(%s) to fd(%d)",buf, fd);
	} else if(edx == 33){
		int fd = eax;
		reg[7] = task->filp[fd]->fd_inode->i_size;
		debug("filesize of fd[%d] = %d",fd,reg[7]);
	} else if(edx == 34){
		//struct TSS32 {
		//	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3; //esp0��ss0Ϊ����ϵͳ��ջ�κź�ջ��ָ��
		//	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
		//	int es, cs, ss, ds, fs, gs;
		//	int ldtr, iomap;
		//};
		struct TSS32 tss;
		tss.backlink = 0;
		//tss.esp0 = task->tss.esp0;  
		tss.ss0 = 1 * 8;   //�ں�ջѡ����
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
		debug("ss = %d", task->tss.ss);
		struct TASK * new_task = do_fork(task, &tss);
		debug("has create child process[%d]",new_task->pid);
		reg[7] = new_task->pid;
		task_add(new_task);
		//task_run(new_task,new_task->level,new_task->priority);
	} else if(edx == 35){
		reg[7] = task->pid;
		//debug("pid = %d",reg[7]);
	} else if(edx == 36){
		debug("addr = %d",ebx);
		int* add_status = (int *)(ds_base+ebx);
		debug("ds_base = %d, add_status = %d", ds_base, (int)add_status);
		
		int child_pid = do_wait(task, add_status);
		debug("exit_status = %d", *add_status);
		reg[7] = child_pid;
	} else if(edx == 37){
		char *path =  (char *)(ds_base + ebx); 
		char **argv = (char *)(ds_base + ecx);
		
		debug("path = %s", path);
		int *regs_push_by_interrupt = &user_ss + 1 + 8;
		reg[7] = do_exec(path, argv, fat, regs_push_by_interrupt);
	}
	return 0;
}

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* �ُ�I�������� */
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* �ُ�I�������� */
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