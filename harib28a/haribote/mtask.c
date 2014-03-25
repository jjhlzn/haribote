/* 儅儖僠僞僗僋娭學 */

#include "bootpack.h"
#include "linkedlist.h"
#include "fs.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; /* 活动中 */
	return;
}

void task_remove(struct TASK *task)
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
	task->flags = 1; /* 休眠中 */

	/* 移动 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

/* 切换到最高优先级的LEVEL  */
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

void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i,j;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	//事先将全部的TASK结构的TSS和LDT、文件描述符都设置好
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].pid = i;
		taskctl->tasks0[i].forked = 0;
		strcpy(taskctl->tasks0[i].name,"anony");
		taskctl->tasks0[i].flags = 0; //标志未使用
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8; //设置TSS的seletor, 当进行任务切换的时候，是跳转到这个段上的。
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8; 
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);  //TSS段 
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT); //LDT段
		//设置任务的文件描述符
		for(j=0; j<NR_FILES; j++){
			taskctl->tasks0[i].filp[j] = 0;
		}
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

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
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
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			task->forked = 0; //重置是否是fork创建的标志
			return task;
		}
	}
	return 0; /* 全部正在使用 */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* 不改变LEVEL */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 改变活动中的LEVEL */
		task_remove(task); /* 这里执行之后flag的值会变为1，于是下面的if语句也会被执行 */
	}
	if (task->flags != 2) {
		/* 从休眠状态唤醒的情形 */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* 下次任务切换时检查LEVEL */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* 如果处于活动状态 */
		now_task = task_now();
		task_remove(task); /* 执行此语句的话flags将变为1 */
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

void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) { //任务控制器的LEVEL已经发现过变化
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		//debug("switch from process[%d,%s] to process[%d,%s]",now_task->pid,now_task->name,new_task->pid,new_task->name);
		farjmp(0, new_task->sel);
	}
	return;
}

/*  获取所有可运行的进程 */
struct Node* get_all_running_tasks(){
	int i, j;
	struct Node *head = NULL;
	//迭代每个层级的可运行的进程
	for(i=0; i<MAX_TASKLEVELS; i++){
		struct TASKLEVEL level = taskctl->level[i];
		for(j=0; j<level.running; j++){
			struct TASK *task = level.tasks[j];
			struct Node *node = CreateNode(task);
			if(head == NULL){
				head = node;
			}else{
				Append(head,node);
			}
		}
	}
	return head;
}
