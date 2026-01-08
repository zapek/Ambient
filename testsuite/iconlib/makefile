#
# $Id: makefile,v 1.1 2016/07/18 08:20:49 itix Exp $
#

include ../../makefile.conf

test: iconlibtest
	$(OUT)iconlibtest

iconlibtest: iconlibtest.c
	$(OUT)$(HOSTCC) $(HOSTCFLAGS) -noixemul -o $@ $<
