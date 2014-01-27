
#include "apilib.h"
#include <string.h>

void HariMain(){
	
	char pathname[50];
	sprintf(pathname,"/test.c");
	api_putstr0(pathname);
	int mode = 1;
	int fd = api_open(pathname,mode);
	
	int n = 0;
	char bufw[100], bufr[100];
	sprintf(bufw,"my name is jinjunhang!");
	n = api_write(fd,bufw,strlen(bufw));
	api_close(fd);
	
	fd = api_open(pathname,2);
	n = api_read(fd,bufr,22);
	bufr[n] = 0;
	api_putstr0(bufr);
	
	api_close(fd);
	api_end();
}
