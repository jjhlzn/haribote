#include "kernel.h"
#include "apilib.h"

#include <stdio.h>
#include <string.h>

void HariMain()
{
	int win, xsize = 400, ysize = 200;
	char win_buf[xsize * ysize];
	struct MOUSE_INFO minfo;
	minfo.flag = -1;
	int counter = 0;
	int index = 0;
	char cbuf[20];
	
	win = api_openwin((char *)win_buf, xsize, ysize, 0 /*黑色*/, "minesweeper");
    api_refreshwin(win, 0, 0, xsize-1, ysize-1);

	char s[20];
	for(;;)
	{
		counter++;
		sprintf(cbuf,"%10d",counter);
		api_boxfilwin(win, 30, 30, 30+15*8, 30+16, 15/*也不知道是什么颜色*/);
		api_putstrwin(win, 30, 30, 0/*黑色*/, strlen(cbuf) /*20 字符串长度*/,  cbuf);
		
		api_getmouse(1, &minfo, win);
		
		if(minfo.flag != -1)
		{
			sprintf(s,"%d %5d %5d, [l m r]", ++index, minfo.x, minfo.y);
			if( minfo.btn & 0x01 ){
				s[14] = 'L';
			}
			if( minfo.btn & 0x02 ){
				s[16] = 'M';
			}
			if( minfo.btn & 0x04 ){
				s[18] = 'R';
			}
			api_boxfilwin(win, 30, 50, 30+strlen(s)*8, 50+16, 15/*也不知道是什么颜色*/);
			api_putstrwin(win, 30, 50, 0/*黑色*/, strlen(s) /*20 字符串长度*/,  s);
			api_refreshwin(win, 0,0,xsize-1,ysize-1);
		}
	}
	api_closewin(win);
	api_end();
}
