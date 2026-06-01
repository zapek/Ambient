WARNINGS = -W -Wno-unused -Wno-sign-compare -Wpointer-arith\
		   -Wmissing-declarations -Wcomments   \
          

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(WARNINGS) -g  -noixemul -I ../  -I ../wblib -I ../includes-private  -D__AMIGADATE__=\"$(shell date "+%d.%m.%y")\" -DSYSTEM_PRIVATE -D__COPYRIGHTYEAR__=\"$(shell date "+%Y")\"


CC = ppc-morphos-gcc-9
STRIP = ppc-morphos-strip

TARGET = release/PanelPrefs.pobj

PATH_OBJS = $(patsubst %.o,objects/%.o, $(OBJS))


DEPINCLUDES = -I. -I../  -I../..

  
DEPFILE = .depend.morphos

COBJS_NODEP_DIRS = MorphOS/include

OBJS = PanelPrefs.o locale.o SliderSizeClass.o mui_func.o PanelListTreeClass.o \
						DefaultWindowPrefsClass.o BaseButtonPrefsClass.o MUIClassesPrefs.o ClassList.o \
						panelitem.o name.o SliderSpeedClass.o debug.o cattmp.o
				


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


$(TARGET): release/.sentinel  $(PATH_OBJS)
	@$(CC)  $(CFLAGS) $(PATH_OBJS) -nostartfiles -nostdlib   $(DEBUGLIB) -labox -laboxstubs -lc -o  $(TARGET)
	$(STRIP) $(TARGET)


clean:
	rm -rf objects $(TARGET) $(PANEL_INCLUDES)
