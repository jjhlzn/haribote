#include "bootpack.h"
#include "hd.h"
#include "fs.h"
#include <string.h>


void cmd_ls(struct CONSOLE *cons){
	//获取文件列表
	int dev = MINOR(ROOT_DEV);
	debug("dev = %d",dev);
	struct FILEINFO *p_file = get_all_files(dev);
	char str[100];
	while(p_file){
		sprintf(str,"%s   %d",p_file->filename,p_file->size);
		cons_putstr0(cons,str);
	}
}
