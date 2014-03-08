/* FIFOライブラリ */

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
/* FIFOｻｺｳ衂�ｳ�ﾊｼｻｯ */
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* ﾊ｣ﾓ狒ﾕｼ� */
	fifo->flags = 0;
	fifo->p = 0; /* ﾐｴﾈ�ﾎｻﾖﾃ */
	fifo->q = 0; /* ｶﾁﾈ｡ﾎｻﾖﾃ */
	fifo->task = task; /* ﾓﾐﾊ�ｾﾝﾐｴﾈ�ﾊｱﾐ靨ｪｻｽﾐﾑｵﾄﾈﾎﾎ� */
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* ﾏ�FIFOﾐｴﾈ�ﾊ�ｾﾝｲ｢ﾀﾛｻ�ﾆ�ﾀｴ */
{
	if (fifo->free == 0) {
		/* ﾃｻﾓﾐﾊ｣ﾓ狒ﾕｼ蕚�ﾒ邉� */
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
		if (fifo->task->flags != 2) { /* ﾈ郢�ﾈﾎﾎ�ｴｦﾓﾚﾐﾝﾃﾟﾗｴﾌｬ */
			task_run(fifo->task, -1, 0); /* ｽｫﾈﾎﾎ�ｻｽﾐﾑ */
		}
	}
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)
/* FIFOからデータを一つとってくる */
{
	int data;
	if (fifo->free == fifo->size) {
		/* バッファが空っぽのときは、とりあえず-1が返される */
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
/* どのくらいデータが溜まっているかを報告する */
{
	return fifo->size - fifo->free;
}
