#
# Makefile for loadwb
# Copyright 2002-2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
#
# $Id: makefile,v 1.7 2014/12/21 00:16:30 piru Exp $

include ../../makefile.conf

OBJS = main.o
CFLAGS = -noixemul -O2 -mcpu=$(CPU_DEFAULT) -fomit-frame-pointer $(WARNINGS)
LDFLAGS = -noixemul
EXE = LoadWB

.c.o:
	$(ECHO) "CC $(notdir $<)"
	$(OUT)$(CC) $(CFLAGS) -c $<

all: $(EXE)

$(EXE): $(OBJS)
	$(ECHO) "==> [loadwb]"
	$(ECHO) "LD -> $@"
	$(OUT)$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(ECHO) "STRIP -> $@"
	$(OUT)$(STRIP) --remove-section=.comment $@
	$(ECHO) "<== [loadwb]"

mrproper: clean

clean:
	$(ECHO) "==> [loadwb]"
	$(ECHO) "cleaning.."
	$(OUT)$(DELETE) $(OBJS) $(EXE)
	$(ECHO) "<== [loadwb]"

install: all
	$(ECHO) "COPY $(EXE) -> $(INSTALLDIR)morphos/C"
	$(OUT)chmod +x $(EXE)
	$(OUT)$(MKDIR) $(INSTALLDIR)morphos/C
	$(OUT)$(COPY) $(EXE) $(INSTALLDIR)morphos/C
