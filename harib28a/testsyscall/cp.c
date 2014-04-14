#include <stdio.h>

int main(int argc, char **argv)
{
	if(argc != 3){
		printf("usage: cp src dest\n");
		return 0;
	}
	
    char *spath = argv[1];
	char *dpath = argv[2];
	
	FILE* ifd = fopen(spath,"r");
	if( ifd == NULL ){
		printf("can't open file[%s] to read!\n",spath);
		return 1;
	}
	
	FILE* ofd = fopen(dpath, "w");
	if( ifd == NULL){
		printf("can't open file[%s] to write!\n",dpath);
		return 1;
	}
	
	
	char buf[1024];
	int count;
	while( (count = fread(buf,1,1024,ifd)) != 0 ){
		fwrite(buf,1,count,ofd);
	}
	
	//int c;
	//while((c = getc(ifd)) != EOF ){
	//	putc(c, ofd);
	//}
	
	return 0;
}
