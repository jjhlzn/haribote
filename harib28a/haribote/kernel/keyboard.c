/* 僉乕儃乕僪娭學 */

#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);	/* IRQ-01庴晅姰椆傪PIC偵捠抦 */
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
	/* 僉乕儃乕僪僐儞僩儘乕儔偑僨乕僞憲怣壜擻偵側傞偺傪懸偮 */
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	/* 彂偒崬傒愭偺FIFO僶僢僼傽傪婰壇 */
	keyfifo = fifo;
	keydata0 = data0;
	/* 僉乕儃乕僪僐儞僩儘乕儔偺弶婜壔 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

int read_from_keyboard(struct TASK *task, int mode)
{
	task->readKeyboard = 1;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	int i;
	struct CONSOLE *cons = task->cons;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->ch_buf) == 0) {
			if (mode != 0) {
				task_sleep(task);	/* FIFO中没有内容，睡眠等待 */
			} else {
				io_sti();
				return -1;
			}
		}
		i = fifo32_get(&task->ch_buf);
		io_sti();
		if (i >= 256 && i<512) { /* 键盘按键 */
			if (cons->cur_x < CONSOLE_CONTENT_WIDTH) {
				/* 显示输入的字符 */
				cons_putchar(cons, i - 256, 1);
			}
			task->readKeyboard = 0;
			return i - 256;
		}
	}
}
