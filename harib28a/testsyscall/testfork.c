#include "apilib.h"

#define O_CREATE 1
#define O_RDWR 2

//#define set_bit(nr,addr) ({\
//register int res __asm__("ax"); \
//__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
					 "=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
//res;})

void HariMain(void){
	//int pid;
	//char str[100];
	
//int i = 0;
//set_bit(0,&i);
//sprintf(str,"i = %d\n",i);
//api_putstr0(str);
	
	//char * mem = api_malloc( 512 * 1024);
	//sprintf(str,"mem = %d\n",(int)mem);
	//api_putstr0(str);
	//mem[512 * 1024 - 3] = 'a';
	//mem[512 * 1024 - 2] = 'd';
	//mem[512 * 1024 - 1] = 0;
	//sprintf(str,"mem[end] = %s\n", mem+(512 * 1024 - 3));
	//api_putstr0(str);
	
    //pid = fork();
	//if (pid != 0) { /* parent process */
	//	sprintf(str,"parent is running, child pid:%d\n", pid);
	//	printf(str);
	//	int s = -1;
	//	int child_pid = wait(&s);
	//	sprintf(str,"child_pid = %d, status = %d\n", child_pid, s);
	//	printf(str);
	//	api_end();
	//} 
	//else {	/* child process */
	//	pid = getpid();
	//	sprintf(str,"child is running, pid:%d\n", pid);
	//	printf(str);
	//	char bufw[100];
	//	char ** argv = 0;
	//	execv("hello4.hrb", argv);
	//	api_end();
	//}
}
