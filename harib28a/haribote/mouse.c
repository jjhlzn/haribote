/* ƒ}ƒEƒXŠÖŒW */

#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

static struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
static char strbuf[200];
static long i = 0;
void inthandler2c(int *esp)
/* PS/2ƒ}ƒEƒX‚©‚ç‚ÌŠ„‚è‚İ */
{
	int data;
	io_out8(PIC1_OCW2, 0x64);	/* IRQ-12ó•tŠ®—¹‚ğPIC1‚É’Ê’m */
	io_out8(PIC0_OCW2, 0x62);	/* IRQ-02ó•tŠ®—¹‚ğPIC0‚É’Ê’m */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	
	sprintf(strbuf,"mouse interrupt %d happen", i++);
	boxfill8(binfo->vram,binfo->scrnx, COL8_848484, 10, 500+16+16, 500+8*50, 500+16+16+16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 10, 500+16+16, COL8_000000, strbuf);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
	/* ‘‚«‚İæ‚ÌFIFOƒoƒbƒtƒ@‚ğ‹L‰¯ */
	mousefifo = fifo;
	mousedata0 = data0;
	/* ¼¤»îÊó±ê */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	/* ‚¤‚Ü‚­‚¢‚­‚ÆACK(0xfa)‚ª‘—M‚³‚ê‚Ä‚­‚é */
	mdec->phase = 0; /* ƒ}ƒEƒX‚Ì0xfa‚ğ‘Ò‚Á‚Ä‚¢‚é’iŠK */
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* ƒ}ƒEƒX‚Ì0xfa‚ğ‘Ò‚Á‚Ä‚¢‚é’iŠK */
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* ƒ}ƒEƒX‚Ì1ƒoƒCƒg–Ú‚ğ‘Ò‚Á‚Ä‚¢‚é’iŠK */
		if ((dat & 0xc8) == 0x08) {
			/* ³‚µ‚¢1ƒoƒCƒg–Ú‚¾‚Á‚½ */
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/* ƒ}ƒEƒX‚Ì2ƒoƒCƒg–Ú‚ğ‘Ò‚Á‚Ä‚¢‚é’iŠK */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/* ƒ}ƒEƒX‚Ì3ƒoƒCƒg–Ú‚ğ‘Ò‚Á‚Ä‚¢‚é’iŠK */
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
		mdec->y = - mdec->y; /* ƒ}ƒEƒX‚Å‚Íy•ûŒü‚Ì•„†‚ª‰æ–Ê‚Æ”½‘Î */
		return 1;
	}
	return -1; /* ‚±‚±‚É—ˆ‚é‚±‚Æ‚Í‚È‚¢‚Í‚¸ */
}
