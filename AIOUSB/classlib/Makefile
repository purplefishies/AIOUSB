#
# $RCSfile: Makefile,v $
# $Revision: 1.13 $
# $Date: 2009/12/20 22:28:25 $
# jEdit:tabSize=4:indentSize=4:collapseFolds=1:
#
# make file for AIOUSB C++ class library and documentation
#

CPPSOURCES 	:= $(wildcard *.cpp)
HPPSOURCES 	:= $(wildcard *.hpp)
CPPOBJS 	:= $(subst .cpp,.o,$(CPPSOURCES))
CPPDBGOBJS 	:= $(subst .o,.dbg.o,$(CPPOBJS))
LIBS 		:= libclassaiousb.a libclassaiousbdbg.a
INCLUDES	:= -I.

DOCDIR := doc
DOCINDEX := $(DOCDIR)/index.html				# use index.html as a proxy for all documentation files

COMMONOPTS := -c -Wall -pthread -fPIC

.PHONY : all
all : $(LIBS) $(DOCINDEX)

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
	g++ $(INCLUDES) -DNDEBUG $(COMMONOPTS) $(CPPOPTS) $< -o $@

%.dbg.o : %.cpp
	g++ -ggdb $(INCLUDES) $(COMMONOPTS) $(CPPOPTS) $< -o $@

$(DOCINDEX) : Doxyfile $(HPPSOURCES) $(CPPSOURCES)
	doxygen

# end of file
