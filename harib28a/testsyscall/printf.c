#define STDOUT 1
int main(int argc, char **argv)
{
	char msg[100];
	sprintf(msg,"hello world!\n");
	write(STDOUT, msg, strlen(msg));
	//printf("%s", "hellow world!");
	return 0;
}
