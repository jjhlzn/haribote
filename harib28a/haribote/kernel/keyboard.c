#include "bootpack.h"
#include "window.h"

struct FIFO32 *keyfifo;
int keydata0;

static int read_char_from_keyboard(int read_bytes);

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);	/* 通知PIC"IRQ-01已经处理完毕" */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, data + keydata0); /* 将按键发送到队列 */
	return;
}

////等待键盘控制电路准备完毕
void wait_KBC_sendready(void)
{
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	/* 设置放键盘数据的缓冲区，以及区别键盘数据的初始值 */
	keyfifo = fifo;
	keydata0 = data0;
	/* 初始化键盘控制电路 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}


////从键盘中读取数据，直到缓冲区满，或者读到回车
//参数：buf -- 放键盘数据的缓冲区，len -- 缓冲区大小
//返回：读取键盘数据的字节
int 
read_from_keyboard(char *buf, int len)
{
	if( len <= 0 ){
		return 0;
	}

	int ch = -1;
	int i = 0;
	int end = 0;
	while(1){
		if(len == 1){
			buf[i++] = 0;
			break;
		}
		ch = read_char_from_keyboard(i);
		//debug("ch = %d",ch);
		
		switch(ch){
			case -1:  //read error
				buf[i++] = 0;
				len--;
				end = 1;
				break;
			case 10:  //line feed
				buf[i++] = ch;
				len--;
				buf[i++] = 0;
				end = 1;
				break;
			case 8:  //backspace
				if(i > 0){
					i--;
				}
				break;
			default:
				buf[i++] = ch;
				len--;
		}
		
		if(end)
			break;
	};
	
	return i-1;
}

static int 
read_char_from_keyboard(int read_bytes)
{
	struct TASK *task = current;
	struct CONSOLE *cons = task->cons;
	cons->sht->read_kb_task = task;
	//debug("read_bytes = %d",read_bytes);
	//debug("read kb pid: %d", task->pid);
	task->readKeyboard = 1;
	int i;
	for (;;) {
		io_cli();
		if (fifo32_status(&task->ch_buf) == 0) {
			task_sleep(task);	/* FIFO中没有内容，睡眠等待 */
		}
		i = fifo32_get(&task->ch_buf);
		io_sti();
		if (i >= 256 && i<512) { /* 键盘按键 */
			if (cons->cur_x < CONSOLE_CONTENT_WIDTH) {
				/* 显示输入的字符 */
				int ch = i - 256;
				switch(ch){
					case 8:
						if (read_bytes > 0) {
							/* 将当前的字符变为空格 */
							cons->cur_x -= 8;
							cons->buf_x -= 8;
							cons_putchar(cons, ' ', 0);
							
						}
						break;
					default:
						cons_putchar(cons, ch, 1);
				}
			}
			task->readKeyboard = 0;
			return i - 256;
		}
	}
}
