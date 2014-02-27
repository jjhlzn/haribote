
#include "apilib.h"
#include <stdio.h>
#include <string.h>


#define O_CREATE 1
#define O_RDWR 2

void HariMain(){
	
	char pathname[50];
	sprintf(pathname,"/test.c");
	//api_putstr0(pathname);
	int fd = api_open(pathname,O_CREATE | O_RDWR);
	
	int n = 0;
	char bufw[100], bufr[100] = {0};
	sprintf(bufw,"my name is jinjunhang!");
	n = api_write(fd,bufw,strlen(bufw));
	fd = api_open(pathname, O_RDWR);
	n = api_read(fd,bufr,22);
	bufr[n] = 0;
	api_putstr0(bufr);
	
	api_close(fd);
	api_end();
}
