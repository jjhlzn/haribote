STACK    = 132k
MALLOC   = 64k

TOOLPATH = ../../../tolset/z_tools/
INCPATH  = ../../../tolset/z_tools/haribote/
APILIBPATH   = ../apilib/
HARIBOTEPATH = ../haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -I$(HARIBOTEPATH) -I../haribote/include/ -I../ -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIM2BIN  = $(TOOLPATH)bim2bin.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# デフォルト動作

default :
	$(MAKE) testfs.hrb

# ファイル生成規則


haribote.img : ../haribote/boot/ipl09.bin ../haribote/haribote.sys testfs.hrb create.hrb write.hrb \
		Makefile ../app_make.txt 
	$(EDIMG)   imgin:../../../tolset/z_tools/fdimg0at.tek \
		wbinimg src:../haribote/boot/ipl09.bin len:512 from:0 to:0 \
		copy from:../haribote/haribote.sys to:@: \
		copy from:testfs.hrb to:@: \
		copy from:create.hrb to:@: \
		copy from:write.hrb to:@: \
		copy from:../nihongo/nihongo.fnt to:@: \
		imgout:haribote.img

# 一般規則

%.gas : %.c ../apilib.h Makefile ../app_make.txt
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile ../app_make.txt
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile ../app_make.txt
	$(NASK) $*.nas $*.obj $*.lst
	
%.bim : %.obj $(APILIBPATH)apilib.lib Makefile ../app_make.txt
	$(OBJ2BIM) @$(RULEFILE) out:$*.bim map:$*.map stack:$(STACK) \
		$*.obj $(APILIBPATH)apilib.lib

%.org : %.bim Makefile ../app_make.txt
	$(BIM2HRB) $*.bim $*.org $(MALLOC)
	
%.hrb : %.org Makefile ../app_make.txt
	$(BIM2BIN) -osacmp in:$*.org out:$*.hrb

# コマンド

run :
	$(MAKE) haribote.img
	$(COPY) haribote.img ..\..\..\tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ..\..\..\tolset/z_tools/qemu

full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) testfs.hrb

run_full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) -C ../haribote
	$(MAKE) run

clean :
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.org
	-$(DEL) haribote.img

src_only :
	$(MAKE) clean
	-$(DEL) testfs.hrb
