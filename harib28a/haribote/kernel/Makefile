OBJS =  bootpack.obj naskfunc.obj hankaku.obj dsctbl.obj int.obj fifo.obj \
		keyboard.obj mouse.obj mtask.obj console.obj file.obj tek.obj \
		exec.obj linkedlist.obj timer.obj forkexit.obj utils.obj syscall.obj

TOOLPATH = ../../../../tolset/z_tools/
INCPATH  = ../../../../tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
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
	$(MAKE) hankaku.obj
	$(MAKE) -C blk_drv
	copy /B bmp.src bmp.obj
	copy /B jpeg.src jpeg.obj
	$(MAKE) -C ../ haribote.sys 
	
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku
	
%.gas : %.c ../include/bootpack.h Makefile
	$(CC1) -I../include -I./blk_drv -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst
	
clean :
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) *.bin
	$(MAKE) -C blk_drv clean
