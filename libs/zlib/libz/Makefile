# Makefile for zlib
# Copyright (C) 1995-2002 Jean-loup Gailly.
# For conditions of distribution and use, see copyright notice in zlib.h 

# To compile and test, type:
#   ./configure; make test
# The call of configure is optional if you don't have special requirements
# If you wish to build zlib as a shared library, use: ./configure -s

# To install /usr/local/lib/libz.* and /usr/local/include/zlib.h, type:
#    make install
# To install in $HOME instead of /usr/local, use:
#    make install prefix=$HOME

include ../../../makefile.conf

CFLAGS=-noixemul -O2 -mcpu=$(CPU) -fomit-frame-pointer

LDFLAGS=-L. -lz
LDSHARED=$(CC)
CPP=$(CC) -E

VER=1.1.4
LIBS=libz.a
SHAREDLIB=libz.so

OBJS = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
	   zutil.o inflate.o inftrees.o inffast.o

all: info libz.a infoend

libz.a: $(OBJS) $(OBJA)
	$(ECHO) -n "AR libz.a"
	$(OUT)$(AR) rc $@ $(OBJS) $(OBJA)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1
	$(ECHO) " done"

.PHONY: info
info:
	$(ECHO) "==> [libz.a]"

.PHONY: infoend
infoend:
	$(ECHO) "<== [libz.a]"

$(SHAREDLIB).$(VER): $(OBJS)
	$(LDSHARED) -o $@ $(OBJS)
	rm -f $(SHAREDLIB) $(SHAREDLIB).1
	ln -s $@ $(SHAREDLIB)
	ln -s $@ $(SHAREDLIB).1

.PHONY: clean
clean:
	$(OUT)$(DELETEALL) *.o *~ example minigzip libz.a libz.so* foo.gz so_locations \
	   _match.s maketree

distclean:	clean

depend:
	makedepend -- $(CFLAGS) -- *.[ch]

%.o : %.c
	$(ECHO)	"CC $(notdir $<)"
	$(OUT)$(CC) $(CFLAGS) -DBASE_NAME=\"$(notdir $<)\" -c $<

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
minigzip.o:  zlib.h zconf.h 
trees.o: deflate.h zutil.h zlib.h zconf.h trees.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h  
