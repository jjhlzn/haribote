#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

void HariMain(void){
	int pid;
	char str[100];
    pid = fork();
	if (pid != 0) { /* parent process */
		sprintf(str,"parent is running, child pid:%d\n", pid);
		printf(str);
		int s = -1;
		int child_pid = wait(&s);
		sprintf(str,"child_pid = %d, status = %d\n", child_pid, s);
		printf(str);
		api_end();
	} 
	else {	/* child process */
		pid = getpid();
		sprintf(str,"child is running, pid:%d\n", pid);
		printf(str);
		char bufw[100];
		char ** argv = 0;
		execv("hello4.hrb", argv);
		api_end();
	}
}
