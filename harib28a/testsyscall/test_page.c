#include <stdio.h>

int main()
{
	unsigned long address = 0x1600004;
	unsigned long * page_table = (unsigned long *) ((address>>20) & 0xffc);
	
	printf("page_table = %d",(unsigned long)page_table);
	return 0;
}