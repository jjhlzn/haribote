#include "kernel.h"
#include "apilib.h"

#include <stdio.h>
#include <string.h>

void HariMain()
{
	int win, xsize = 400, ysize = 200;
	char win_buf[xsize * ysize];
	api_initmalloc();
	
	struct MOUSE_INFO *minfo;
	minfo = api_malloc(sizeof(struct MOUSE_INFO));
	minfo->flag = -589;
	int counter = 0;
	int index = 0;
	char cbuf[20];
	
	win = api_openwin((char *)win_buf, xsize, ysize, -1 /*ºÚÉ«*/, "minesweeper");
    api_refreshwin(win, 0, 0, xsize-1, ysize-1);

	char s[20];
	for(;;)
	{
		
		sprintf(cbuf, "%10d", counter);
		api_boxfilwin(win, 30, 30, 30 + strlen(cbuf) * 8, 30 + 16, 8/*ÁÁ»Ò*/);
		api_putstrwin(win, 30, 30, 0 /*ºÚÉ«*/, strlen(cbuf),  cbuf);
		
		api_getmouse(1, minfo, win);
			
		if(minfo->flag == 1)
		{
			sprintf(s,"%5d %5d, [l m r]", minfo->x, minfo->y);
			//sprintf(s,"%d", (int)minfo);
			if( minfo->btn & 0x01 != 0 ){
				s[14] = 'L';
			}
			if( minfo->btn & 0x02 != 0 ){
				s[16] = 'M';
			}
			if( minfo->btn & 0x04 != 0 ){
				s[18] = 'R';
			}
			api_boxfilwin(win, 30, 50, 30 + strlen(s) * 8, 50 + 16, 8/*ÁÁ»Ò*/);
			api_putstrwin(win, 30, 50, 0, strlen(s),  s);
			minfo->flag = -1;
		}
		
		//api_free(minfo, sizeof(struct MOUSE_INFO));
		counter++;
		
		// if (api_getkey(1) == 128) {
			// break;
		// }
	}
	
		
	api_closewin(win);
	api_end();
}
