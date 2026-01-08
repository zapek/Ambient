#
# Ambient (about.amod) - © 2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# All Rights Reserved
#
# $Id: makefile,v 1.8 2017/07/25 20:36:33 piru Exp $
#

APPNAME = about.amod

include ../../makefile.conf
include makefile.objs
include makefile.libs

SOURCE = ../../
VPATH := $(SOURCE)

EXE = $(APPNAME)

OBJS = $(COBJS) $(COBJS_NODEP)

ifndef CPU-TYPES
CPU-TYPES = $(CPU_DEFAULT)
endif

RELEASE ?= BETA_RELEASE

INCLUDES = -I$(SOURCE). -I$(SOURCE)../../ -I$(SOURCE)../../include -I.
DEPINCLUDES = -I. -I../../ -I../../include

DEPOPTS = -noixemul -DDEPEND -DDEBUG

CFLAGS = -noixemul -c -O2 -fomit-frame-pointer -mcpu=$(CPU) $(WARNINGS) $(INCLUDES) -Umorphos
LDFLAGS = -nostdlib -noixemul -o

.PHONY: all
ifeq ($(wildcard $(DEPFILE)),$(DEPFILE))
all: .depend.check build
else
all: depend
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -f $(MAKEFILENAME) build
endif

.PHONY: build
build: $(LOBJS) ppcinline/about.h proto/about.h
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

.depend.check: makefile.objs makefile.libs
	$(OUT)$(MAKE) $(NODIR) $(SILENT) -f $(MAKEFILENAME) depend

.PHONY: depend
depend:
	$(OUT)../../rev
	$(ECHO)	-n "generating dependencies.."
	$(OUT)touch $(patsubst %.o,%.h, $(COBJS_NODEP)) $(COBJS_NODEP_H)
	$(OUT)sh ../../mkdep.sh -f $(DEPFILE) $(DEPOPTS) $(DEPINCLUDES) $(patsubst %.o,%.c, $(COBJS))
	$(OUT)$(DELETE) $(patsubst %.o,%.h, $(COBJS_NODEP)) $(COBJS_NODEP_H)
	$(OUT)touch .depend.check
	$(ECHO) " done"

.PHONY: clean
clean:
	$(ECHO) "==> [about]"
	$(ECHO) -n "cleaning.."
	$(OUT)$(DELETEALL) $(BUILDDIR)
	$(ECHO) ""
	$(ECHO) "<== [about]"

.PHONY: mrproper
mrproper: clean
	$(OUT)$(DELETEALL) rev.h $(DEPFILE) .depend.check $(patsubst %.o,%.h, $(COBJS_NODEP)) $(COBJS_NODEP_H) proto ppcinline $(APPNAME)

.PHONY: dump
dump:
	$(OBJDUMP) --section-headers --all-headers --reloc --disassemble-all --syms $(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).db >$(BUILDDIR)/$(CPU_DEFAULT)/$(EXE).dump

install: all
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/Ambient/modules"
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/ambient/modules
	$(OUT)$(COPY) $(BUILDDIR)/$(CPU-TYPES)/$(APPNAME) $(INSTALLDIR)morphos/ambient/modules
ifeq ($(shell uname), MorphOS)
	@-/mossys/c/flushlib $(APPNAME) quiet
endif

update:
	$(OUT)../../rev INCREV
	$(OUT)touch copyright.h

-include $(DEPFILE)
include makefile.rules
