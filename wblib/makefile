#
# Ambient (workbench.library) - © 2001-2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# All Rights Reserved
#
# $Id: makefile,v 1.9 2017/07/25 20:36:33 piru Exp $
#

APPNAME = workbench.library

ROOTPATH=../../../
include $(ROOTPATH)SDK.global

include ../makefile.conf
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

INCLUDES = -I$(SOURCE). -I$(SOURCE)../include
DEPINCLUDES = -I. -I../include

DEFINES = -DBUILD_WBLIB -DSYSTEM_PRIVATE

DEPOPTS = -noixemul -DDEPEND $(DEFINES) -DDEBUG

CFLAGS = -noixemul -c -O2 -D$(RELEASE) $(DEFINES) -fomit-frame-pointer -mcpu=$(CPU) \
       $(WARNINGS) $(INCLUDES)
LDFLAGS        = -nostdlib -noixemul -o

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
depend:
	$(OUT)../rev
	$(ECHO)	-n "generating dependencies.."
	$(OUT)sh ../mkdep.sh -f $(DEPFILE) $(DEPOPTS) $(DEPINCLUDES) $(patsubst %.o,%.c, $(COBJS))
	$(ECHO) " done"

.PHONY: clean
clean:
	$(ECHO) "==> [wblib]"
	$(ECHO) -n "cleaning.."
	$(OUT)$(DELETEALL) $(BUILDDIR) $(EXE)
	$(ECHO) ""
	$(ECHO) "<== [wblib]"

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
	$(OUT)$(COPY) $(EXE) $(INSTALLDIR)morphos/extmodules/workbench.elf
endif
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/Libs"
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/Libs
	$(OUT)-$(COPY) $(BUILDDIR)/$(CPU-TYPES)/$(EXE) $(INSTALLDIR)morphos/Libs/workbench.library
ifeq ($(shell uname), MorphOS)
	@-/mossys/c/flushlib $(APPNAME) quiet
endif

.PHONY: sdk
sdk:
	@$(AUTODOC) $(AUTODOCFLAGS) -o workbench.$(AUTODOCPOSTFIX) $(wildcard *.c)
	mkdir -p $(SDKPATH)$(SDKROOT)$(SDKDOCS)
	cp workbench.$(AUTODOCPOSTFIX) $(SDKPATH)$(SDKROOT)$(SDKDOCS)


.PHONY: update
update:
	$(OUT)../rev INCREV
	$(OUT)touch copyright.h

-include $(DEPFILE)
include makefile.rules
