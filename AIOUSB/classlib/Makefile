#
# $Revision$
# $Date$
# $Author$
#
# make file for AIOUSB C++ class library and documentation
#

include $(AIOUSB_ROOT)/Mkfiles/oses.inc

CPPSOURCES 	:= $(wildcard *.cpp)
HPPSOURCES 	:= $(wildcard *.hpp)
CPPOBJS 	:= $(subst .cpp,.o,$(CPPSOURCES))
CPPDBGOBJS 	:= $(subst .o,.dbg.o,$(CPPOBJS))


DOCDIR 		:= doc
DOCINDEX 	:= $(DOCDIR)/index.html				# use index.html as a proxy for all documentation files

ifdef LDFLAGS
	LDFLAGS	+= -L$(AIO_LIB_DIR)
else
	LDFLAGS	= -L$(AIO_LIB_DIR) 
endif


COMMONOPTS 	:= -c -Wall -pthread
ifneq ("$(OSTYPE)","CYGWIN")
COMMONOPT	+= -fPIC 
endif

override CXXFLAGS += -fPIC -I/usr/include/libusb-1.0 -I. -I../lib -D__aiousb_cplusplus

ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       += -DNDEBUG
endif


ifeq ("$(OSTYPE)","Darwin") 
CXX_SHFLAGS := -dynamiclib -undefined dynamic_lookup
else 
CXX_SHFLAGS := -shared -Wl,-soname
endif

AIOUSBLIBS	:= -laiousbcpp -laiousb
SHARED_LIBS	:= $(AIOUSBLIBS)


STCPPLIB	:=$(LIBPREFIX)classaiousb.$(STATICSUFFIX)
STCPPDBGLIB	:=$(LIBPREFIX)classaiousbdbg.$(STATICSUFFIX)
SHCPPLIB	:=$(LIBPREFIX)classaiousb.$(LIBSUFFIX)
SHCPPDBGLIB	:=$(LIBPREFIX)classaiousbdbg.$(LIBSUFFIX)

LIBS 		:= $(STCPPLIB) $(STCPPDBGLIB) $(SHCPPLIB) $(SHCPPDBGLIB)


.PHONY : all

all : $(LIBS) 


completely_all: $(LIBS) $(DOCINDEX)

.PHONY : mostlyclean
mostlyclean:
	-rm -f $(CPPOBJS) $(CPPDBGOBJS)

.PHONY : clean
clean:
	-rm -f $(LIBS) $(CPPOBJS) $(CPPDBGOBJS) $(DOCDIR)/*

distclean: clean
	-rm -f $(LIBS)

$(LIBPREFIX)classaiousb.$(STATICSUFFIX) : $(CPPOBJS)
	$(AR) cr $@ $(CPPOBJS)

$(LIBPREFIX)classaiousbdbg.$(STATICSUFFIX) : $(CPPDBGOBJS)
	$(AR) cr $@ $(CPPDBGOBJS)




ifeq ("$(OSTYPE)","Darwin")


else ifeq ("$(OSTYPE)","CYGWIN")
$(SHCPPLIB) : $(CPPOBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CC_SHFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(CPPOBJS) -Wl,--no-whole-archive $(SHARED_LIBS)
$(SHCPPDBGLIB): $(CPPDBGOBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CC_SHFLAGS) -shared -o $@  -Wl,--out-implib=$(subst cyg,lib,$@).a -Wl,--export-all-symbols -Wl,--enable-auto-import  -Wl,--whole-archive $(CPPDBGOBJS) -Wl,--no-whole-archive $(SHARED_LIBS)
else				# Linux and UNIXes
$(SHCPPLIB) : $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $(CXX_SHFLAGS),$@ -o $@ $(CPPOBJS) $(SHARED_LIBS)

$(SHCPPDBGLIB) : $(CPPDBGOBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $(CXX_SHFLAGS),$@ -o $@ $(CPPDBGOBJS) $(SHARED_LIBS)
endif



$(CPPOBJS) $(CPPDBGOBJS) : $(HPPSOURCES)

%.o : %.cpp
	$(CXX) $(DEBUGOPTS) $(CXXFLAGS) $(CFLAGS) $(COMMONOPTS) $(CPPOPTS) $< -o $@

%.dbg.o : %.cpp
	$(CXX)  $(DEBUGOPTS) $(CXXFLAGS) $(CFLAGS) $(COMMONOPTS) $(CPPOPTS) $< -o $@

$(DOCINDEX) : Doxyfile $(HPPSOURCES) $(CPPSOURCES)
	doxygen

# end of file
