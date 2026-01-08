#
# Ambient (icon.library) - © 2001-2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# 
# All Rights Reserved
#
# $Id: makefile,v 1.14 2017/07/25 20:36:32 piru Exp $
#

APPNAME = icon.library

include ../makefile.conf
include makefile.objs
include makefile.libs

SOURCE = ../../
VPATH := $(SOURCE) $(SOURCE)../

EXE = $(APPNAME)

OBJS = $(COBJS_NODEP) $(COBJS_PARENT) $(COBJS)

ifndef CPU-TYPES
CPU-TYPES = $(CPU_DEFAULT)
endif

RELEASE ?= BETA_RELEASE

# Detect LEGACY build
ifndef LEGACY
ifeq ($(wildcard ../config.h),../config.h)
export LEGACY := $(shell grep "^\#define USE_LEGACY" ../config.h | grep -c 1)
endif
endif
# These are hardcoded now, unfortunately.
# The USE_LEGACY crap in config.h broke the old grepping.
ifeq ($(LEGACY),1)
ifndef LIBPNG
export LIBPNG  := 1
endif
ifndef LIBZ
export LIBZ    := 1
endif
else
ifndef LIBPNG
export LIBPNG  := 0
endif
ifndef LIBZ
export LIBZ    := 0
endif
endif

INCLUDES = -I$(SOURCE). -I$(SOURCE)../ -I$(SOURCE)../include 
DEPINCLUDES = -I. -I.. -I../include 

ifeq ($(LIBPNG),1)
INCLUDES += -I$(SOURCE)../libs/pnglib/libpng
DEPINCLUDES += -I../libs/pnglib/libpng
endif
ifeq ($(LIBZ),1)
INCLUDES += -I$(SOURCE)../libs/zlib/libz
DEPINCLUDES += -I../libs/zlib/libz
endif

DEFINES = -DBUILD_ICONLIB -DSYSTEM_PRIVATE

ifeq ($(LIBPNG),0)
DEFINES += -DUSE_SHARED_LIBPNG=1
endif

DEPOPTS = -noixemul -DDEPEND $(DEFINES) -DDEBUG

CFLAGS = -noixemul -c -O2 -D$(RELEASE) $(DEFINES) -fomit-frame-pointer -mcpu=$(CPU) \
	$(WARNINGS) $(INCLUDES)
LDFLAGS	= -nostdlib -noixemul -L$(SOURCE)../$(BUILDDIR)/lib -o

.PHONY: all
ifeq ($(wildcard $(DEPFILE)),$(DEPFILE))
all: build
else
all: depend
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -f $(MAKEFILENAME) build
endif

.PHONY: build
build: rev.h
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
	$(ECHO) -n "LD -> $(subst $(SOURCE),,$@) (final)"
	$(OUT)$(CC) $(LDFLAGS) $(EXE) $(OBJS) $(LIBS)
	$(ECHO) ""
	$(OUT)$(STRIP) --remove-section=.comment $(EXE)
else
$(EXE).db: $(OBJS)
	$(ECHO) -n "LD -> $(subst $(SOURCE),,$@)"
	$(OUT)$(CC) -Wl,--cref,-Map,$(EXE).map $(LDFLAGS) $(EXE).db $(OBJS) $(LIBS)
	$(ECHO) ""
	$(ECHO) -n "STRIP -> $(EXE)"
	$(OUT)$(STRIP) --remove-section=.comment -o $(SOURCE)$(EXE) $(EXE).db
	$(ECHO) ""
endif

.PHONY: depend
depend: ../config.h
	$(OUT)../rev
	$(ECHO)	-n "generating dependencies.."
	$(OUT)sh ../mkdep.sh -f $(DEPFILE) $(DEPOPTS) $(DEPINCLUDES) $(patsubst %.o,%.c, $(COBJS)) $(addprefix ../, $(patsubst %.o,%.c,$(COBJS_PARENT)))
	$(ECHO) " done"

.PHONY: clean
clean:
	$(ECHO) "==> [iconlib]"
	$(ECHO) -n "cleaning.."
	$(OUT)$(DELETEALL) $(BUILDDIR) $(EXE)
	$(ECHO) ""
	$(ECHO) "<== [iconlib]"

.PHONY: mrproper
mrproper: clean
	$(OUT)$(DELETE) rev.h $(DEPFILE) $(patsubst %.o,%.h, $(COBJS_NODEP)) $(COBJS_NODEP_H)

.PHONY: dump
dump:
	$(OBJDUMP) --section-headers --all-headers --reloc --disassemble-all --syms $(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).db >$(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).dump

.PHONY: install
install: all
ifeq ($(wildcard /sys/morphos/extmodules),/sys/morphos/extmodules)
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/extmodules"
	$(OUT)$(COPY) $(EXE) $(INSTALLDIR)morphos/extmodules/icon.elf
endif
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/Libs"
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/Libs
	$(OUT)-$(COPY) $(BUILDDIR)/$(CPU-TYPES)/$(EXE) $(INSTALLDIR)morphos/Libs/icon.library
ifeq ($(shell uname), MorphOS)
	@-/mossys/c/flushlib $(APPNAME) quiet
endif

.PHONY: update
update:
	$(OUT)../rev INCREV
	$(OUT)touch copyright.h

-include $(DEPFILE)
include makefile.rules

