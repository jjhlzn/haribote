#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

void HariMain(void){
	int fd_stdin  = open("/test.c", O_RDWR);
	int fd_stdout = open("/test.c", O_RDWR);

	int pid = fork();
	if (pid != 0) { /* parent process */
		printf("parent is running, child pid:%d\n", pid);
		//spin("parent");
	}
	else {	/* child process */
		printf("child is running, pid:%d\n", getpid());
		//spin("child");
	}
}
