#ifndef	_BOOTPACK_H
#define	_BOOTPACK_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

/* asmhead.nas */
struct hd_i_struct {
	int head; //��ͷ��
	int sect; //û�ŵ�������
	int cyl;  //������
	int wpcom; //дǰԤ���������
	int lzone; //��ͷ��½�������
	int ctl; //�����ֽ�
};
	
struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* �u�[�g�Z�N�^�͂ǂ��܂Ńf�B�X�N��ǂ񂾂̂� */
	char leds; /* �u�[�g���̃L�[�{�[�h��LED�̏�� */
	char vmode; /* �r�f�I���[�h  ���r�b�g�J���[�� */
	char reserve;
	short scrnx, scrny; /* ��ʉ𑜓x */
	char *vram;
	int hd0;
	char hd1[16];
	//struct hd_i_struct hd_info;
};
#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG		0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler0e(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2c(void);
void asm_inthandler2e(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void asm_linux_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0, int argc, int argv);
void start_app_elf(int eip, int cs, int esp, int ds, int *tss_esp0, int argc, int argv);
void asm_end_app(void);
void ud2(); 
void open_page();
int  get_cr3();
int  get_cr0();

/* fifo.c */
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_put2(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000		0    //��
#define COL8_FF0000		1    //����
#define COL8_00FF00		2    //����
#define COL8_FFFF00		3    //����
#define COL8_0000FF		4    //����
#define COL8_FF00FF		5    //����
#define COL8_00FFFF		6    //ǳ����
#define COL8_FFFFFF		7    //��
#define COL8_C6C6C6		8    //����
#define COL8_840000		9    //����
#define COL8_008400		10   //����
#define COL8_848400		11   //����
#define COL8_000084		12   //����
#define COL8_840084		13   //����
#define COL8_008484		14   //ǳ����
#define COL8_848484		15   //����

/* dsctbl.c */
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define DESCRIPTOR_LIMIT(sd) ( (unsigned char)sd.limit_high & 0x80 ? \
									( (unsigned int)(((unsigned char)sd.limit_high & 0x0F) << 16) + (unsigned short)sd.limit_low + 1) * 0x1000 : \
									( (unsigned int)(((unsigned char)sd.limit_high & 0x0F) << 16) + (unsigned short)sd.limit_low + 1) )
#define DESCRIPTOR_BASE(sd)  ( ((unsigned int)( ((unsigned char)sd.base_high) << 24)) + ((unsigned int)( ((unsigned char)sd.base_mid) << 16)) + (unsigned short)sd.base_low )

#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT			0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

typedef	unsigned long long	u64;
typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

#define	PUBLIC		/* PUBLIC is the opposite of PRIVATE */
#define	PRIVATE	static	/* PRIVATE x limits the scope of x */

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
int read_from_keyboard(char *buf, int len);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
#define MEMMAN_FREES		4090	/* ��Լ��32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* ������Ϣ */
	unsigned int addr, size;
};
struct MEMMAN {		/* �ڴ���� */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
int get_count_of_free_pages();
int get_count_of_total_pages();
int get_count_of_used_pages();
void mem_init();
void print_page_config();



/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
	struct TASK *task;
	struct TASK *read_kb_task;
};
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout;
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);

