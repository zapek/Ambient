#
# Makefile for Open
# Copyright 2006 - 2008 Ambient Open Source Team
#
# $Id: makefile,v 1.2 2014/12/21 00:16:30 piru Exp $

include ../../makefile.conf

OBJS = main.o
CFLAGS = -noixemul -O2 -mcpu=$(CPU_DEFAULT) -fomit-frame-pointer $(WARNINGS)
LDFLAGS = -nostartfiles -noixemul
EXE = Open

.c.o:
	$(ECHO) "CC $(notdir $<)"
	$(OUT)$(CC) $(CFLAGS) -c $<

all: $(EXE)

$(EXE): $(OBJS)
	$(ECHO) "==> [open]"
	$(ECHO) "LD -> $@"
	$(OUT)$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(ECHO) "STRIP -> $@"
	$(OUT)$(STRIP) --remove-section=.comment $@
	$(ECHO) "<== [open]"

mrproper: clean

clean:
	$(ECHO) "==> [open]"
	$(ECHO) "cleaning.."
	$(OUT)$(DELETE) $(OBJS) $(EXE)
	$(ECHO) "<== [open]"

install: all
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/C"
	$(OUT)chmod +x $(EXE)
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/C
	$(OUT)$(COPY) $(EXE) $(INSTALLDIR)morphos/C
