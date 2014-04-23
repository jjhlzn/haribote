OBJS_API =	api001.obj api004.obj api005.obj alloca.obj

TOOLPATH = ../../../tolset/z_tools/
INCPATH  = ../../../tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GCC      = $(TOOLPATH)gcc.exe -I$(INCPATH) -Os -Wall -quiet
AR       = $(TOOLPATH)ar.exe rs

NASM      = $(TOOLPATH)nasm.exe -f elf
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

# �f�t�H���g����

default :
	$(MAKE) apilib.lib

# �t�@�C�������K��

apilib.lib : Makefile $(OBJS_API)
	$(AR) apilib_linux.lib $(OBJS_API) 

# ��ʋK��

%.obj : %.nas Makefile
	$(NASM) $*.nas -o $*.obj 

# �R�}���h

clean :
	-$(DEL) *.lst
	-$(DEL) *.obj

src_only :
	$(MAKE) clean
	-$(DEL) apilib.lib