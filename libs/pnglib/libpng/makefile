# makefile for libpng on Linux ELF with gcc
# Copyright (C) 1998, 1999 Greg Roelofs
# Copyright (C) 1996, 1997 Andreas Dilger
# For conditions of distribution and use, see copyright notice in png.h

include ../../../makefile.conf

# Where the zlib library and include files are located
ZLIBINC=../../zlib/libz

WARNMORE=-Wwrite-strings -Wpointer-arith -Wshadow \
	-Wmissing-declarations -Wtraditional -Wcast-align \
	-Wstrict-prototypes -Wmissing-prototypes #-Wconversion

CFLAGS=-noixemul -I$(ZLIBINC) -Wall -O3 -funroll-loops -fomit-frame-pointer -mcpu=$(CPU) $(FEATURES)

# $(WARNMORE) -g -DPNG_DEBUG=5
LDFLAGS=-L. -Wl,-rpath,. -Wl,-rpath, -lpng -lm

# read libpng.txt or png.h to see why PNGMAJ is 3.  You should not
# have to change it.
PNGMAJ = 3
PNGMIN = 1.2.1
PNGVER = $(PNGMAJ).$(PNGMIN)

OBJS = png.o pngset.o pngget.o pngrutil.o pngtrans.o pngwutil.o \
	pngread.o pngrio.o pngwio.o pngwrite.o pngrtran.o \
	pngwtran.o pngmem.o pngerror.o pngpread.o

.SUFFIXES:      .c .o .pic.o
build: info libpng.a infoend

.PHONY: all
all:
	@echo "Sigh. You suck. Don't run make from here but the parent directory."

.PHONY: info
info:
	$(ECHO) "==> [libpng.a]"

.PHONY: infoend
infoend:
	$(ECHO) "<== [libpng.a]"


libpng.a: $(OBJS)
	$(ECHO) -n "AR libpng.a"
	$(OUT)$(AR) rc $@ $(OBJS)
	$(OUT)$(RANLIB) $@
	$(ECHO) " done"

.PHONY: clean
clean:
	$(OUT)$(DELETEALL) *.o libpng.a libpng.so libpng.so.$(PNGMAJ)* pngtest pngout.png

%.o : %.c
	$(ECHO)	"CC $(notdir $<)"
	$(OUT)$(CC) $(CFLAGS) -DBASE_NAME=\"$(notdir $<)\" -c $<

# DO NOT DELETE THIS LINE -- make depend depends on it.

png.o png.pic.o: png.h pngconf.h
pngerror.o pngerror.pic.o: png.h pngconf.h
pngrio.o pngrio.pic.o: png.h pngconf.h
pngwio.o pngwio.pic.o: png.h pngconf.h
pngmem.o pngmem.pic.o: png.h pngconf.h
pngset.o pngset.pic.o: png.h pngconf.h
pngget.o pngget.pic.o: png.h pngconf.h
pngread.o pngread.pic.o: png.h pngconf.h
pngrtran.o pngrtran.pic.o: png.h pngconf.h
pngrutil.o pngrutil.pic.o: png.h pngconf.h
pngtrans.o pngtrans.pic.o: png.h pngconf.h
pngwrite.o pngwrite.pic.o: png.h pngconf.h
pngwtran.o pngwtran.pic.o: png.h pngconf.h
pngwutil.o pngwutil.pic.o: png.h pngconf.h
pngpread.o pngpread.pic.o: png.h pngconf.h

pngtest.o: png.h pngconf.h

