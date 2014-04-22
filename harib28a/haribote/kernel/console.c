/* �R���\�[���֌W */

#include "bootpack.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include "hd.h"
#include "fs.h"
#include "linkedlist.h"
#include "elf.h"

int do_rdwt(MESSAGE * msg,struct TASK *pcaller);
void print_identify_info(u16* hdinfo, char* str);

int *fat;
struct FIFO32* log_fifo_buffer = 0;

void log_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int i;
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	log_fifo_buffer = (struct FIFO32 *)memman_alloc_4k(memman, sizeof(struct FIFO32));
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

	//if (cons.sht != 0) {
	//	cons.timer = timer_alloc();
	//	timer_init(cons.timer, &task->fifo, 1);
	//	timer_settime(cons.timer, 50);
	//}
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
	
	
	//��ʼ��log������
	int char_count = 4096;
	int *log_buffer = (int *)memman_alloc_4k(memman,4 * char_count);
	fifo32_init(log_fifo_buffer,char_count,log_buffer, task);

	for (;;) {
		//print_on_screen("log");
		io_cli();
		
		if (fifo32_status(log_fifo_buffer) == 0) {
			//print_on_screen("go to sleep");
			task_sleep(task);
			io_sti();
		} else{
			char ch;
			ch = (char)fifo32_get(log_fifo_buffer);
			io_sti();
			
			cons_putchar(&cons, ch, 1);
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
	
	console_loop(task,memtotal,last_cmdline);
}


void console_loop(struct TASK *task, int memtotal, char *last_cmdline){
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
			if (i <= 1 && cons->sht != 0) { /* �J�[�\���p�^�C�} */
				if (i != 0) {
					timer_init(cons->timer, &task->fifo, 0); /* ����0�� */
					if (cons->cur_c >= 0) {
						cons->cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons->timer, &task->fifo, 1); /* ����1�� */
					if (cons->cur_c >= 0) {
						cons->cur_c = COL8_000000;
					}
				}
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* �J�[�\��ON */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* �J�[�\��OFF */
				if (cons->sht != 0) {
					boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000,
							 cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
				}
				cons->cur_c = -1;
			}
			if (i == 4) {	/* �R���\�[���́u�~�v�{�^���N���b�N */
				cmd_exit(cons, fat);
			}
			if (256 <= i && i <= 511) { /* �������� */
				//debug("i = %d",i);
				if (i ==  56  + 256 ) {//���Ϸ����
					cons->cur_x = 16;
					sprintf(cmdline,last_cmdline);
					cons_putstr0(cons,cmdline);
					continue;
				}
				if (i == 8 + 256) {  //backsapce
					if (cons->cur_x > 16) {
						/* ����ǰ���ַ���Ϊ�ո� */
						cons_putchar(cons, ' ', 0);
						cons->cur_x -= 8;
					}
				} else if (i == 10 + 256) {  //�س�
					cons_putchar(cons, ' ', 0);
					cmdline[cons->cur_x / 8 - 2] = 0;
					cons_newline(cons);
					cons_runcmd(cmdline, cons, fat, memtotal);	/* ִ������Ӧ�ó��� */
					sprintf(last_cmdline,cmdline);
					if (cons->sht == 0) {
						cmd_exit(cons, fat);
					}
					/* ��ӡ��ʾ�� */
					cons_putchar(cons, '>', 1);
				} else {
					/* ��û�е���߽�֮ǰ */
					if (cons->cur_x < CONSOLE_CONTENT_WIDTH) {
						/* ��ʾ������ַ� */
						cmdline[cons->cur_x / 8 - 2] = i - 256;
						cons_putchar(cons, i - 256, 1);
					}
				}
			}
			if ( i >= 0x80000000 ){
				debug("i = %x", i);
			}
			
			/* �J�[�\���ĕ\�� */
			if (cons->sht != 0) {
				if (cons->cur_c >= 0) {
					boxfill8(cons->sht->buf, cons->sht->bxsize, cons->cur_c, 
							 cons->cur_x, cons->cur_y, cons->cur_x + 7, cons->cur_y + 15);
				}
				sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 8, cons->cur_y + 16);
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
	} else if(strcmp(cmdline, "page") == 0){
		print_page_config();
	}else if (cmdline[0] != 0) {
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

char *get_next_arg(char *cmdline, int *skip){
	//debug("1 = %s",cmdline);
	int i=0;
	while(cmdline[i] == ' ') i++;
	//debug("2 = %s",cmdline+i);
	
	if(cmdline[i] != 0){ //���ǽ�β��
		//�Ƶ���β�������Ƶ��¸�����ǰ�Ŀո�
		struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
		char *arg = (char *)memman_alloc_4k(memman,1024);
		int j = 0;
		while(cmdline[i] != ' ' && cmdline[i] != 0){
			arg[j] = cmdline[i];
			++i;
			++j;
		}
		arg[j] = 0;
		//debug("3 = %s",cmdline+i);
		*skip = i;
		return arg;
	}else{
		return NULL;
	}
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
	//open_std_files(task);

	/* ��ȡ�����ļ��� */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; 
	
	//��ȡ������ÿ���������ֻ����1024���ȡ�
	struct Node *list = NULL;
	int count = 0;
	while(1){
		int skip = 0;
		char *arg = get_next_arg(cmdline, &skip);
		if(arg == NULL)
			break;
		else{
			struct Node *node = CreateNode(arg);
			if(list == NULL){
				list = node;
			}else{
				Append(list,node);
			}
			count++;
			cmdline += skip;
		}
	}
	
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
			//debug("esp = %d",esp);
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
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0), 0, 0); 

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
		} else if (appsiz >= sizeof(Elf32_Ehdr) && strncmp(p + 1, "ELF", 3) == 0 && p[0] == 0x7F ) {
			Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)p;
			//debug_Elf32_Ehd(elf_hdr);
			
			int i;
			
			//�����ַ�����
			Elf32_Shdr* str_section = (Elf32_Shdr*)(p + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize);
			char *str_contents = (char *)(p + str_section->sh_offset);
			
			for (i = 0; i<elf_hdr->e_shnum; i++) {
				Elf32_Shdr *elf_shdr = (Elf32_Shdr*)(p + elf_hdr->e_shoff + i * elf_hdr->e_shentsize);
				char *sh_name = str_contents+elf_shdr->sh_name;
				if(strlen(sh_name) == 0)
					continue;
				//debug("name = %s",sh_name);
			}
			
			u8 *cod_seg;
			
			int data_limit = 1024 * 132;
			
			cod_seg =  (u8 *)memman_alloc_4k(memman, data_limit); //TODO: ����̶���64K
			//data_seg = (u8 *)memman_alloc_4k(memman, 1024*64); //TODO: ���ݶι̶���64K
			task->ds_base = (int) cod_seg;  //��������ݶ���ͬһ����
			task->cs_base = (int) cod_seg;
			
			esp = data_limit - 500;
			char msg[200];
			for(i=0; i<200; i++)
				msg[i] = 0;
			//string_memory(cod_seg+esp, 20, msg);
			//debug(msg);
			
			set_segmdesc(task->ldt + 0, data_limit - 1, (int) cod_seg, AR_CODE32_ER + 0x60); //��������ݶ���ʵָ��ͬһ���ռ�
			set_segmdesc(task->ldt + 1, data_limit - 1, (int) cod_seg, AR_DATA32_RW + 0x60);
			//�������ݶΡ������
			//debug("elf_hdr->e_phnum = %d",elf_hdr->e_phnum);
			for (i=0; i<elf_hdr->e_phnum; i++){
				Elf32_Phdr *elf_phdr = (Elf32_Phdr *)(p + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
				//debug("p_type = %d",elf_phdr->p_type);
				if(elf_phdr->p_type == PT_LOAD){
					debug("see PT_LOAD section");
					
					//debug_Elf32_Phdr(elf_phdr);
					//char msg[1024];
					//int j=0;
					//for(j=0; j<1024; j++)
					//	msg[j] = 0;
					//string_memory(p + elf_phdr->p_offset,elf_phdr->p_filesz,msg);
					//debug(msg);
					//debug("\n");
					
					phys_copy(cod_seg +(int)elf_phdr->p_vaddr, p + elf_phdr->p_offset, elf_phdr->p_filesz);
					
					//sprintf(msg,"");
					//string_memory(cod_seg +(int)elf_phdr->p_vaddr,elf_phdr->p_filesz,msg);
					//debug(msg);
				}
			}
			

			//debug("count = %d", count);
			
			//��װ��argv
			char **argv = (char **)memman_alloc(memman,sizeof(char **) * (count+1));
			//�ο�����
			i = 0;
			while(list != NULL){
				debug("arg = %s",(char *)list->data);
				argv[i] =  (char *)list->data;
				list = list->next;
				i++;
			}
			argv[i] = 0;

			char **p_argv = argv;
			
			int PROC_ORIGIN_STACK = 500;
			char arg_stack[PROC_ORIGIN_STACK];

			int stack_len = 0;
			int argc = 0;
			while(*p_argv++){
				assert(stack_len + 2 * sizeof(char *) < PROC_ORIGIN_STACK);
				stack_len += sizeof(char *);
				argc++;
			}
			debug("argc = %d",argc);
			
			*((int *)(&arg_stack[stack_len])) = 0;
			stack_len += sizeof(char *);
			
			for(p_argv = argv; *p_argv != 0; p_argv++){
				
				assert(stack_len + strlen(*p_argv) + 1 < PROC_ORIGIN_STACK);
				
				strcpy(&arg_stack[stack_len], *p_argv);
				//debug("*p_argv = %s",*p_argv);
				stack_len += strlen(*p_argv);
				arg_stack[stack_len] = 0;
				stack_len++;
			}
			
			//�ͷ�׼������ʱ���ڴ�
			memman_free(memman, argv, sizeof(char **) * (count+1));
			struct Node *tmp = NULL;
			while(list != NULL){
				tmp = list;
				list = list->next;
				memman_free_4k(memman, tmp->data, 1024);
				FreeNode(list);
			}
			
			
			esp = data_limit - PROC_ORIGIN_STACK;
			//debug("esp = %d",esp);

			phys_copy(cod_seg+esp, arg_stack, stack_len);
			
			//debug("stack_len = %d",stack_len);
			
		
			u8 *stack = (u8 *)(cod_seg+esp);
			char *argv_contents = (char *)(stack + (argc + 1) * 4);
			for(i = 0; i<argc; i++){
				//debug("argv_contents = %d",argv_contents);
				*((char **)stack) = (int)argv_contents - (int)cod_seg ;
				argv_contents += strlen(argv_contents) + 1;
				stack += 4;
			}
			
			stack = (u8 *)(cod_seg+esp);
			string_memory(cod_seg+esp-4, stack_len, msg);
			//debug(msg);
			
			//���ջ�е�����
			argv_contents = (char *)(stack + (argc + 1) * 4);
			for(i = 0; i<argc; i++){
				//debug("argv[%d] = %s",i,argv_contents);
				//debug("&argv[0] = %d", (int)argv_contents - (int)cod_seg);
				//debug("stack[0] = %d", *((int *)stack));
				argv_contents += strlen(argv_contents) + 1;
				stack += 4;
			}
			
			//debug("argc = %d, argv = %d", argc, esp);
			char ** pp = (char **)(cod_seg+esp);
			//debug("argv[0] = %d", (int)(pp[0]));
			//debug("tt = %d",*((int *)esp));
			start_app(elf_hdr->e_entry, 0 * 8 + 4, esp-4, 1 * 8 + 4, &(task->tss.esp0), argc, esp); 
		} else {
			cons_putstr0(cons, ".hrb or .elf file format error.\n");
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/* �t�@�C����������Ȃ������ꍇ */
	return 0;
}
	   
static void debug_Elf32_Ehd(Elf32_Ehdr* elf_hdr)
{
	debug("-------------------Elf32 header-------------------");
	debug("e_ident = %s", elf_hdr->e_ident);
	debug("e_type = %d",elf_hdr->e_type);
	debug("e_machine = %d",elf_hdr->e_machine);
	debug("e_version = %d",elf_hdr->e_version);
	debug("e_entry = %d",elf_hdr->e_entry);
	debug("e_phoff = %d",elf_hdr->e_phoff);
	debug("e_shoff = %d",elf_hdr->e_shoff);
	debug("e_flags = %d",elf_hdr->e_flags);
	debug("e_ehsize = %d",elf_hdr->e_ehsize);
	debug("e_phentsize = %d",elf_hdr->e_phentsize);
	debug("e_phnum = %d",elf_hdr->e_phnum);
	debug("e_shentsize = %d",elf_hdr->e_shentsize);
	debug("e_shnum = %d",elf_hdr->e_shnum);
	debug("e_shstrndx = %d",elf_hdr->e_shstrndx);
	debug("--------------------------------------------------");
}
static void debug_Elf32_Phdr(Elf32_Phdr *phdr)
{
	debug("-----------------Program header-------------------");
	debug("p_type = %d", phdr->p_type);
	debug("p_offset = %d", phdr->p_offset);
	debug("p_vaddr = %d", phdr->p_vaddr);
	debug("p_paddr = %d", phdr->p_paddr);
	debug("p_filesz = %d", phdr->p_filesz);
	debug("p_memsz = %d", phdr->p_memsz);
	debug("p_flags = %d", phdr->p_flags);
	debug("p_align = %d", phdr->p_align);
	debug("--------------------------------------------------");
}

static void debug_Elf32_Shdr(Elf32_Shdr *phdr)
{
	debug("-----------------Section header-------------------");
	debug("sh_name = %s", phdr->sh_name);
	debug("sh_type = %d", phdr->sh_type);
	debug("sh_flags = %d", phdr->sh_flags);
	debug("sh_addr = %d", phdr->sh_addr);
	debug("sh_offset = %d", phdr->sh_offset);
	debug("sh_size = %d", phdr->sh_size);
	debug("sh_link = %d", phdr->sh_link);
	debug("sh_info = %d", phdr->sh_info);
	debug("sh_addralign = %d", phdr->sh_addralign);
	debug("sh_entsize = %d", phdr->sh_entsize);
	debug("--------------------------------------------------");
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
