#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

void HariMain(void){
	int pid;
	char str[100];
	
	char pathname[20];
	sprintf(pathname,"/test.c");
	//int fd = api_open(pathname, O_CREATE | O_RDWR);
	//if(fd <0){
	//	fd = api_open(pathname, O_RDWR);
	//}
	
	sprintf(str,"add of str = %d\n",(int)str);
	api_putstr0(str);
    pid = fork();
	
	if (pid != 0) { /* parent process */
		sprintf(str,"parent is running, child pid:%d\n", pid);
		api_putstr0(str);
		char bufw[100];
		//int n = api_read(fd,bufw,25);
		//api_putstr0("father said: ");
		//api_putstr0(bufw);
		//api_putstr0("\n");
		//spin("parent");
		//while(1);
		int s = -1;
		//sprintf(str,"addr = %d", &s);
		//api_putstr0(str);
		int child_pid = wait(&s);
		sprintf(str,"child_pid = %d, status = %d\n", child_pid, s);
		api_putstr0(str);
		api_end();
	}
	else {	/* child process */
		pid = getpid();
		sprintf(str,"child is running, pid:%d\n", pid);
		api_putstr0(str);
		//spin("child");
		char bufw[100];
		//int n = api_read(fd,bufw,25);
		//api_putstr0("child said: ");
		//api_putstr0(bufw);
		//api_putstr0("\n");
		//int i=1000000;
		//while(i--);
		char ** argv = 0;
		//execv("hello4.hrb", argv);
		//while(1);
		api_end();
	}
}
