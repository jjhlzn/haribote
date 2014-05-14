#include "apilib.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

void HariMain(){
	char pathname[50];
	sprintf(pathname,"/usr/root/hello.c");
	int fd = api_open(pathname,O_RDWR);
	int n;
	char buf[1024];
	do{
		n = api_read(fd,buf,10);
		if( n <= 0 )
			break;
		buf[n] = 0;
		printf(buf);
	}while(1);
	
	//int fd_read = -1;
	//int fd_write = api_open(pathname,O_CREATE | O_RDWR);
	//if(fd_write <0){
	//	fd_write = api_open(pathname, O_RDWR);
	//	fd_read  = api_open(pathname, O_RDWR);
	//}else{
	//	fd_read = api_open(pathname, O_RDWR);
	//}
	
	//if(fd_write == -1){
	//	printf("can't open file for write");
	//	return;
	//}
	
	//if(fd_read == -1){
	//	printf("can't open file for read");
	//	return;
	//}
	
	//int n = 0;
	//char bufw[100];
	
	////int i = 2;
	////while(i--){
	//	//sprintf(bufw,"jinjunhang! %d ", i);
	//	//n = api_write(fd_write,bufw,strlen(bufw));
	//	n = api_read(fd_read,bufw,100);
	//	printf(bufw);
	//	
	////}
	//printf("\n");
	////api_close(fd_read);
	////api_close(fd_write);
	
	////sprintf(bufw,"stdout jinjunhang!");
	////api_write(STDOUT, "stdout jinjunhang", bufw);
	
	api_end();
}
