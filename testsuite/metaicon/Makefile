##############################################################################################
# This Makefile was automatically generated from a Flow Studio project
# Do not edit manually
##############################################################################################


CC = ppc-morphos-gcc-5
CINCLUDES = -I../..
CXX = ppc-morphos-g++-5
OBJCC = ppc-morphos-gcc-5
OBJCFLAGS = -fconstant-string-class=OBConstantString -std=gnu99
OBJCINCLUDES = -I/gg/Frameworks/include
CFLAGS += -MD -MP
CPPFLAGS += -MD -MP
OBJCFLAGS += -MD -MP

ifdef GCC_FLOWCOLORS
export GCC_COLORS = $(GCC_FLOWCOLORS)
export TERM = xterm
endif

LD=ppc-morphos-gcc-5
LINKLIBS = ../../objects/750-final/sxmlc.o ../../objects/750-final/sxmlsearch.o -ldebug

AR=ppc-morphos-ar
RANLIB=ppc-morphos-ranlib

CLIB_RELEASE = -noixemul
CC_DEBUG = ppc-morphos-gcc-5
CFLAGS_DEBUG = -g -O0
CXX_DEBUG = ppc-morphos-g++-5
CPPFLAGS_DEBUG = -fconstant-string-class=OBConstantString -std=gnu99 -g -O0
OBJCC_DEBUG = ppc-morphos-gcc-5
OBJCINCLUDES_DEBUG = -I/gg/Frameworks/include
CFLAGS_DEBUG += -MD -MP
CPPFLAGS_DEBUG += -MD -MP
OBJCFLAGS_DEBUG += -MD -MP

CLIB_DEBUG = -noixemul
LD_DEBUG=ppc-morphos-gcc-5
LDFLAGS_DEBUG = -g


SOURCES = metaicon.c

C_SOURCES := $(filter %.c,$(SOURCES))
CXX_SOURCES := $(filter %.cpp %.cxx %.cc,$(SOURCES))
OBJC_SOURCES := $(filter %.m,$(SOURCES))

.PHONY: all
all: RELEASE

OBJSDIR_RELEASE = 
EXECDIR_RELEASE = 
C_OBJS_RELEASE=$(C_SOURCES:%.c=$(OBJSDIR_RELEASE)%_RELEASE.o)
CXX_OBJS_RELEASE=$(patsubst %.cxx,$(OBJSDIR_RELEASE)%_RELEASE.o,$(patsubst %.cc,$(OBJSDIR_RELEASE)%_RELEASE.o,$(patsubst %.cpp,$(OBJSDIR_RELEASE)%_RELEASE.o,$(CXX_SOURCES))))
OBJC_OBJS_RELEASE=$(OBJC_SOURCES:%.m=$(OBJSDIR_RELEASE)%_RELEASE.mo)
ALL_OBJS_RELEASE=$(patsubst %.c,$(OBJSDIR_RELEASE)%_RELEASE.o,$(patsubst %.m,$(OBJSDIR_RELEASE)%_RELEASE.mo,$(patsubst %.cxx,$(OBJSDIR_RELEASE)%_RELEASE.o,$(patsubst %.cc,$(OBJSDIR_RELEASE)%_RELEASE.o,$(patsubst %.cpp,$(OBJSDIR_RELEASE)%_RELEASE.o,$(SOURCES))))))

.PHONY: MKDIR_RELEASE
MKDIR_RELEASE:

$(OBJSDIR_RELEASE)%_RELEASE.o : %.c ;  $(CC) $(CLIB_RELEASE) -c $(CFLAGS) $(CINCLUDES) $(CDEFINES) $< -o $@
$(OBJSDIR_RELEASE)%_RELEASE.o : %.cxx ;  $(CXX) $(CLIB_RELEASE) -c $(CFLAGS) $(CPPINCLUDES) $(CPPDEFINES) $< -o $@
$(OBJSDIR_RELEASE)%_RELEASE.o : %.cc ;  $(CXX) $(CLIB_RELEASE) -c $(CPPFLAGS) $(CPPINCLUDES) $(CPPDEFINES) $< -o $@
$(OBJSDIR_RELEASE)%_RELEASE.o : %.cpp ;  $(CXX) $(CLIB_RELEASE) -c $(CPPFLAGS) $(CPPINCLUDES) $(CPPDEFINES) $< -o $@
$(OBJSDIR_RELEASE)%_RELEASE.mo : %.m ;  $(OBJCC) $(CLIB_RELEASE) -c $(OBJCFLAGS) $(OBJCINCLUDES) $(OBJCDEFINES) $< -o $@

EXECNAME_RELEASE = metaicon
.PHONY: RELEASE
RELEASE: MKDIR_RELEASE  $(EXECNAME_RELEASE) 
	
$(EXECNAME_RELEASE): $(ALL_OBJS_RELEASE) 
	$(LD) $(CLIB_RELEASE) $(LDFLAGS) $(ALL_OBJS_RELEASE) $(LINKLIBS) -o $@

