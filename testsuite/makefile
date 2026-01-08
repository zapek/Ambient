#
# Ambient - © 2001-2003 by David Gerber <zapek@meanmachine.ch>
# Copyright 2006 Ambient Open Source Team
# All Rights Reserved
#
# $Id: makefile,v 1.4 2010/07/23 13:20:48 henes Exp $
#

SUBDIRS = $(sort $(filter-out $(shell if [ -f .ignore ]; then cat .ignore; fi;),$(filter-out CVSROOT,$(filter-out .,$(filter-out CVS,$(shell find . -maxdepth 1 -type d | sed 's/\.\///'))))))

all: .DEFAULT

.DEFAULT:
	@for i in $(SUBDIRS); \
		do echo ""; \
			echo -n "Entering "; \
			echo -n $$i; \
			echo ".."; \
			echo ""; \
			cd $$i; \
			if ! $(MAKE) $(MAKECMDGOALS); \
				then exit 1; \
			fi; \
			cd .. ; \
			done
