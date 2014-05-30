#include <stdio.h>
#include <stdlib.h>

void main(int argc, char **argv){
	int pid;
	char str[100];
	//printf("before invoke fork()\n");
    	pid = fork();
	if (pid != 0) { /* parent process */
		sprintf(str,"p: parent[%d] is running, child pid:%d\n",getpid(), pid);
		printf(str);
		int s = 100;
		int child_pid = wait(&s);
		sprintf(str,"p: child_pid = %d, status = %d\n", child_pid, s);
		printf(str);
		exit(0);
	} 
	else {	/* child process */
		pid = getpid();
		sprintf(str,"c: child is running, pid:%d\n", pid);
		printf(str);
		char bufw[100];
		char ** argv = 0;
		//execv("hello4.hrb", argv);
		exit(0);
	}
}

