#
# $Revision$
# $Date$
# $Author$
#
# make file for AIOUSB C++ class library and documentation
#

CPPSOURCES 	:= $(wildcard *.cpp)
HPPSOURCES 	:= $(wildcard *.hpp)
CPPOBJS 	:= $(subst .cpp,.o,$(CPPSOURCES))
CPPDBGOBJS 	:= $(subst .o,.dbg.o,$(CPPOBJS))
LIBS 		:= libclassaiousb.a libclassaiousbdbg.a libclassaiousbdbg.so libclassaiousb.so


DOCDIR 		:= doc
DOCINDEX 	:= $(DOCDIR)/index.html				# use index.html as a proxy for all documentation files

COMMONOPTS 	:= -c -Wall -pthread -fPIC 

override CXXFLAGS += -I. -I../lib -D__aiousb_cplusplus

ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       += -DNDEBUG
endif

CXX_SHFLAGS      := -shared -Wl,-soname




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

libclassaiousb.a : $(CPPOBJS)
	$(AR) cr libclassaiousb.a $(CPPOBJS)

libclassaiousb.so : $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $(CXX_SHFLAGS),$@ -o $@ $(CPPOBJS) $(SHARED_LIBS)

libclassaiousbdbg.a : $(CPPDBGOBJS)
	$(AR) cr libclassaiousbdbg.a $(CPPDBGOBJS)

libclassaiousbdbg.so : $(CPPDBGOBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $(CXX_SHFLAGS),$@ -o $@ $(CPPDBGOBJS) $(SHARED_LIBS)




$(CPPOBJS) $(CPPDBGOBJS) : $(HPPSOURCES)

%.o : %.cpp
	$(CXX) $(DEBUGOPTS) $(CXXFLAGS) $(CFLAGS) $(COMMONOPTS) $(CPPOPTS) $< -o $@

%.dbg.o : %.cpp
	$(CXX)  $(DEBUGOPTS) $(CXXFLAGS) $(CFLAGS) $(COMMONOPTS) $(CPPOPTS) $< -o $@

$(DOCINDEX) : Doxyfile $(HPPSOURCES) $(CPPSOURCES)
	doxygen

# end of file
