#include "bootpack.h"
#include "linkedlist.h"
#include "fs.h"
#include "memory.h"
#include <string.h>
#include <signal.h>

struct TASKCTL *taskctl;
struct TIMER *task_timer;
struct m_inode * get_root_inode();
static int  get_new_pid();
static void _task_change_status(struct TASK *task, enum TASK_STATUS task_status);
static void check_sigchld_signal();

int sys_getpid()
{
	return current->pid;
}

////获取当前正在运行的任务
//返回：当前在运行的任务
struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

////将任务放到可运行队列中
//参数：需要被放到可运行的任务
void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = TASK_STATUS_RUNNING; /* 活动中 */
	return;
}

////获取任务结构，这个结构必须要在使用中
//参数:pid -- 任务的pid
//返回：成功找到在使用的任务结构，返回这个任务结构，否则返回NULL
struct TASK * get_task(int pid)
{
	int i;
	for(i=0 ; i<MAX_TASKS ; i++)
		if (taskctl->tasks0[i].flags != 0 && taskctl->tasks0[i].pid ==pid) 
			return &taskctl->tasks0[i];
	return NULL;
}

////把该任务从可运行队列中删除，并且将它设置为指定的状态
//参数：task -- 从可运行队列中被删除的任务，task_status -- 设成的目标状态
void task_remove(struct TASK *task, enum TASK_STATUS task_status)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* 寻找task所在的位置 */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* 在这里 */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* 需要移动成员，要相应的处理 */
	}
	if (tl->now >= tl->running) {
		/* 如果now的值出现异常，则进行修正 */
		tl->now = 0;
	}

	/* 设置成参数指定的状态 */
	task->flags = task_status;

	/* 移动 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

//// 切换到最高优先级的LEVEL
void task_switchsub(void)
{
	int i;
	/* 寻找最上层的LEVEL */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* 找到了 */
		}
	}
	taskctl->now_lv = i; //切换到最高优先级的LEVEL
	taskctl->lv_change = 0;
	return;
}

////idle进程的运行主函数
void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

////初始化进程管理器
//返回：内核进程task_a
struct TASK *task_init(struct MEMMAN *memman)
{
	int i,j;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	//事先将全部的TASK结构的TSS和LDT、文件描述符都设置好
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].pid = -1;
		taskctl->tasks0[i].forked = 0;
		strcpy(taskctl->tasks0[i].name,"anony");
		taskctl->tasks0[i].flags = 0; //标志未使用
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8; //设置TSS的seletor, 当进行任务切换的时候，是跳转到这个段上的。
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8; //任务号
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);  //TSS段 
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT); //LDT段
		//设置任务的文件描述符
		for(j=0; j<NR_FILES; j++){
			taskctl->tasks0[i].filp[j] = 0;
		}
		
		/* TODO: 初始化进程的键盘输入缓冲，这里的内存可能没有被释放*/
		int bufCount = 100;
		int *fifobuf = (int *)memman_alloc(memman,sizeof(int)*bufCount);
		taskctl->tasks0[i].ch_buf.task = &taskctl->tasks0[i];
		fifo32_init(&taskctl->tasks0[i].ch_buf, bufCount, fifobuf, 0);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	task = task_alloc();
	task->flags = 2;	/* 活动中标志 */
	task->priority = 2; /* 0.02秒 */
	task->level = 0;	/* 最高LEVEL */
	task_add(task);
	task_switchsub();	/* LEVEL设置 */
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	strcpy(idle->name,"idle");
	task_run(idle, MAX_TASKLEVELS - 1, 1);
	
	//默认情况下，所有进程的父进程都是idle，包括task_a和idle自己。
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[0].parent_pid = idle->pid;
	}

	return task;
}

////分配一个未使用的任务结构
//返回：未使用的任务结构
struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task = NULL;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->signal = 0;
			task->pid = get_new_pid();
			task->nr = i;
			debug("alloc pid = %d",task->pid);
			task->flags = 1; /* 正在使用的标志 */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* 这里先设置为0 */
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.cr3 =   PAGE_DIR_ADDR;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			task->forked = 0; //重置是否是fork创建的标志
			task->exit_status = -1000;
			
			task->pwd = get_root_inode();
			task->root = get_root_inode();
			
			task->readKeyboard = 0;
			
			/* 打开标准文件：STDIN, STDOUT, STDERR */
			open_std_files(task);
			
			return task;
		}
	}
	return 0; /* 全部正在使用 */
}

