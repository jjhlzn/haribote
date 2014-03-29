/* マルチタスク関係 */

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
	task->flags = 2; /* ｻ�ｶｯﾖﾐ */
	return;
}


struct TASK * get_task(int pid)
{
	return &taskctl->tasks0[pid];
}

void task_remove(struct TASK *task, enum TASK_STATUS task_status)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* ﾑｰﾕﾒtaskﾋ�ﾔﾚｵﾄﾎｻﾖﾃ */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* ﾔﾚﾕ簑� */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* ﾐ靨ｪﾒﾆｶｯｳﾉﾔｱ｣ｬﾒｪﾏ獗ｦｵﾄｴｦﾀ� */
	}
	if (tl->now >= tl->running) {
		/* ﾈ郢�nowｵﾄﾖｵｳ�ﾏﾖﾒ�ｳ｣｣ｬﾔ�ｽ�ﾐﾐﾐﾞﾕ� */
		tl->now = 0;
	}
	//task->flags = 1; /* ﾐﾝﾃﾟﾖﾐ */
	task->flags = task_status;

	/* ﾒﾆｶｯ */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

/* ﾇﾐｻｻｵｽﾗ�ｸﾟﾓﾅﾏﾈｼｶｵﾄLEVEL  */
void task_switchsub(void)
{
	int i;
	/* ﾑｰﾕﾒﾗ�ﾉﾏｲ羞ﾄLEVEL */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* ﾕﾒｵｽﾁﾋ */
		}
	}
	taskctl->now_lv = i; //ﾇﾐｻｻｵｽﾗ�ｸﾟﾓﾅﾏﾈｼｶｵﾄLEVEL
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
	//ﾊﾂﾏﾈｽｫﾈｫｲｿｵﾄTASKｽ盪ｹｵﾄTSSｺﾍLDT｡｢ﾎﾄｼ�ﾃ靆�ｷ�ｶｼﾉ靹ﾃｺﾃ
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].pid = i;
		taskctl->tasks0[i].forked = 0;
		strcpy(taskctl->tasks0[i].name,"anony");
		taskctl->tasks0[i].flags = 0; //ｱ�ﾖｾﾎｴﾊｹﾓﾃ
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8; //ﾉ靹ﾃTSSｵﾄseletor, ｵｱｽ�ﾐﾐﾈﾎﾎ�ﾇﾐｻｻｵﾄﾊｱｺ�｣ｬﾊﾇﾌ�ﾗｪｵｽﾕ篋�ｶﾎﾉﾏｵﾄ｡｣
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8; 
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);  //TSSｶﾎ 
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT); //LDTｶﾎ
		//ﾉ靹ﾃﾈﾎﾎ�ｵﾄﾎﾄｼ�ﾃ靆�ｷ�
		for(j=0; j<NR_FILES; j++){
			taskctl->tasks0[i].filp[j] = 0;
		}
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	task = task_alloc();
	task->flags = 2;	/* ｻ�ｶｯﾖﾐｱ�ﾖｾ */
	task->priority = 2; /* 0.02ﾃ� */
	task->level = 0;	/* ﾗ�ｸﾟLEVEL */
	task_add(task);
	task_switchsub();	/* LEVELﾉ靹ﾃ */
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
	
	//ﾄｬﾈﾏﾇ鯀�ﾏﾂ｣ｬﾋ�ﾓﾐｽ�ｳﾌｵﾄｸｸｽ�ｳﾌｶｼﾊﾇidle｣ｬｰ�ﾀｨtask_aｺﾍidleﾗﾔｼｺ｡｣
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[0].parent_pid = idle->pid;
	}

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* ﾕ�ﾔﾚﾊｹﾓﾃｵﾄｱ�ﾖｾ */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* ﾕ簑�ﾏﾈﾉ靹ﾃﾎｪ0 */
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
			task->forked = 0; //ﾖﾘﾖﾃﾊﾇｷ�ﾊﾇforkｴｴｽｨｵﾄｱ�ﾖｾ
			task->exit_status = -1000;
			return task;
		}
	}
	return 0; /* ﾈｫｲｿﾕ�ﾔﾚﾊｹﾓﾃ */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* ｲｻｸﾄｱ膈EVEL */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* ｸﾄｱ莉�ｶｯﾖﾐｵﾄLEVEL */
		task_remove(task, TASK_STATUS_SLEEP); /* ﾕ簑�ﾖｴﾐﾐﾖｮｺ�flagｵﾄﾖｵｻ盂萸ｪ1｣ｬﾓﾚﾊﾇﾏﾂﾃ豬ﾄifﾓ�ｾ萪ｲｻ盂ｻﾖｴﾐﾐ */
	}
	if (task->flags != 2) {
		/* ｴﾓﾐﾝﾃﾟﾗｴﾌｬｻｽﾐﾑｵﾄﾇ鰔ﾎ */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* ﾏﾂｴﾎﾈﾎﾎ�ﾇﾐｻｻﾊｱｼ�ｲ餃EVEL */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* ﾈ郢�ｴｦﾓﾚｻ�ｶｯﾗｴﾌｬ */
		now_task = task_now();
		task_remove(task,TASK_STATUS_SLEEP); /* ﾖｴﾐﾐｴﾋﾓ�ｾ莊ﾄｻｰflagsｽｫｱ萸ｪ1 */
		if (task == now_task) {
			/* ﾈ郢�ﾊﾇﾈﾃﾗﾔｼｺﾐﾝﾃﾟ｣ｬﾔ�ﾐ靨ｪｽ�ﾐﾐﾈﾎﾎ�ﾇﾐｻｻ */
			task_switchsub();
			now_task = task_now(); /* ﾔﾚﾉ雜ｨｺ�ｻ�ﾈ｡ｵｱﾇｰﾈﾎﾎ�ｵﾄﾖｵ */
			//debug("process[%d,%s] go to sleep, process[%d,%s] is running",task->pid,task->name,now_task->pid,now_task->name);
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_wait(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == TASK_STATUS_RUNNING) {
		/* ﾈ郢�ｴｦﾓﾚｻ�ｶｯﾗｴﾌｬ */
		now_task = task_now();
		debug("task->pid = %d, now_task->pid = %d",task->pid, now_task->pid);
		task_remove(task,TASK_STATUS_WAITING); /* ﾖｴﾐﾐｴﾋﾓ�ｾ莊ﾄｻｰflagsｽｫｱ萸ｪ1 */
		if (task == now_task) {
			/* ﾈ郢�ﾊﾇﾈﾃﾗﾔｼｺﾐﾝﾃﾟ｣ｬﾔ�ﾐ靨ｪｽ�ﾐﾐﾈﾎﾎ�ﾇﾐｻｻ */
			task_switchsub();
			now_task = task_now(); /* ﾔﾚﾉ雜ｨｺ�ｻ�ﾈ｡ｵｱﾇｰﾈﾎﾎ�ｵﾄﾖｵ */
			debug("process[%d,%s] go to wait, process[%d,%s] is running",task->pid,task->name,now_task->pid,now_task->name);
			farjmp(0, now_task->sel);
		}
	}
	return;
}


void task_exit(struct TASK *task, enum TASK_STATUS task_status)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* ﾈ郢�ｴｦﾓﾚｻ�ｶｯﾗｴﾌｬ */
		now_task = task_now();
		task_remove(task, task_status); 
		if (task == now_task) {
			/* ﾈ郢�ﾊﾇﾈﾃﾗﾔｼｺﾐﾝﾃﾟ｣ｬﾔ�ﾐ靨ｪｽ�ﾐﾐﾈﾎﾎ�ﾇﾐｻｻ */
			task_switchsub();
			now_task = task_now(); /* ﾔﾚﾉ雜ｨｺ�ｻ�ﾈ｡ｵｱﾇｰﾈﾎﾎ�ｵﾄﾖｵ */
			//debug("process[%d,%s] go to sleep, process[%d,%s] is running",task->pid,task->name,now_task->pid,now_task->name);
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_hang(struct TASK *task)
{
}

void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) { //ﾈﾎﾎ�ｿﾘﾖﾆﾆ�ｵﾄLEVELﾒﾑｾｭｷ｢ﾏﾖｹ�ｱ莉ｯ
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

/*  ｻ�ﾈ｡ﾋ�ﾓﾐｿﾉﾔﾋﾐﾐｵﾄｽ�ｳﾌ */
struct Node* get_all_running_tasks(){
	int i, j;
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

