#
# $Revision$
# $Date$
# $Author$
#
# make file for AIOUSB library
#

COBJS := \
AIOUSB_ADC.o \
AIOUSB_ADC_ExternalCal.o \
AIOUSB_Core.o \
AIOUSB_CTR.o \
AIOUSB_CustomEEPROMRead.o \
AIOUSB_CustomEEPROMWrite.o \
AIOUSB_DAC.o \
AIOUSB_DIO.o \
AIOUSB_WDG.o \
AIOContinuousBuffer.o \
AIOChannelMask.o \
AIOUSB_Properties.o \
AIODeviceInfo.o \
DIOBuf.o


CCINC		:= 
OSTYPE		= $(shell uname -s )
#OSTYPE		= $(shell fake_uname)
CYGWINTYPE	= $(patsubst CYGWIN%,CYGWIN,$(OSTYPE))
ifeq ("$(CYGWINTYPE)","CYGWIN")
OSTYPE		=CYGWIN
endif


ifeq ("$(OSTYPE)","Darwin") 
CC_SHFLAGS := -dynamiclib
else 
CC_SHFLAGS := -shared -Wl,-soname
endif


CDBGOBJS := $(COBJS:.o=.dbg.o)
CPPOBJS := $(COBJS:.o=.cpp.o)
CPPDBGOBJS := $(CPPOBJS:.cpp.o=.cpp.dbg.o)


ifeq ("$(OSTYPE)","Darwin")
LIBPREFIX=lib
LIBSUFFIX=dylib
STATICSUFFIX=a
else ifeq ("$(OSTYPE)","CYGWIN")
LIBPREFIX=cyg
LIBSUFFIX=dll
STATICSUFFIX=a
else
LIBPREFIX=lib
LIBSUFFIX=so
STATICSUFFIX=a
endif

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Library names
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SHCDBGLIB 	:= $(LIBPREFIX)aiousbdbg.$(LIBSUFFIX)
SHCPPLIB	:= $(LIBPREFIX)aiousbcpp.$(LIBSUFFIX)
SHCPPDBGLIB 	:= $(LIBPREFIX)aiousbcppdbg.$(LIBSUFFIX)
SHCLIB		:= $(LIBPREFIX)aiousb.$(LIBSUFFIX)

CLIB 		:= $(LIBPREFIX)aiousb.$(STATICSUFFIX)
CDBGLIB 	:= $(LIBPREFIX)aiousbdbg.$(STATICSUFFIX)
CPPLIB 		:= $(LIBPREFIX)aiousbcpp.$(STATICSUFFIX)
CPPDBGLIB 	:= $(LIBPREFIX)aiousbcppdbg.$(STATICSUFFIX)


ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       := -DNDEBUG
endif

DEBUG_FLAGS	:= -ggdb 


LIBS 		:= $(CLIB) $(CDBGLIB) $(CPPLIB) $(CPPDBGLIB)  $(SHCLIB) $(SHCDBGLIB) $(SHCPPLIB) $(SHCPPDBGLIB)
override CFLAGS	+= -I. -std=gnu99
TESTFLAGS	:= -g
SHARED_LIBS	:=  -lusb-1.0 -pthread -lm
override CXXFLAGS += -D__aiousb_cplusplus

.PHONY : all testit

all : $(LIBS)

testit:
	@echo $(CPPDBGOBJS)
	@echo $(OSTYPE)
	@echo $(CC_SHFLAGS)
	@echo $(CYGWINTYPE)

all_cscope: $(LIBS) cscope

#*******************************  LIBRARIES  ********************************

$(CLIB) : $(COBJS)
	$(AR) cr $(CLIB) $(COBJS)
$(CDBGLIB) : $(CDBGOBJS)
	$(AR) cr $(CDBGLIB) $(CDBGOBJS)
$(CPPLIB) : $(CPPOBJS)
	$(AR) cr $(CPPLIB) $(CPPOBJS)
$(CPPDBGLIB) : $(CPPDBGOBJS)
	$(AR) cr $(CPPDBGLIB) $(CPPDBGOBJS)

ifeq ("$(OSTYPE)","Darwin")
$(SHCLIB) : $(COBJS)
	$(CC)  $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(COBJS) $(SHARED_LIBS)
$(SHCDBGLIB): 	$(CDBGOBJS)
	$(CC) $(CC_SHFLAGS) $(LDFLAGS) -o $@ $(CDBGOBJS) $(SHARED_LIBS) 