/* mtask.c */
#define MAX_TASKS		1000	/* �ő�^�X�N�� */
#define TASK_GDT0		3		/* TSS��GDT�̉��Ԃ��犄�蓖�Ă�̂� */
#define MAX_TASKS_LV	100   /*        ÿ��������ɵĽ���         */
#define MAX_TASKLEVELS	10    /*  ���̹�������㼶 */
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3; //esp0��ss0Ϊ����ϵͳ��ջ�κź�ջ��ָ��
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
#define	NR_FILES	64
struct TASK {
	int pid;
	int parent_pid; //������pid, ����ý�����ͨ��fork�����ģ���ôparent_pid�ǵ���forkϵͳ���õĽ��̡����ߣ������̾���idle��
	int exit_status; //����ý����˳�ʱ�Ĳ������������exit(1)ʱ��exit_status��ֵ����1
	char name[20];
	char forked;
	int sel, flags; /* sel����TSS��GDT��� */
	int level, priority;
	struct FIFO32 fifo;
	struct FIFO32 ch_buf;
	int readKeyboard;
	int sendMouse;  //Ϊ�����api,������api_getMouse��ʱ��sendMouse����Ϊ1�����û�������־λ����ô������Ϣ�ͻ�
	                //һֱ��fifo����䣬�ܿ�fifo�ͻᱻ������
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE *cons;
	struct TASK *wait_return_task; //һ�����̵���wait()ϵͳ���ú���wait()���صĽ��̻����õ����������
	int ds_base, cons_stack;
	int cs_base;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	unsigned char langmode, langbyte1;
	//������ļ�������
	struct file *filp[NR_FILES];
	u16 euid;
	u16 egid;
	u16 uid;
	u16 gid;
	struct m_inode * pwd;  //��ǰ����Ŀ¼
	struct m_inode * root;  //��Ŀ¼
	struct m_inode * executable;
	unsigned long close_on_exec;
	unsigned short umask;
	long leader, pgrp;
	int tty;
	int signal;
};

#define task_struct TASK

enum TASK_STATUS {
	TASK_STATUS_UNUSED = 0,  //��struct TASKδ��ʹ��
	TASK_STATUS_SLEEP = 1,   //�ý����Ѿ�˯���ˣ����߸ý����ڴ����У���������������
	TASK_STATUS_RUNNING = 2, //�ý��̿�������
	TASK_STATUS_HANGING = 3, //�ý����ǽ�ʬ���̣��ȴ������̵���wait()ϵͳ����
	TASK_STATUS_WAITING = 4, //�ý����ڵȴ��ӽ��̽���
	TASK_STATUS_UNINTERRUPTIBLE = 5 //�ý��̲��ܱ��ж�
};

#define TASK_UNINTERRUPTIBLE TASK_STATUS_UNINTERRUPTIBLE
#define current task_now()

#define CURRENT_TIME   1

struct TASKLEVEL {
	int running; /* �������е��������� */
	int now; /* �������������¼��ǰ�������е����ĸ����� */
	struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL {
	int now_lv; /* ���ڻ�е�LEVEL */
	char lv_change; /* ���´������л�ʱ�Ƿ���Ҫ�ı�LEVEL */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
struct Node* get_all_running_tasks();
struct TASK * get_task(int pid);
void task_add(struct TASK *task);
void task_exit(struct TASK *task, enum TASK_STATUS task_status);
void task_wait();
void sleep_on(struct task_struct **p);
void wake_up(struct task_struct **p);

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);

/* console.c */
struct CONSOLE {
	struct SHEET *sht;
	struct SHEET *sht_buf;
	int buf_cur_x, buf_cur_y;
	int buf_x, buf_y;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer; //���timer������ʲô�ģ�����
}; 
struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};
void console_task(struct SHEET *sheet, int memtotal);
void log_task(struct SHEET *sheet, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_langmode(struct CONSOLE *cons, char *cmdline);
void cmd_hd(struct CONSOLE *cons);
void cmd_partition(struct CONSOLE *cons);
void cmd_ls(struct CONSOLE *cons);
void cmd_ps(struct CONSOLE *cons);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			  int fs, int gs,
			 int es, int ds,
			 int eip, int cs, int eflags, int esp0, int ss0);
int *linux_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax,
			   int fs, int gs,
			   int es, int ds,
			   int eip, int cs, int eflags, int user_esp, int user_ss);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);

/* file.c */
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
	
	//unsigned char filename[20]; //Ϊ�˴������6���ַ��ļ��������
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile2(int clustno, int *psize, int *fat);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

/*  proc.c */
PUBLIC	void*	va2la(int pid, void* va);

/*  fs.c */
void init_fs();

/* exec.c */
int do_exec(char *name, char *argv[], int *fat, int *reg_push_by_interrupt);

