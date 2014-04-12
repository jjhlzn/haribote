#include <stdio.h>

int main(int arg, char **argv)
{
	int sum;
	int v;
	sum = 0;
	while (scanf("%3d", &v) == 1)
		printf("\t%.d\n", sum += v);
	return 0;
}