$(SHCPPLIB) : $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)
$(SHCPPDBGLIB) : $(CPPDBGOBJS)
	$(CXX) $(CXXFLAGS)  $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)
else ifeq ("$(OSTYPE)","CYGWIN")

$(SHCLIB) : $(COBJS)
#echo "$(CC) $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(COBJS) $(SHARED_LIBS)"
	$(CC) $(LDFLAGS) $(CC_SHFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(COBJS) -Wl,--no-whole-archive $(SHARED_LIBS)


$(SHCDBGLIB): 	$(CDBGOBJS)
#	@echo "$(CC) $(CC_SHFLAGS) $(LDFLAGS) -o $@ $(CDBGOBJS) $(SHARED_LIBS)"
	$(CC) $(CC_SHFLAGS) $(LDFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(CDBGOBJS) -Wl,--no-whole-archive $(SHARED_LIBS)

$(SHCPPLIB) : $(CPPOBJS)
#@echo "$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(CPPOBJS) -Wl,--no-whole-archive $(SHARED_LIBS)

$(SHCPPDBGLIB) : $(CPPDBGOBJS)
#@echo "$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(CPPOBJS) -Wl,--no-whole-archive $(SHARED_LIBS)

else 
$(SHCLIB) : $(COBJS)
	$(CC)  $(LDFLAGS) $(CC_SHFLAGS),$@ -o $@ $(COBJS) $(SHARED_LIBS)
$(SHCDBGLIB): 	$(CDBGOBJS)
	$(CC) $(CC_SHFLAGS),$@ $(LDFLAGS) -o $@ $(CDBGOBJS) $(SHARED_LIBS) 
$(SHCPPLIB) : $(CPPOBJS)
	$(CXX) $(CXXFLAGS),$@ $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)
$(SHCPPDBGLIB) : $(CPPDBGOBJS)
	$(CXX) $(CXXFLAGS),$@  $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPDBJOBJS) $(SHARED_LIBS)
endif


#********************************  OBJECTS  *********************************

$(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) : aiousb.h AIOUSB_Core.h

OBJOPTS			:= -c 
COMMONOPTS		:= -Wall -pthread
ifneq ("$(OSTYPE)","CYGWIN")
COMMONOPTS		+= -fPIC
endif


EXTRA_COPTS		:= -std=gnu99 -D_GNU_SOURCE
SELF_TEST_FLAGS		:= -DSELF_TEST
SELF_TEST_LIBS		:= -lm -lpthread libaiousbdbg.a libaiousbcppdbg.a -lusb-1.0
SELF_TEST_CXX_FLAGS	:= -D__aiousb_cplusplus -DSELF_TEST 
SELF_TEST_CXX_LIBS	:= $(SELF_TEST_LIBS) -lgtest

PYCFLAGS		= $(DEBUGOPTS) $(COMMONOPTS)
COPTS			:= $(DEBUGOPTS) $(COMMONOPTS) $(EXTRA_COPTS)
CPPOPTS			:= $(DEBUGOPTS) $(COMMONOPTS)

$(COBJS) : %.o : %.c
	$(CC) $(OBJOPTS) $(CFLAGS) $(COPTS) $< -o $@

$(CDBGOBJS) : %.dbg.o : %.c
	$(CC) $(OBJOPTS) $(DEBUG_FLAGS) $(CFLAGS) $(COPTS) $< -o $@

$(CPPOBJS) : %.cpp.o : %.c
	$(CXX) $(CXXFLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

#$(CPPDBGOBJS) : 
%.cpp.dbg.o : %.c
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

pywrapper:
	CC=$(CC) CFLAGS="$(EXTRA_COPTS)  -lusb-1.0" python setup.py build_ext


#
# Self testing files
#
%_c_test: %.c $(SHCDBGLIB) $(SHCPPDBGLIB)
	$(CC) $(TESTFLAGS) $(CFLAGS) $(SELF_TEST_FLAGS) $< -o $@ -L. $(SELF_TEST_LIBS)

%_cpp_test: %.c $(SHCDBGLIB) $(SHCPPDBGLIB)
	$(CXX) $(TESTFLAGS) $(SELF_TEST_CXX_FLAGS) $(subst _cpp_test,.c,$<) -o $@ -L. $(SELF_TEST_CXX_LIBS)

mostlyclean:
	-rm -f $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) 

clean:
	-rm -f $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) cscope.out

distclean: 
	-rm -f $(LIBS) $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) cscope.out *_test
cscope:	
	cscope -b 
