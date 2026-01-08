#
# $Id: makefile,v 1.3 2006/08/08 13:31:43 fab Exp $
#

include ../../makefile.conf

test: wbtest wbparam/wbparam
	$(OUT)ln -s wbparam/wbparam tool-softlink1
	$(OUT)ln -s wbparam/wbparam softlink/tool-softlink2
	$(OUT)wbtest
	$(OUT)$(DELETE) softlink/tool-softlink2
	$(OUT)$(DELETE) tool-softlink1

wbtest: wbtest.c
	$(OUT)$(HOSTCC) $(HOSTCFLAGS) -o wbtest wbtest.c

wbparam/wbparam: wbparam/wbparam.c
	$(OUT)$(HOSTCC) $(HOSTCFLAGS) -o wbparam/wbparam wbparam/wbparam.c

