OBJS =  bitmap.obj buffer.obj file_dev.obj file_table.obj inode.obj namei.obj open.obj read_write.obj super.obj truncate.obj 

TOOLPATH = ../../../../tolset/z_tools/
INCPATH  = ../../../../tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
NASM     = $(TOOLPATH)nasm.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -Werror -quiet
GCC      = $(TOOLPATH)gcc.exe  -Wall -O1
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

default:
	$(MAKE) $(OBJS)
	
%.s : %.c ../include/bootpack.h Makefile
	$(GCC) -I../include -I../kernel/blk_drv -S -o $*.s $*.c

%.obj : %.c ../include/bootpack.h Makefile
	$(GCC) -I../include -I../kernel/blk_drv -c -o $*.obj $*.c


#%.obj : %.gas Makefile
#	$(NASM) -f bin $*.gas -o $*.obj 

	
#%.gas : %.c ../include/bootpack.h Makefile
#	$(CC1) -I../include -I../kernel/blk_drv -o $*.gas $*.c

#%.nas : %.gas Makefile
#	$(GAS2NASK) $*.gas $*.nas

#%.obj : %.nas Makefile
#	$(NASK) $*.nas $*.obj $*.lst
	
#orange_fs_read_write.s : orange_fs_read_write.c ../include/bootpack.h Makefile
#	$(GCC) -I../include -I../kernel/blk_drv    -S -o orange_fs_read_write.s orange_fs_read_write.c
	
#orange_fs.obj : orange_fs.c ../include/bootpack.h Makefile
#	$(GCC) -I../include -I../kernel/blk_drv -fleading-underscore -c -o orange_fs.obj orange_fs.c
#orange_fs_misc.obj : orange_fs_misc.c ../include/bootpack.h Makefile
#	$(GCC) -I../include -I../kernel/blk_drv  -c -o orange_fs_misc.obj orange_fs_misc.c
#orange_fs_open.obj : orange_fs_open.c ../include/bootpack.h Makefile
#	$(GCC) -I../include -I../kernel/blk_drv  -c -o orange_fs_open.obj orange_fs_open.c
#orange_fs_read_write.obj : orange_fs_read_write.c ../include/bootpack.h Makefile
#	$(GCC) -I../include -I../kernel/blk_drv  -c -o orange_fs_read_write.obj orange_fs_read_write.c
	
clean :
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	
