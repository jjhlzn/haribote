#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_LENGTH 1024

int main(int argc, char *argv[]) {
  char line[MAX_LENGTH];

  while (1) {
    printf("# ");
    if (!fgets(line, MAX_LENGTH, stdin)) break;
    int pid = fork();
    if(pid){  //parent process
		int status = 0;
		pid = wait(&status);
		printf("child process[%d] exit with status[%d]\n",pid,status);
	}else{
		line[strlen(line)-1] = 0;
		printf("line=[%s]\n",line);
		char *name[2];
		int i;
		for(i=0; i<2; i++){
			name[i] = NULL;
		}
		name[0] = line;
		execvp(name[0],name);
		perror("");
		exit(0);
	}
  }
  return 0;
}
