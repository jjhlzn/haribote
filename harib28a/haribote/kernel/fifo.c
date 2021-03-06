/* FIFO队列 */

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

//// FIFO缓冲区初始化 
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 剩余空间 */
	fifo->flags = 0;
	fifo->p = 0; /* 写入位置 */
	fifo->q = 0; /* 读取位置 */
	fifo->task = task; /* 有数据写入时需要唤醒的任务 */
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* 向FIFO写入数据并累积起来 */
{
	if (fifo->free == 0) {
		/* 没有剩余空间则溢出 */
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
		if (fifo->task->flags == TASK_STATUS_SLEEP) { /* 如果任务处于休眠状态 */
			task_run(fifo->task, -1, 0); /* 将任务唤醒 */
		}
	}
	return 0;
}

int fifo32_put2(struct FIFO32 *fifo, int data)
/* 向FIFO写入数据并累积起来 */
{
	if (fifo->free == 0) {  //满了就重置 
		fifo->p = 0; /* 写入位置 */
		fifo->q = 0; /* 读取位置 */
	}else{
		fifo->buf[fifo->p] = data;
		fifo->p++;
		if (fifo->p == fifo->size) {
			fifo->p = 0;
		}
		fifo->free--;
	}
	
	if (fifo->task != 0) {
		if (fifo->task->flags == TASK_STATUS_SLEEP) { /* 如果任务处于休眠状态 */
			//debug("fifo32_put2: wakeup pid: %d, data=%d",fifo->task->pid,data);
			task_run(fifo->task, -1, 0); /* 将任务唤醒 */
		}
	}
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)
/* 从FIFO中获取数据 */
{
	int data;
	if (fifo->free == fifo->size) {
		/* FIFO为空 */
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
/* 获取队列还有多少空间 */
{
	return fifo->size - fifo->free;
}
