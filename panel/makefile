WARNINGS = -W -Wno-unused -Wno-sign-compare -Wpointer-arith\
		   -Wmissing-declarations -Wcomments   \
          

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(WARNINGS) -DPANEL_APP -g  -noixemul -I ../  -I ../wblib -I ../includes-private  -D__AMIGADATE__=\"$(shell date "+%d.%m.%y")\" -DSYSTEM_PRIVATE -D__COPYRIGHTYEAR__=\"$(shell date "+%Y")\"


CC = ppc-morphos-gcc-9
STRIP = ppc-morphos-strip

TARGET = release/PanelApp

PATH_OBJS = $(patsubst %.o,objects/%.o, $(OBJS))
PATH_POBJS = $(patsubst %.o,objects/%.o, $(OBJS_PARENT))


DEPINCLUDES = -I. -I../  -I../..

  
DEPFILE = .depend.morphos

COBJS_NODEP_DIRS = MorphOS/include

OBJS = BaseButtonClass.o SeparatorClass.o  SpacerClass.o paneldragclass.o \
				CommandButtonClass.o SubPanelButtonClass.o panellib.o DefaultGroupClass.o \
				SubPanelWinClass.o  DefaultWindowClass.o MUIClassesApp.o\
				PanelApp.o ViewWatcherClass.o BookmarksClass.o\
				 am_name.o am_icon_read.o def_view_logo.o\
				am_gfx_bitmap.o	am_gfx_blit.o am_gfx_mask.o am_gfx_scale.o\
				locale.o
				
OBJS_PARENT =  methodstack.o gfx_dbuf.o gfx_pen.o iconmem.o mui_func.o 


DEBUGLIB  = -ldebug
RELEASE = 0

ECHO = echo
ECHE = echo -e
BOLD = \033[1m
NRML = \033[22m

COMPILING = @$(ECHE) "compiling $@$..."
LINKING = @$(ECHE) "linking $(BOLD)$@$(NRML)..."
STRIPPING = @$(ECHE) "stripping $(BOLD)$@$(NRML)..."
ARCHIVING = @$(ECHE) "archiving $(BOLD)$@$(NRML)..."



.PHONY: all 
ifeq ($(wildcard $(DEPFILE)),$(DEPFILE))
# nope
all:  $(TARGET) 
else
# yes
all: depend $(TARGET) 
 	
endif


.depend.check: $(OBJS)
	 @$(MAKE) --no-print-directory -s -f Makefile.MorphOS depend

.PHONY: depend
depend:
	@echo "generating dependencies (takes about a minute)"
	@sh ../mkdep.sh -f $(DEPFILE) $(DEPOPTS) $(DEPINCLUDES) -noixemul $(patsubst %.o,%.c, $(OBJS))
	@touch .depend.check

-include $(DEPFILE)


$(PATH_POBJS):
	$(COMPILING)
	@$(CC) $(CFLAGS) -DRELEASE=$(RELEASE) -DBASE_NAME=\"$(notdir $<)\" -DDATE="\"`date "+%d.%m.%y"`\"" -o $@ -c ../$(notdir $*).c

.PRECIOUS: %/.sentinel
%/.sentinel:
	mkdir -p ${@D}
	touch $@

objects/%.o: %.c objects/.sentinel
	$(COMPILING)
	@$(CC) $(CFLAGS) -DRELEASE=$(RELEASE) -DBASE_NAME=\"$(notdir $<)\" -DDATE="\"`date "+%d.%m.%y"`\"" -o $@ -c $*.c

PANEL_INCLUDES = ../includes-private/fd/panel_lib.fd ../includes-private/proto/panel.h ../includes-private/ppcinline/panel.h

$(TARGET): release/.sentinel $(PANEL_INCLUDES) $(PATH_OBJS) $(PATH_POBJS)
	@$(CC)  $(PATH_OBJS)  $(PATH_POBJS) -noixemul  $(DEBUGLIB)  -o  $(TARGET)
	$(STRIP) $(TARGET)

clean:
	rm -rf objects $(TARGET) $(PANEL_INCLUDES)


../includes-private/fd/panel_lib.fd: panellib.c
ifeq ($(shell uname), MorphOS)
	genfd.pl $< >$@
else
	../../development/tools/genfd/genfd.pl $< >$@
endif

../includes-private/ppcinline/panel.h: ../includes-private/fd/panel_lib.fd ../includes-private/clib/panel_protos.h
	cvinclude.pl --fd ../includes-private/fd/panel_lib.fd --clib ../includes-private/clib/panel_protos.h --inline $@

../includes-private/proto/panel.h:
	cvinclude.pl --fd ../includes-private/fd/panel_lib.fd --proto $@

.PHONY: install-iso
install-iso:
	@echo "disabled since this makes no sense"
	#$(COPY) -R release/* $(ISOPATH)MorphOS/Ambient

