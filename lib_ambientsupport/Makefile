# $Id: Makefile,v 1.2 2021/11/06 17:56:08 piru Exp $

ROOTPATH=../../../
include $(ROOTPATH)Makefile.global
include $(ROOTPATH)SDK.global

CFLAGS+=-Iinclude -Wall -W -DAROS_ALMOST_COMPATIBLE -DSYSTEM_PRIVATE -O2 -I../../development/os-include -Ios-include
TARGET = ambientsupport.library

CFLAGSVEC += -fvec -maltivec

CC = ppc-morphos-gcc-9

OBJS = dummy.o library.o debug.o \
		internal/math_sqrt.o    \
		internal/altivec.o      \
		func_gfx/gfx_alpha.o    \
		func_gfx/gfx_bitmap.o   \
		func_gfx/gfx_analyze.o  \
		func_gfx/gfx_blit.o     \
		func_gfx/gfx_blur.o     \
		func_gfx/gfx_cmap.o     \
		func_gfx/gfx_dbuf.o     \
		func_gfx/gfx_linedraw.o \
		func_gfx/gfx_mask.o     \
		func_gfx/gfx_pen.o      \
		func_gfx/gfx_scale.o    \

all: os-include/ppcinline/ambientsupport.h os-include/proto/ambientsupport.h $(TARGET)

$(TARGET): includes $(OBJS)
	$(LINKECHO)
	$(LINKPREFIX)$(CC) $(CFLAGS) $(CFLAGSVEC) -noixemul -nostartfiles  $(OBJS) -o $@.db -ldebug
	$(LINKPREFIX)$(STRIP) $(STRIPFLAGS) $@.db -o $@
	$(LINKPREFIX)chmod u+x $@

clean:
	rm -f $(TARGET) *.db *.s $(OBJS)

dump:
	objdump --disassemble-all --reloc $(TARGET).db >$(TARGET).s

install: all
	@mkdir -p /sys/morphos/Libs
	cp $(TARGET) /sys/morphos/Libs/
	-flushlib $(TARGET)

install-iso: all
	mkdir -p $(ISOPATH)MorphOS/Libs
	cp $(TARGET) $(ISOPATH)MorphOS/Libs/

includes:
	cp os-include/clib/ambientsupport_protos.h ../../development/os-include/clib/
	cp os-include/workbench/ambientsupport.h   ../../development/os-include/workbench/
	cp os-include/ppcinline/ambientsupport.h   ../../development/os-include/ppcinline/
	cp os-include/proto/ambientsupport.h       ../../development/os-include/proto/
	cp os-include/fd/ambientsupport_lib.fd     ../../development/os-include/fd/

includesgg:
	cp os-include/clib/ambientsupport_protos.h /gg/os-include/clib/
	cp os-include/workbench/ambientsupport.h   /gg/os-include/workbench/
	cp os-include/ppcinline/ambientsupport.h   /gg/os-include/ppcinline/
	cp os-include/proto/ambientsupport.h       /gg/os-include/proto/
	cp os-include/fd/ambientsupport_lib.fd     /gg/os-include/fd/

source:


sdk: os-include/fd/ambientsupport_lib.fd os-include/workbench/ambientsupport.h
	@$(AUTODOC) $(AUTODOCFLAGS) -o ambientsupport.$(AUTODOCPOSTFIX) $(wildcard *.c)
	mkdir -p $(SDKPATH)$(SDKROOT)$(SDKDOCS)
	cp ambientsupport.$(AUTODOCPOSTFIX)         $(SDKPATH)$(SDKROOT)$(SDKDOCS)

os-include/ppcinline/ambientsupport.h: os-include/fd/ambientsupport_lib.fd os-include/clib/ambientsupport_protos.h
	@mkdir -p os-include/ppcinline
	cvinclude.pl --fd os-include/fd/ambientsupport_lib.fd --clib os-include/clib/ambientsupport_protos.h --inline $@

os-include/proto/ambientsupport.h: os-include/fd/ambientsupport_lib.fd
	@mkdir -p os-include/proto
	cvinclude.pl --fd os-include/fd/ambientsupport_lib.fd --clib os-include/clib/ambientsupport_protos.h --proto $@

sdkpriv:
	cp ambientsupport_protos.h GG:os-include/clib/
	cp ambientsupport_lib.fd   GG:os-include/fd/
	cp ambientsupport.h        GG:os-include/workbench/

dep:
	echo >Makefile.dep
	$(CC) -MM $(CFLAGS) $(CFLAGSVEC) `ls *.c` >>Makefile.dep

-include Makefile.dep
