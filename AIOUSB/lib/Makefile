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
AIOUSB_ListDevices.o

AR		:= ar
CC		:= gcc
CPP		:= g++
CCINC		:= 
CC_SHFLAGS	:= -shared -Wl,-soname

CDBGOBJS := $(COBJS:.o=.dbg.o)
CPPOBJS := $(COBJS:.o=.cpp.o)
CPPDBGOBJS := $(CPPOBJS:.o=.dbg.o)

CLIB 		:= libaiousb.a
SHCLIB		:= libaiousb.so

CDBGLIB 	:= libaiousbdbg.a
SHCDBGLIB 	:= libaiousbdbg.so

CPPLIB 		:= libaiousbcpp.a
SHCPPLIB	:= libaiousbcpp.so

CPPDBGLIB 	:= libaiousbcppdbg.a
SHCPPDBGLIB 	:= libaiousbcppdbg.so

ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       := -DNDEBUG
endif


LIBS 		:= $(CLIB) $(CDBGLIB) $(CPPLIB) $(CPPDBGLIB)  $(SHCLIB) $(SHCDBGLIB) $(SHCPPLIB) $(SHCPPDBGLIB)
CFLAGS		+= -I.
SHARED_LIBS	:=  -lusb-1.0 -pthread
CPPFLAGS	:= -D__aiousb_cplusplus

.PHONY : all
all : $(LIBS)

all_cscope: $(LIBS) cscope

#*******************************  LIBRARIES  ********************************

$(CLIB) : $(COBJS)
	$(AR) cr $(CLIB) $(COBJS)
$(SHCLIB) : $(COBJS)
	$(CC) $(CC_SHFLAGS),$@ -o $@ $(COBJS) $(SHARED_LIBS)


$(CDBGLIB) : $(CDBGOBJS)
	$(AR) cr $(CDBGLIB) $(CDBGOBJS)
$(SHCDBGLIB): 	$(CDBGOBJS)
	$(CC) $(CC_SHFLAGS),$@ -o $@ $(COBJS) $(SHARED_LIBS) 


$(CPPLIB) : $(CPPOBJS)
	$(AR) cr $(CPPLIB) $(CPPOBJS)

$(SHCPPLIB) : $(CPPOBJS)
	$(CPP) $(CPPFLAGS) $(CC_SHFLAGS),$@ -o $@ $(CPPOBJS) $(SHARED_LIBS)


$(CPPDBGLIB) : $(CPPDBGOBJS)
	$(AR) cr $(CPPDBGLIB) $(CPPDBGOBJS)

$(SHCPPDBGLIB) : $(CPPDBGOBJS)
	$(CPP) $(CPPFLAGS) $(CC_SHFLAGS),$@ -o $@ $(CPPOBJS) $(SHARED_LIBS)


#********************************  OBJECTS  *********************************

$(COBJS) $(CDBGOBJS) $(CPPOBJS) $(CPPDBGOBJS) : aiousb.h AIOUSB_Core.h

OBJOPTS		:= -c 
COMMONOPTS 	:= -Wall -pthread -fPIC  $(CFLAGS)
EXTRA_COPTS	:= -std=gnu99 -D_GNU_SOURCE


CFLAGS		= $(DEBUGOPTS) $(COMMONOPTS)
COPTS		:= $(DEBUGOPTS) $(COMMONOPTS) $(EXTRA_COPTS)
CPPOPTS		:= $(DEBUGOPTS) $(COMMONOPTS)

$(COBJS) : %.o : %.c
	$(CC) $(OBJOPTS) $(COPTS) $< -o $@

$(CDBGOBJS) : %.dbg.o : %.c
	$(CC) $(OBJOPTS) $(COPTS) $< -o $@

$(CPPOBJS) : %.cpp.o : %.c
	$(CPP) $(CPPFLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

$(CPPDBGOBJS) : %.cpp.dbg.o : %.c
	$(CPP) $(CPPFLAGS) $(OBJOPTS) $(CPPOPTS) $< -o $@

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
