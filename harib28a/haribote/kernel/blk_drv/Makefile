OBJS =  hd.obj ll_rw_blk.obj

TOOLPATH = ../../../../../tolset/z_tools/
INCPATH  = ../../../../../tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -Werror -quiet
GCC      = $(TOOLPATH)gcc.exe -Wall  -O1
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

#%.s : %.c ../../include/bootpack.h Makefile
#	$(GCC) -I../../include  -S -o $*.s $*.c
	
%.obj : %.c ../../include/bootpack.h Makefile
	$(GCC) -I../../include  -c -o $*.obj $*.c

	
#%.gas : %.c ../../include/bootpack.h Makefile
#	$(CC1) -I../../include  -o $*.gas $*.c

#%.nas : %.gas Makefile
#	$(GAS2NASK) $*.gas $*.nas

#%.obj : %.nas Makefile
#	$(NASK) $*.nas $*.obj $*.lst
	
clean :
	-$(DEL) *.lst
	-$(DEL) *.obj