-include $(patsubst %.c,$(OBJSDIR_RELEASE)%_RELEASE.d,$(C_SOURCES))
-include $(patsubst %.cxx,$(OBJSDIR_RELEASE)%_RELEASE.d,$(patsubst %.cc,$(OBJSDIR_RELEASE)%_RELEASE.d,$(patsubst %.cpp,$(OBJSDIR_RELEASE)%_RELEASE.d,$(CXX_SOURCES))))
-include $(patsubst %.m,$(OBJSDIR_RELEASE)%_RELEASE.d,$(OBJC_SOURCES))

OBJSDIR_DEBUG = 
EXECDIR_DEBUG = 
C_OBJS_DEBUG=$(C_SOURCES:%.c=$(OBJSDIR_DEBUG)%_DEBUG.o)
CXX_OBJS_DEBUG=$(patsubst %.cxx,$(OBJSDIR_DEBUG)%_DEBUG.o,$(patsubst %.cc,$(OBJSDIR_DEBUG)%_DEBUG.o,$(patsubst %.cpp,$(OBJSDIR_DEBUG)%_DEBUG.o,$(CXX_SOURCES))))
OBJC_OBJS_DEBUG=$(OBJC_SOURCES:%.m=$(OBJSDIR_DEBUG)%_DEBUG.mo)
ALL_OBJS_DEBUG=$(patsubst %.c,$(OBJSDIR_DEBUG)%_DEBUG.o,$(patsubst %.m,$(OBJSDIR_DEBUG)%_DEBUG.mo,$(patsubst %.cxx,$(OBJSDIR_DEBUG)%_DEBUG.o,$(patsubst %.cc,$(OBJSDIR_DEBUG)%_DEBUG.o,$(patsubst %.cpp,$(OBJSDIR_DEBUG)%_DEBUG.o,$(SOURCES))))))

.PHONY: MKDIR_DEBUG
MKDIR_DEBUG:

$(OBJSDIR_DEBUG)%_DEBUG.o : %.c ;  $(CC_DEBUG) $(CLIB_DEBUG) -c $(CFLAGS_DEBUG) $(CINCLUDES_DEBUG) $(CDEFINES_DEBUG) $< -o $@
$(OBJSDIR_DEBUG)%_DEBUG.o : %.cxx ;  $(CXX_DEBUG) $(CLIB_DEBUG) -c $(CFLAGS_DEBUG) $(CPPINCLUDES_DEBUG) $(CPPDEFINES_DEBUG) $< -o $@
$(OBJSDIR_DEBUG)%_DEBUG.o : %.cc ;  $(CXX_DEBUG) $(CLIB_DEBUG) -c $(CPPFLAGS_DEBUG) $(CPPINCLUDES_DEBUG) $(CPPDEFINES_DEBUG) $< -o $@
$(OBJSDIR_DEBUG)%_DEBUG.o : %.cpp ;  $(CXX_DEBUG) $(CLIB_DEBUG) -c $(CPPFLAGS_DEBUG) $(CPPINCLUDES_DEBUG) $(CPPDEFINES_DEBUG) $< -o $@
$(OBJSDIR_DEBUG)%_DEBUG.mo : %.m ;  $(OBJCC_DEBUG) $(CLIB_DEBUG) -c $(OBJCFLAGS_DEBUG) $(OBJCINCLUDES_DEBUG) $(OBJCDEFINES_DEBUG) $< -o $@

EXECNAME_DEBUG = metaicon_DEBUG
.PHONY: DEBUG
DEBUG: MKDIR_DEBUG  $(EXECNAME_DEBUG) 
	
$(EXECNAME_DEBUG): $(ALL_OBJS_DEBUG) 
	$(LD_DEBUG) $(CLIB_DEBUG) $(LDFLAGS_DEBUG) $(ALL_OBJS_DEBUG) $(LINKLIBS_DEBUG) -o $@

-include $(patsubst %.c,$(OBJSDIR_DEBUG)%_DEBUG.d,$(C_SOURCES))
-include $(patsubst %.cxx,$(OBJSDIR_DEBUG)%_DEBUG.d,$(patsubst %.cc,$(OBJSDIR_DEBUG)%_DEBUG.d,$(patsubst %.cpp,$(OBJSDIR_DEBUG)%_DEBUG.d,$(CXX_SOURCES))))
-include $(patsubst %.m,$(OBJSDIR_DEBUG)%_DEBUG.d,$(OBJC_SOURCES))



.PHONY: clean
clean:
	rm -f $(EXECDIR_RELEASE)$(EXECNAME_RELEASE) $(OBJSDIR_RELEASE)*.o $(OBJSDIR_RELEASE)*.mo $(OBJSDIR_RELEASE)*.d $(EXECDIR_DEBUG)$(EXECNAME_DEBUG) $(OBJSDIR_DEBUG)*.o $(OBJSDIR_DEBUG)*.mo $(OBJSDIR_DEBUG)*.d

