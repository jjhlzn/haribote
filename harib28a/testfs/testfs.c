
#include "apilib.h"
#include <string.h>

void HariMain(){
	
	char pathname[50];
	sprintf(pathname,"/test.c");
	api_putstr0(pathname);
	int mode = 1;
	int fd = api_open(pathname,mode);
	api_close(fd);
	fd = api_open(pathname,2);
	api_close(fd);
	api_end();
}
