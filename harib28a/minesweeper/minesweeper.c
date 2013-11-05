#include "apilib.h"
#include <stdio.h>

struct MOUSE_INFO {
	int x, y;
	char button;
}

void HariMain()
{
	int win, xsize = 200, ysize = 100;
	char *win_buf;
	char win_buf = char[200 * 100];
	
	win = api_openwin(win_buf, xsize, ysize, 0 /*黑色*/, "minesweeper");
    api_refresh(win, 0, 0, xsize-1, ysize-1);
	
	struct MOUSE_INFO *mouse_info;
	char s[20];
	for(;;)
	{
		mouse_info = api_getmouse();
		if(mouse_info != 0)
		{
			sprintf("%5d %5d, [l m r]", mouse_info->x, mouse_info->y);
			if( mouse_info->button & 0x01 ){
				s[14] = 'L';
			}
			if( mouse_info->button & 0x02 ){
				s[16] = 'M';
			}
			if( mouse_info->button & 0x04 ){
				s[18] = 'R';
			}
			api_putstrwin(win, 10, 10, 15 /*也不知道是什么颜色*/, strlen(s) /*20 字符串长度*/,  s);
			api_refresh(win, 0, 0, xsize-1, ysize-1);
		}
	}
}