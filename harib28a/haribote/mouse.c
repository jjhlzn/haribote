/* 儅僂僗娭學 */

#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

static struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
static char strbuf[200];
static long i = 0;
void inthandler2c(int *esp)
/* PS/2儅僂僗偐傜偺妱傝崬傒 */
{
	int data;
	io_out8(PIC1_OCW2, 0x64);	/* 通知PIC1 IRQ-12的受理已经完成  0x64 = 0110 0100*/
	io_out8(PIC0_OCW2, 0x62);	/* 通知PIC0 IRQ-02的受理已经完成  0x62 = 0110 0010 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
	/* 彂偒崬傒愭偺FIFO僶僢僼傽傪婰壇 */
	mousefifo = fifo;
	mousedata0 = data0;
	/* 激活鼠标 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	/* 偆傑偔偄偔偲ACK(0xfa)偑憲怣偝傟偰偔傞 */
	mdec->phase = 0; /* 儅僂僗偺0xfa傪懸偭偰偄傞抜奒 */
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* 儅僂僗偺0xfa傪懸偭偰偄傞抜奒 */
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* 儅僂僗偺1僶僀僩栚傪懸偭偰偄傞抜奒 */
		if ((dat & 0xc8) == 0x08) {
			/* 惓偟偄1僶僀僩栚偩偭偨 */
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/* 儅僂僗偺2僶僀僩栚傪懸偭偰偄傞抜奒 */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/* 儅僂僗偺3僶僀僩栚傪懸偭偰偄傞抜奒 */
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /* 儅僂僗偱偼y曽岦偺晞崋偑夋柺偲斀懳 */
		return 1;
	}
	return -1; /* 偙偙偵棃傞偙偲偼側偄偼偢 */
}
