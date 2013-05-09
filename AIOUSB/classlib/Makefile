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
LIBS 		:= libclassaiousb.a libclassaiousbdbg.a


DOCDIR 		:= doc
DOCINDEX 	:= $(DOCDIR)/index.html				# use index.html as a proxy for all documentation files

COMMONOPTS 	:= -c -Wall -pthread -fPIC
CFLAGS		+= -I. -I../lib 

INCLUDES	:= $(CFLAGS) 

ifdef DEBUG
        DEBUGOPTS       += -ggdb $(DEBUG)
else
        DEBUGOPTS       += -DNDEBUG
endif




.PHONY : all
all : $(LIBS) 


completely_all: $(LIBS) $(DOCINDEX)

.PHONY : mostlyclean
mostlyclean:
	-rm -f $(CPPOBJS) $(CPPDBGOBJS)

.PHONY : clean
clean:
	-rm -f $(LIBS) $(CPPOBJS) $(CPPDBGOBJS) $(DOCDIR)/*

libclassaiousb.a : $(CPPOBJS)
	ar cr libclassaiousb.a $(CPPOBJS)

libclassaiousbdbg.a : $(CPPDBGOBJS)
	ar cr libclassaiousbdbg.a $(CPPDBGOBJS)

$(CPPOBJS) $(CPPDBGOBJS) : $(HPPSOURCES)

%.o : %.cpp
	g++ $(DEBUGOPTS) $(INCLUDES) $(COMMONOPTS) $(CPPOPTS) $< -o $@

%.dbg.o : %.cpp
	g++  $(DEBUGOPTS) $(INCLUDES) $(COMMONOPTS) $(CPPOPTS) $< -o $@

$(DOCINDEX) : Doxyfile $(HPPSOURCES) $(CPPSOURCES)
	doxygen

# end of file
