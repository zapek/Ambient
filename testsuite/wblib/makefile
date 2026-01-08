#
# $Id: makefile,v 1.1 2006/08/08 13:43:44 fab Exp $
#

include ../../makefile.conf

test: wblibtest
	$(OUT)wblibtest

wblibtest: wblibtest.c
	$(OUT)$(HOSTCC) $(HOSTCFLAGS) -noixemul -o wblibtest wblibtest.c
