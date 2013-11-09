#include "apilib.h"

#include <stdio.h>
#include <string.h>

void HariMain()
{
	test1();
}

void test1(){
	int win, xsize = 400, ysize = 200;
	char *win_buf;
	api_initmalloc();
	win_buf = api_malloc(xsize * ysize);
	
	win = api_openwin((char *)win_buf, xsize, ysize, -1 /*ºÚÉ«*/, "test");
    api_refreshwin(win, 0, 0, xsize-1, ysize-1);

	char s[20];
	int key = 0;
	int x = 1, y = 50;
	for(;;)
	{
		key = api_getkey(1);
		s[0] = (char)key;
		s[1] = 0;
		api_putstrwin(win, x, y, 0, strlen(s),s);
		unsigned int x = -1;
		sprintf(s,"x = %x, x = %u",x, x);
		api_boxfilwin (win, 1, y+16, 1+3*8, y+16+16, 6);
		api_putstrwin(win, 1, y+16, 0, strlen(s), s);
		x += 8;
	}
	
	api_closewin(win);
	api_end();
}
