/* キーボード関係 */

#include "bootpack.h"
#include "window.h"

struct FIFO32 *keyfifo;
int keydata0;



void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);	/* IRQ-01受付完了をPICに通知 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, data + keydata0);
	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
	/* キーボードコントローラがデータ送信可能になるのを待つ */
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	/* 書き込み先のFIFOバッファを記憶 */
	keyfifo = fifo;
	keydata0 = data0;
	/* キーボードコントローラの初期化 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

int read_from_keyboard(struct TASK *task, int mode)
{
	task->readKeyboard = 1;
	int i;
	struct CONSOLE *cons = task->cons;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->ch_buf) == 0) {
			if (mode != 0) {
				task_sleep(task);	/* FIFOﾖﾐﾃｻﾓﾐﾄﾚﾈﾝ｣ｬﾋｯﾃﾟｵﾈｴ� */
			} else {
				io_sti();
				return -1;
			}
		}
		i = fifo32_get(&task->ch_buf);
		io_sti();
		if (i >= 256 && i<512) { /* ｼ�ﾅﾌｰｴｼ� */
			if (cons->cur_x < CONSOLE_CONTENT_WIDTH) {
				/* ﾏﾔﾊｾﾊ菠�ｵﾄﾗﾖｷ� */
				cons_putchar(cons, i - 256, 1);
			}
			task->readKeyboard = 0;
			return i - 256;
		}
	}
}
