/* FIFO���� */

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
/* FIFO��������ʼ�� */
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* ʣ��ռ� */
	fifo->flags = 0;
	fifo->p = 0; /* д��λ�� */
	fifo->q = 0; /* ��ȡλ�� */
	fifo->task = task; /* ������д��ʱ��Ҫ���ѵ����� */
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* ��FIFOд�����ݲ��ۻ����� */
{
	if (fifo->free == 0) {
		/* û��ʣ��ռ������ */
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	if (fifo->task != 0) {
		if (fifo->task->flags != 2) { /* �������������״̬ */
			//char msg[100];
			//sprintf(msg,"make process[%d,%s] awake",fifo->task->pid,fifo->task->name);
			//print_on_screen(msg);
			//debug("fifo32_put: wakeup pid: %d",fifo->task->pid);
			task_run(fifo->task, -1, 0); /* �������� */
		}
	}
	return 0;
}

int fifo32_put2(struct FIFO32 *fifo, int data)
/* ��FIFOд�����ݲ��ۻ����� */
{
	if (fifo->free == 0) {  //���˾����� 
		fifo->p = 0; /* д��λ�� */
		fifo->q = 0; /* ��ȡλ�� */
	}else{
		fifo->buf[fifo->p] = data;
		fifo->p++;
		if (fifo->p == fifo->size) {
			fifo->p = 0;
		}
		fifo->free--;
	}
	
	if (fifo->task != 0) {
		if (fifo->task->flags != 2) { /* �������������״̬ */
			//debug("fifo32_put2: wakeup pid: %d, data=%d",fifo->task->pid,data);
			task_run(fifo->task, -1, 0); /* �������� */
		}
	}
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)
/* ��FIFO�л�ȡ���� */
{
	int data;
	if (fifo->free == fifo->size) {
		/* FIFOΪ�� */
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}



int fifo32_status(struct FIFO32 *fifo)
/* ��ȡ���л��ж��ٿռ� */
{
	return fifo->size - fifo->free;
}
