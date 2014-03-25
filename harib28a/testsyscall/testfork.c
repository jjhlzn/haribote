#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

void HariMain(void){
	//int fd_stdin  = api_open("/test.c", O_RDWR);
	//int fd_stdout = open("/test.c", O_RDWR);
	int pid;
	char str[100];
	
	char pathname[20];
	sprintf(pathname,"/test.c");
	int fd = api_open(pathname, O_CREATE | O_RDWR);
	if(fd <0){
		fd = api_open(pathname, O_RDWR);
	}
	
	//short i = -32768;
	//sprintf(str,"i = %u\n", (unsigned short)i);
	//api_putstr0(str);
	sprintf(str,"add of str = %d\n",(int)str);
	api_putstr0(str);
    pid = fork();
	
	if (pid != 0) { /* parent process */
		sprintf(str,"parent is running, child pid:%d\n", pid);
		api_putstr0(str);
		char bufw[100];
		int n = api_read(fd,bufw,25);
		api_putstr0("father said: ");
		api_putstr0(bufw);
		api_putstr0("\n");
		//spin("parent");
		while(1);
	}
	else {	/* child process */
		pid = getpid();
		sprintf(str,"child is running, pid:%d\n", pid);
		api_putstr0(str);
		//spin("child");
		char bufw[100];
		int n = api_read(fd,bufw,25);
		api_putstr0("child said: ");
		api_putstr0(bufw);
		api_putstr0("\n");
		int i=1000000;
		while(i--);
		api_end();
	}
}