/* forkexit.c */
void do_exit(struct TASK *p, int status);
struct TASK* do_fork_elf(struct TSS32 *tss);
struct TASK* do_fork(struct TSS32 *tss);
int do_wait(struct TASK *task, int *status);

/* apploader.c */
int load_app(struct CONSOLE *cons, int *fat, char *cmdline);

/* utils.c */
void string_memory(u8 *mem, int size, char *buf);


/* Hard Drive */
#define SECTOR_SIZE		512
#define SECTOR_BITS		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9


/**
 * MESSAGE mechanism is borrowed from MINIX
 */
struct mess1 {
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};
struct mess2 {
	void* m2p1;
	void* m2p2;
	void* m2p3;
	void* m2p4;
};
struct mess3 {
	int	m3i1;
	int	m3i2;
	int	m3i3;
	int	m3i4;
	u64	m3l1;
	u64	m3l2;
	void*	m3p1;
	void*	m3p2;
};
typedef struct {
	int source;
	int type;
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;
} MESSAGE;



/* #define	PID		u.m3.m3i2 */
/* #define	STATUS		u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1
/* #define	STATUS		u.m3.m3i1 */


/**
 * @enum msgtype
 * @brief MESSAGE types
 */
enum msgtype {
	/* 
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT = 1,

	/* SYS task */
	GET_TICKS,

	/* FS */
	OPEN, CLOSE, READ, WRITE, LSEEK, STAT, UNLINK,

	/* TTY, SYS, FS, MM, etc */
	SYSCALL_RET,

	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};



#define	phys_copy	memcpy1
#define	phys_set	memset1

/* max() & min() */
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))
      
PUBLIC	void*	memcpy1(void* p_dst, void* p_src, int size);
PUBLIC	void	memset1(void* p_dst, char ch, int size);

PUBLIC void	port_read(u16 port, void* buf, int n);
PUBLIC void	port_write(u16 port, void* buf, int n);


PUBLIC void print_on_screen2(char *msg, int x, int y);
PUBLIC void print_on_screen(char *msg);
void print_on_screen3(char *fmt, ...);
void debug_userspace(char *str);
void debug_userspace1(char *str, int len);
void get_str_userspace(char *str, char *buf);
void get_str_userspace1(char *str, int len, char *buf);
PUBLIC void debug(const char *fmt, ...);
void panic(const char *fmt, ...);

//#define	NR_FILES	64
#define	NR_FILE_DESC	64	/* FIXME */

#define	NR_SUPER_BLOCK	8


/* magic chars used by `printx' */
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'

/* the assert macro */
#define ASSERT
#ifdef ASSERT 
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if (exp) ; \
        else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

#define	DIOCTL_GET_GEO	1

#define	PROC_IMAGE_SIZE_DEFAULT	0x100000 /*  1 MB */

#define cli io_cli
#define sti io_sti
#define printk debug
#define suser() (current->euid == 0)
void hd_init(void);

void print_hdinfo(char * str);
typedef int (*fn_ptr)();



/****** log buffer manager ***/

#define LOG_ENTRY_SIZE 512
/* logBufferMgrά��һ��log buffer�ĳأ�ÿ��debug��Ҫ����־ʱ����Ҫ�������ȡ��һ�����壬Ȼ��ŵ�log�����һ��������
 * log_console��������������л�ȡlog����ĵ�ַ����ʾ���֮���ٰѻ��巵�ص����� */
struct LogBufferMgr{
	/* �ܵĻ������ */
	int buf_count;
	/* ���л������ */
	int frees;
	/* ���л����ͷ */
	int nr_free_buf;
	/* buf��һ���������ڴ棬ÿ����־���ڴ��ַ����buf + i*/
	char *buf;
};

void init_logmgr(struct LogBufferMgr *log_mgr);
char * get_log_buf(struct LogBufferMgr *log_mgr);
void put_log_buf(struct LogBufferMgr *log_mgr, char *buf);
#endif
