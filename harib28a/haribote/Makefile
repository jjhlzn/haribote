OBJS_BOOTPACK = kernel/bootpack.obj kernel/naskfunc.obj kernel/hankaku.obj kernel/dsctbl.obj kernel/int.obj kernel/fifo.obj \
				kernel/keyboard.obj kernel/mouse.obj kernel/mtask.obj kernel/console.obj kernel/file.obj kernel/tek.obj kernel/utils.obj kernel/syscall.obj \
				kernel/bmp.obj kernel/jpeg.obj kernel/blk_drv/hd.obj kernel/exec.obj kernel/linkedlist.obj kernel/timer.obj kernel/forkexit.obj kernel/apploader.obj \
				window/graphic.obj  window/window.obj window/sheet.obj \
				mm/memory.obj   \
				fs/fs_misc.obj fs/fs_open.obj fs/fs_read_write.obj fs/fs.obj  \
				../apilib/alloca.obj

TOOLPATH = ../../../tolset/z_tools/
INCPATH  = ../../../tolset/z_tools/haribote/

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

# デフォルト動作

default :
	$(MAKE) -C boot
	$(MAKE) -C fs
	$(MAKE) -C window
	$(MAKE) -C mm
	$(MAKE) -C kernel
	$(MAKE) haribote.sys 

bootpack.bim : $(OBJS_BOOTPACK)  Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : boot/asmhead.bin bootpack.hrb Makefile
	copy /B C:\Users\jjh\Documents\GitHub\haribote\harib28a\haribote\boot\asmhead.bin+bootpack.hrb haribote.sys

clean :
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) haribote.sys
	$(MAKE) -C boot clean
	$(MAKE) -C fs clean
	$(MAKE) -C kernel clean
	$(MAKE) -C mm clean
	$(MAKE) -C window clean

src_only :
	$(MAKE) clean
	-$(DEL) ipl09.bin
	-$(DEL) haribote.sys