////将task的放到可运行队列中，并且重新设置level和priority
//参数：task -- 被操作的任务，level -- 目的level, priority -- 目的优先级
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {   /* 不改变LEVEL */
		level = task->level;
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 改变活动中的LEVEL */
		task_remove(task, TASK_STATUS_SLEEP); /* 这里执行之后flag的值会变为1，于是下面的if语句也会被执行 */
	}
	if (task->flags != 2) {
		
		/* 从休眠状态唤醒的情形 */
		task->level = level;
		/* 放置到可运行的队列中 */
		task_add(task);
	}

	taskctl->lv_change = 1; /* 下次任务切换时检查LEVEL */
	return;
}

////将该任务从可运行队列中移除，并且设置为睡眠状态
//参数: task -- 需要睡眠的任务
void task_sleep(struct TASK *task)
{
	_task_change_status(task, TASK_STATUS_SLEEP);
}

////将当前任务从可裕兴队列中移除，同时设置为TASK_STATUS_WAITING状态
//参数: task -- 需要设为等待的任务
void task_wait()
{
	/* has children, but no child is HANGING */
	debug("TASK[%d]: wait child!",current->pid);
	_task_change_status(current, TASK_STATUS_WAITING);
}

void task_exit(struct TASK *task, enum TASK_STATUS task_status)
{
	_task_change_status(task, task_status);
}

////将该任务从可裕兴队列中移除，同时设置为参数指定的状态
//参数：task -- 需要操作的任务, task_status -- task被设成这个任务状态
static void _task_change_status(struct TASK *task, enum TASK_STATUS task_status)
{
	struct TASK *now_task;
	if (task->flags == TASK_STATUS_RUNNING) {
		/* 如果处于活动状态 */
		now_task = task_now();
		task_remove(task, task_status); 
		if (task == now_task) {
			/* 如果是让自己休眠，则需要进行任务切换 */
			task_switchsub();
			now_task = task_now(); /* 在设定后获取当前任务的值 */
			//debug("process[%d,%s] go to sleep, process[%d,%s] is running",task->pid,task->name,now_task->pid,now_task->name);
			farjmp(0, now_task->sel);
			
		}
	}
	return;
}

////检查状态未WAIT的任务，一旦发现这些任务有子进程结束信号，将这些放到可运行的状态
static void check_sigchld_signal()
{
	int i;
	struct TASK *task = NULL;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == TASK_STATUS_WAITING) {
			task = &taskctl->tasks0[i];
			if( (task->signal >> (SIGCHLD-1)) & 0x01 ){
				/* 复位信号 */
				task->signal &= ~(1<<(SIGCHLD-1));
				/* 检测到子进程结束信号，重新进入可执行状态 */
				task_run(task, -1, 0);
			}
		}
	}
}

////执行任务切换
void task_switch(void)
{
	/* 检查子进程结束信号 */
	check_sigchld_signal();
	
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	
	/* 任务控制器的LEVEL已经发现过变化 */
	if (taskctl->lv_change != 0) { 
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}

////  获取所有可运行的进程 
struct Node* get_all_running_tasks(){
	int i;
	struct Node *head = NULL;
	
	for(i = 0; i<MAX_TASKS; i++){
		struct TASK *task = &taskctl->tasks0[i];
		if(task->flags == TASK_STATUS_SLEEP || task->flags == TASK_STATUS_RUNNING || task->flags == TASK_STATUS_WAITING)
		{
			struct Node *node = CreateNode(task);
			//debug("i = %d, pid = %d, status = %d", i, task->pid, task->flags);
			if(head == NULL){
				head = node;
			}else{
				Append(head,node);
			}
		}
	}
	return head;
}

/*******************************文件系统升级修改*****************************/
////让当前任务等待指定的任务，在指定的任务未唤醒当前任务之前，当前任务要一直保持睡眠状态
//参数：p -- 当前任务要等待的任务
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	tmp = *p;
	*p = current;
	current->flags = TASK_UNINTERRUPTIBLE;
	task_switch();
	if (tmp)
		tmp->flags= TASK_STATUS_RUNNING;
}

////唤醒等待当前任务的任务
//参数: p -- 需要被唤醒的任务
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).flags=TASK_STATUS_RUNNING;
		*p=NULL;
	}
}

///////////////////////////////////////////////////////////////////////////


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
	return va;
}


/**********************************************PRIVATE FUNCITONS*********************************************************/
static int  get_new_pid()
{
	static int last_pid = 0;
	int i;
repeat:
	if ((++last_pid)<0) last_pid=1;
	for(i=0 ; i<MAX_TASKS ; i++)
		if (taskctl->tasks0[i].flags != 0 && taskctl->tasks0[i].pid == last_pid) 
			goto repeat;
	return last_pid;
}


