#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

void HariMain(void){
	//int fd_stdin  = api_open("/test.c", O_RDWR);
	//int fd_stdout = open("/test.c", O_RDWR);
	int pid;
	char str[100];

	//short i = -32768;
	//sprintf(str,"i = %u\n", (unsigned short)i);
	//api_putstr0(str);
	sprintf(str,"add of str = %d\n",(int)str);
	api_putstr0(str);
    pid = fork();
	getpid();
	if (pid != 0) { /* parent process */
		sprintf(str,"parent is running, child pid:%d\n", pid);
		api_putstr0(str);
		
		//spin("parent");
		while(1);
	}
	else {	/* child process */
		sprintf(str,"child is running, pid:%d\n", 0);
		api_putstr0(str);
		//spin("child");
		while(1);
	}
}
