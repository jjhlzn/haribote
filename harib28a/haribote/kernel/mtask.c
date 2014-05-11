/* �}���`�^�X�N�֌W */

#include "bootpack.h"
#include "linkedlist.h"
#include "fs.h"
#include "memory.h"
#include <string.h>

struct TASKCTL *taskctl;
struct TIMER *task_timer;
struct m_inode * get_root_inode();
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
	task->flags = 2; /* ��� */
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

	/* Ѱ��task���ڵ�λ�� */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* ������ */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* ��Ҫ�ƶ���Ա��Ҫ��Ӧ�Ĵ��� */
	}
	if (tl->now >= tl->running) {
		/* ���now��ֵ�����쳣����������� */
		tl->now = 0;
	}
	//task->flags = 1; /* ������ */
	task->flags = task_status;

	/* �ƶ� */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

/* �л���������ȼ���LEVEL  */
void task_switchsub(void)
{
	int i;
	/* Ѱ�����ϲ��LEVEL */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* �ҵ��� */
		}
	}
	taskctl->now_lv = i; //�л���������ȼ���LEVEL
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
	//���Ƚ�ȫ����TASK�ṹ��TSS��LDT���ļ������������ú�
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].pid = i;
		taskctl->tasks0[i].forked = 0;
		strcpy(taskctl->tasks0[i].name,"anony");
		taskctl->tasks0[i].flags = 0; //��־δʹ��
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8; //����TSS��seletor, �����������л���ʱ������ת��������ϵġ�
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8; 
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);  //TSS�� 
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT); //LDT��
		//����������ļ�������
		for(j=0; j<NR_FILES; j++){
			taskctl->tasks0[i].filp[j] = 0;
		}
		
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
	task->flags = 2;	/* ��б�־ */
	task->priority = 2; /* 0.02�� */
	task->level = 0;	/* ���LEVEL */
	task_add(task);
	task_switchsub();	/* LEVEL���� */
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
	
	//Ĭ������£����н��̵ĸ����̶���idle������task_a��idle�Լ���
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
			task->flags = 1; /* ����ʹ�õı�־ */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* ����������Ϊ0 */
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
			task->forked = 0; //�����Ƿ���fork�����ı�־
			task->exit_status = -1000;
			
			task->pwd = get_root_inode();
			task->root = get_root_inode();
			
			task->readKeyboard = 0;
			open_std_files(task);
			return task;
		}
	}
	return 0; /* ȫ������ʹ�� */
}
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* ���ı�LEVEL */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* �ı��е�LEVEL */
		task_remove(task, TASK_STATUS_SLEEP); /* ����ִ��֮��flag��ֵ���Ϊ1�����������if���Ҳ�ᱻִ�� */
	}
	if (task->flags != 2) {
		/* ������״̬���ѵ����� */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* �´������л�ʱ���LEVEL */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* ������ڻ״̬ */
		now_task = task_now();
		task_remove(task,TASK_STATUS_SLEEP); /* ִ�д����Ļ�flags����Ϊ1 */
		if (task == now_task) {
			/* ��������Լ����ߣ�����Ҫ���������л� */
			task_switchsub();
			now_task = task_now(); /* ���趨���ȡ��ǰ�����ֵ */
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
		/* ������ڻ״̬ */
		now_task = task_now();
		//debug("task->pid = %d, now_task->pid = %d",task->pid, now_task->pid);
		task_remove(task,TASK_STATUS_WAITING); /* ִ�д����Ļ�flags����Ϊ1 */
		if (task == now_task) {
			/* ��������Լ����ߣ�����Ҫ���������л� */
			task_switchsub();
			now_task = task_now(); /* ���趨���ȡ��ǰ�����ֵ */
			debug("proc[%d,%s] go to wait, proc[%d,%s] will run",task->pid,task->name,now_task->pid,now_task->name);
			debug("now_task[%d]->tss.eip = %d",now_task->pid,now_task->tss.eip);
			farjmp(0, now_task->sel);
		}
	}
	return;
}


void task_exit(struct TASK *task, enum TASK_STATUS task_status)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* ������ڻ״̬ */
		now_task = task_now();
		task_remove(task, task_status); 
		if (task == now_task) {
			/* ��������Լ����ߣ�����Ҫ���������л� */
			task_switchsub();
			now_task = task_now(); /* ���趨���ȡ��ǰ�����ֵ */
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
	if (taskctl->lv_change != 0) { //�����������LEVEL�Ѿ����ֹ��仯
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		//debug("switch from process[%d,%s] to process[%d,%s]",now_task->pid,now_task->name,new_task->pid,new_task->name);
		if(now_task->pid == 4)
			debug("now_task[%d]->tss.eip = %d", now_task->pid, now_task->tss.eip);
		farjmp(0, new_task->sel);
	}
	return;
}

/*  ��ȡ���п����еĽ��� */
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
	//struct proc* p = &proc_table[pid];

	//u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	//u32 la = seg_base + (u32)va;

	////if (pid < NR_TASKS + NR_PROCS) {
	////	assert(la == (u32)va);
	////}

	//return (void*)la;
	
	return va;
}



/*******************************�ļ�ϵͳ�����޸�*****************************/
//TODO: ����ṹ��ͬ ��Ҳ�������µ�״̬
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	//if (current == &(init_task.task))
	//	panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->flags = TASK_UNINTERRUPTIBLE;
	//schedule();
	task_switch();
	if (tmp)
		tmp->flags= TASK_STATUS_RUNNING;
}

//TODO: 
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).flags=TASK_STATUS_RUNNING;
		*p=NULL;
	}
}

///////////////////////////////////////////////////////////////////////////



