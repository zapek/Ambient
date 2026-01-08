ROOTPATH=../../
include $(ROOTPATH)Makefile.global

all: png2c ambientprefs
	$(MAKE) -f makefile fullbuild RELEASE=FINAL_RELEASE MORPHOS_BETABUILD=$(MORPHOS_BETABUILD)
	$(MAKE) ambient-build-bug-fixup TARGETDIR=750-final

ambient-build-bug-fixup:
	# In Ambient, errors are considered features and 'features' are heralded. Fix up some of the mess.
	# OK, so Ambient builds different files depending on if you do 'all' or 'install-iso', sigh...

	# Because Ambient builds different target files depending on if you invoke 'all' or 'install-iso', sigh...
	@if [ ! -f objects/$(TARGETDIR)/Ambient ]; \
	then \
		if [ ! -f objects/$(TARGETDIR)/Ambient.db ]; \
		then \
			echo amb;\
			echo "No, really, Ambient build failed."; \
			exit 1; \
		fi \
	fi

	# See above :/
	@if [ ! -f iconlib/objects/$(TARGETDIR)/icon.library ]; \
	then \
		if [ ! -f iconlib/objects/$(TARGETDIR)/icon.library.db ]; \
		then \
			echo ico;\
			echo "No, really, Ambient build failed."; \
			exit 1; \
		fi \
	fi

	# See above :/
	@if [ ! -f wblib/objects/$(TARGETDIR)/workbench.library  ]; \
	then \
		if [ ! -f wblib/objects/$(TARGETDIR)/workbench.library.db ]; \
		then \
			echo wb; \
			echo "No, really, Ambient build failed."; \
			exit 1; \
		fi \
	fi

../../aboxtools/png2c/png2c:
	make -C ../../aboxtools/png2c

png2c: ../../aboxtools/png2c/png2c
	ln -s `cd ../../aboxtools/png2c/ && pwd`/png2c $@

../development/tools/ambientprefs/ambientprefs:
	make -C ../development/tools/ambientprefs

ambientprefs: ../development/tools/ambientprefs/ambientprefs
	ln -s `cd ../development/tools/ambientprefs/ && pwd`/ambientprefs $@

clean:
	$(MAKE) -f makefile distclean # Yes, this really cleans all.
	rm -f png2c ambientprefs

install: all
	$(MAKE) -f makefile install RELEASE=FINAL_RELEASE
	$(MAKE) ambient-build-bug-fixup TARGETDIR=750-final

install-iso: all
	$(MAKE) -f makefile install-iso
	$(MAKE) ambient-build-bug-fixup TARGETDIR=750-final

source:
	(cd .. && tar --exclude="distribution/icons" -cf $(SOURCEPATH)ambient.tar ambient)

.PHONY: ambient-build-bug-fixup source

