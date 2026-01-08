#
# Ambient (workbench.library) - © 2001-2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# All Rights Reserved
#
# $Id: makefile,v 1.7 2017/07/25 20:36:32 piru Exp $
#

APPNAME = z.alib

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
ifndef LIBZ
ifeq ($(wildcard ../../config.h),../../config.h)
export LIBZ := $(shell grep "\#define USE_SHARED_LIBZ" ../../config.h | grep -c 0)
else
export LIBZ := 1
endif
endif

INCLUDES = -I$(SOURCE). -I$(SOURCE)../../include
DEPINCLUDES = -I. -I../../include

DEPOPTS = -noixemul -DDEPEND -DDEBUG

CFLAGS = -noixemul -c -O2 -D$(RELEASE) -DSYSTEM_PRIVATE -fomit-frame-pointer -mcpu=$(CPU) \
	$(WARNINGS) $(INCLUDES)
LDFLAGS = -nostdlib -noixemul -o

.PHONY: all
ifeq ($(LIBZ),0)
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
build: ppcinline/z.h proto/z.h $(LOBJS)
#	$(OUT)$(MAKE) $(NODIR) $(SILENT) -C libz CPU=$(CPU_DEFAULT)
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
	$(ECHO) "==> [zlib]"
	$(ECHO) -n "cleaning.."
	$(OUT)$(DELETEALL) $(BUILDDIR) $(EXE)
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -C libz clean
	$(ECHO) ""
	$(ECHO) "<== [zlib]"

.PHONY: mrproper
mrproper: clean
	$(OUT)$(DELETEALL) $(DEPFILE) proto ppcinline $(EXE) rev.h

.PHONY: dump
dump:
	$(OBJDUMP) --section-headers --all-headers --reloc --disassemble-all --syms $(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).db >$(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).dump

.PHONY: install
ifeq ($(LIBZ),0)
install:
	@echo "z.alib not enabled, not installing"
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

