#include "apilib.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

void HariMain()
{
	char pathname[100];
	sprintf(pathname,"/usr/root/test.c");
	int fd = api_open(pathname, O_RDWR);
	if(fd < 0){
		printf("can't open file\n");
		return;
	}
	char buf[1024];
	memset(buf,0,1024);
	sprintf(buf,"line 1: test1\n");
	api_write(fd, buf, strlen(buf));
	memset(buf,0,1024);
	sprintf(buf,"line 2: test2\n");
	api_write(fd, buf, strlen(buf));
	
	int fd2 = api_open(pathname, O_RDWR);
	while(1){
		memset(buf,0,1024);
		int n = api_read(fd2, buf, 1024);
		if(n <= 0 )
			break;
		buf[n] = 0;
		printf(buf);
	}
	api_end();
}
