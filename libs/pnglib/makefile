#
# Ambient (workbench.library) - © 2001-2004 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# All Rights Reserved
#
# $Id: makefile,v 1.9 2017/07/25 20:36:32 piru Exp $
#

APPNAME = png.alib

include ../../makefile.conf
include makefile.objs
include makefile.libs

SOURCE = ../../
VPATH := $(SOURCE)

EXE = $(APPNAME)

OBJS = $(COBJS_NODEP) $(COBJS)

ifndef CPU-TYPES
CPU-TYPES = $(CPU_DEFAULT)
endif

RELEASE ?= BETA_RELEASE

# Detect shared libs
ifndef LIBPNG
ifeq ($(wildcard ../../config.h),../../config.h)
export LIBPNG := $(shell grep "\#define USE_SHARED_LIBPNG" ../../config.h | grep -c 0)
else
export LIBPNG := 1
endif
endif

INCLUDES = -I$(SOURCE). -I$(SOURCE)../../include
DEPINCLUDES = -I. -I../../include

DEPOPTS = -noixemul -DDEPEND -DDEBUG

LIBPNG_FEATURES = \
	-DPNG_NO_MNG_FEATURES \
	-DPNG_NO_CONSOLE_IO \
	-DPNG_NO_STDIO \
	-DPNG_iTXt_SUPPORTED \
	-DPNG_NO_PROGRESSIVE_READ \
	-DPNG_READ_BIG_ENDIAN_SUPPORTED \
	-DPNG_NO_READ_sCAL \
	-DPNG_NO_WRITE_sCAL \
	-DPNG_NO_FLOATING_POINT_SUPPORTED \
	-DPNG_NO_WRITE_SUPPORTED \
	-DPNG_NO_MMX_CODE \
	-DPNG_ABORT=local_abort

CFLAGS = -noixemul -c -O2 $(LIBPNG_FEATURES) -D$(RELEASE) -DSYSTEM_PRIVATE -fomit-frame-pointer -mcpu=$(CPU) \
	$(WARNINGS) $(INCLUDES)
LDFLAGS = -nostdlib -noixemul -o

.PHONY: all
ifeq ($(LIBPNG),0)
all:
	@echo "Not enabled, not building"
else
ifeq ($(wildcard $(DEPFILE)),$(DEPFILE))
all: build
else
all: depend
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -f $(MAKEFILENAME) build
endif
endif

.PHONY: build
build: ppcinline/png.h proto/png.h $(LOBJS)
#	$(OUT)$(MAKE) $(NODIR) $(SILENT) -C libpng build FEATURES="$(LIBPNG_FEATURES)" CPU=$(CPU_DEFAULT)
	@for i in $(CPU-TYPES); \
		do (echo -n "==> [$(APPNAME)/"; \
			echo -n $$i; \
			echo "]"; \
			$(MKDIR) $(BUILDDIR)/$$i; \
			cd $(BUILDDIR)/$$i; \
			$(MAKE) $(NODIR) $(SILENT) -f $(SOURCE)$(MAKEFILENAME) flat -I$(SOURCE) CPU=`echo $$i | sed 's/\-.*//'`) ; echo "<== [$(APPNAME)/$$i]"; done

.PHONY: flat
ifeq ($(RELEASE), FINAL_RELEASE)
flat: $(EXE)
else
flat: $(EXE).db
endif

ifeq ($(RELEASE), FINAL_RELEASE)
$(EXE): $(OBJS)
	$(ECHO) -n "LD -> $(subst $(SOURCE),,$@)"
	$(OUT)$(CC) $(LDFLAGS) $(EXE) $(OBJS) $(SOURCE)$(LOBJS) $(LIBS)
	$(ECHO) ""
	$(OUT)$(STRIP) --remove-section=.comment $(EXE)
else
$(EXE).db: $(OBJS)
	$(ECHO) -n "LD -> $(subst $(SOURCE),,$@)"
	$(OUT)$(CC) -Wl,--cref,-Map,$(EXE).map $(LDFLAGS) $(EXE).db $(OBJS) $(SOURCE)$(LOBJS) $(LIBS)
	$(ECHO) ""
	$(ECHO) -n "STRIP -> $(EXE)"
	$(OUT)$(STRIP) --remove-section=.comment -o $(SOURCE)$(EXE) $(EXE).db
	$(ECHO) ""
endif

.PHONY: depend
depend:
	$(OUT)../../rev
	$(ECHO)	-n "generating dependencies.."
	$(OUT)sh ../../mkdep.sh -f $(DEPFILE) $(DEPOPTS) $(DEPINCLUDES) $(patsubst %.o,%.c, $(COBJS))
	$(ECHO) " done"

.PHONY: clean
clean:
	$(ECHO) "==> [pnglib]"
	$(ECHO) -n "cleaning.."
	$(OUT)$(DELETEALL) $(BUILDDIR) $(EXE)
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -C libpng clean
	$(ECHO) ""
	$(ECHO) "<== [pnglib]"

.PHONY: mrproper
mrproper: clean
	$(OUT)$(DELETEALL) $(DEPFILE) proto ppcinline $(EXE) rev.h

.PHONY: dump
dump:
	$(OBJDUMP) --section-headers --all-headers --reloc --disassemble-all --syms $(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).db >$(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).dump

.PHONY: install
ifeq ($(LIBPNG),0)
install:
	@echo "png.alib not enabled, not installing"
else
install: all
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/Ambient/libs"
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/Ambient/libs
	$(OUT)$(COPY) $(EXE) $(INSTALLDIR)morphos/Ambient/libs
ifeq ($(shell uname), MorphOS)
	@-/mossys/c/flushlib $(APPNAME) quiet
endif
endif

.PHONY: update
update:
	$(OUT)../../rev INCREV
	$(OUT)touch copyright.h

-include $(DEPFILE)
include makefile.rules

