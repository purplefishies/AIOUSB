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
AIOUSB_GetDeviceByProductID.o \
AIOUSB_GetDeviceBySerialNumber.o \
AIOUSB_GetDeviceProperties.o \
AIOUSB_GetResultCodeAsString.o \
AIOUSB_ListDevices.o \
AIOUSB_WDG.o \
AIOContinuousBuffer.o \
AIOChannelMask.o

CCINC		:= 
OSTYPE= $(shell uname -s)

ifeq ("$(OSTYPE)","Darwin") 
CC_SHFLAGS := -dynamiclib
else 
CC_SHFLAGS := -shared -Wl,-soname
endif


CDBGOBJS := $(COBJS:.o=.dbg.o)
CPPOBJS := $(COBJS:.o=.cpp.o)
CPPDBGOBJS := $(CPPOBJS:.o=.dbg.o)

CLIB 		:= libaiousb.a
CDBGLIB 	:= libaiousbdbg.a
CPPLIB 		:= libaiousbcpp.a
CPPDBGLIB 	:= libaiousbcppdbg.a

ifeq ($OSTYPE,"Darwin")
SHCDBGLIB 	:= libaiousbdbg.dylib
SHCPPLIB	:= libaiousbcpp.dylib
SHCPPDBGLIB 	:= libaiousbcppdbg.dylib
SHCLIB		:= libaiousb.dylib
else
SHCDBGLIB 	:= libaiousbdbg.so
SHCPPLIB	:= libaiousbcpp.so
SHCPPDBGLIB 	:= libaiousbcppdbg.so
SHCLIB		:= libaiousb.so
endif

ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       := -DNDEBUG
endif

DEBUG_FLAGS	:= -ggdb 


LIBS 		:= $(CLIB) $(CDBGLIB) $(CPPLIB) $(CPPDBGLIB)  $(SHCLIB) $(SHCDBGLIB) $(SHCPPLIB) $(SHCPPDBGLIB)
override CFLAGS	+= -I.
SHARED_LIBS	:=  -lusb-1.0 -pthread -lm
override CXXFLAGS += -D__aiousb_cplusplus

.PHONY : all testit

all : $(LIBS)

testit:
	echo $(OSTYPE)
	echo $(CC_SHFLAGS)

all_cscope: $(LIBS) cscope

#*******************************  LIBRARIES  ********************************

$(CLIB) : $(COBJS)
	$(AR) cr $(CLIB) $(COBJS)
$(SHCLIB) : $(COBJS)
	$(CC)  $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(COBJS) $(SHARED_LIBS)


$(CDBGLIB) : $(CDBGOBJS)
	$(AR) cr $(CDBGLIB) $(CDBGOBJS)
$(SHCDBGLIB): 	$(CDBGOBJS)
	$(CC) $(CC_SHFLAGS) $(LDFLAGS) -o $@ $(CDBGOBJS) $(SHARED_LIBS) 


$(CPPLIB) : $(CPPOBJS)
	$(AR) cr $(CPPLIB) $(CPPOBJS)

$(SHCPPLIB) : $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)


$(CPPDBGLIB) : $(CPPDBGOBJS)
	$(AR) cr $(CPPDBGLIB) $(CPPDBGOBJS)

$(SHCPPDBGLIB) : $(CPPDBGOBJS)
	$(CXX) $(CXXFLAGS)  $(LDFLAGS) $(CC_SHFLAGS) -o $@ $(CPPOBJS) $(SHARED_LIBS)


#********************************  OBJECTS  *********************************

$(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) : aiousb.h AIOUSB_Core.h

OBJOPTS		:= -c 
COMMONOPTS 	:= -Wall -pthread -fPIC  $(CFLAGS)
EXTRA_COPTS	:= -std=gnu99 -D_GNU_SOURCE


PYCFLAGS	= $(DEBUGOPTS) $(COMMONOPTS)
COPTS		:= $(DEBUGOPTS) $(COMMONOPTS) $(EXTRA_COPTS)
CPPOPTS		:= $(DEBUGOPTS) $(COMMONOPTS)

$(COBJS) : %.o : %.c
	$(CC) $(OBJOPTS) $(CFLAGS) $(COPTS) $< -o $@

$(CDBGOBJS) : %.dbg.o : %.c
	$(CC) $(OBJOPTS) $(DEBUG_FLAGS) $(CFLAGS) $(COPTS) $< -o $@

$(CPPOBJS) : %.cpp.o : %.c
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

$(CPPDBGOBJS) : %.cpp.dbg.o : %.c
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(CFLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

pywrapper:
	CC=$(CC) CFLAGS="$(EXTRA_COPTS)  -lusb-1.0" python setup.py build_ext

mostlyclean:
	-rm -f $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS)

clean:
	-rm -f $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) cscope.out

distclean: 
	-rm -f $(LIBS) $(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) cscope.out
cscope:	
	cscope -b 

# end of file
