#include "apilib.h"
#include <stdio.h>
#include <string.h>


#define O_CREATE 1
#define O_RDWR 2

void HariMain(){
	char pathname[50];
	sprintf(pathname,"/test.c");
	int fd_read = -1;
	api_putstr0(pathname);
	int fd_write = api_open(pathname,O_CREATE | O_RDWR);
	if(fd_write <0){
		fd_write = api_open(pathname, O_RDWR);
		fd_read  = api_open(pathname, O_RDWR);
	}else{
		fd_read = api_open(pathname, O_RDWR);
	}
	
	if(fd_write == -1){
		api_putstr0("can't open file for write");
		return;
	}
	
	if(fd_read == -1){
		api_putstr0("can't open file for read");
		return;
	}
	
	int n = 0;
	char bufw[100];
	
	
	int i = 2;
	while(i--){
		sprintf(bufw,"my name is jinjunhang! %d ", i);
		n = api_write(fd_write,bufw,strlen(bufw));
		n = api_read(fd_read,bufw,25);
		api_putstr0(bufw);
		api_putstr0("\n");
	}
	api_close(fd_read);
	api_close(fd_write);
	
	api_end();
}
