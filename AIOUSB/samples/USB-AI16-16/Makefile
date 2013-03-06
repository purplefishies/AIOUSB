#
# $RCSfile: Makefile,v $
# $Revision: 1.18 $
# $Date: 2009/12/21 00:33:29 $
# jEdit:tabSize=4:indentSize=4:collapseFolds=1:
#
# make file for AIOUSB library sample programs
#

ifdef DEBUG
	DEBUGOPTS = -ggdb
	ifdef AIOUSBLIBDIR
		LIBAIOUSB 		:= $(AIOUSBLIBDIR)/libaiousbdbg.a
		LIBAIOUSBCPP 		:= $(AIOUSBLIBDIR)/libaiousbcppdbg.a
		AIOUSBLIB_BUILD		= $(LIBAIOUSB): ; cd $(AIOUSBLIBDIR) && $(MAKE)
		LIBAIOUSB_CDEP		:= $(LIBAIOUSB)
	else
		LIBAIOUSB := -laiousbdbg			# use default library search path
		LIBAIOUSBCPP := -laiousbcppdbg			# use default library search path
	endif
	ifdef AIOUSBCLASSLIBDIR
		LIBCLASSAIOUSB 		:= $(AIOUSBCLASSLIBDIR)/libclassaiousbdbg.a
		CLASSLIB_BUILD  	=  $(LIBCLASSAIOUSB): ; cd $(AIOUSBCLASSLIBDIR) && $(MAKE)
		LIBAIOUSB_CPPDEP	:= $(LIBCLASSAIOUSB)	
	else
		LIBCLASSAIOUSB := -lclassaiousbdbg		# use default library search path
	endif
else
	DEBUGOPTS = -DNDEBUG
	ifdef AIOUSBLIBDIR
		LIBAIOUSB 		:= $(AIOUSBLIBDIR)/libaiousb.a
		LIBAIOUSBCPP 		:= $(AIOUSBLIBDIR)/libaiousbcpp.a
		AIOUSBLIB_BUILD		= $(LIBAIOUSB): ; cd $(AIOUSBLIBDIR) && $(MAKE)
		LIBAIOUSB_CDEP		:= $(LIBAIOUSB)

	else
		LIBAIOUSB := -laiousb				# use default library search path
		LIBAIOUSBCPP := -laiousbcpp			# use default library search path
	endif
	ifdef AIOUSBCLASSLIBDIR
		LIBCLASSAIOUSB 		:= $(AIOUSBCLASSLIBDIR)/libclassaiousb.a
		CLASSLIB_BUILD  	= $(LIBCLASSAIOUSB): ; cd $(AIOUSBCLASSLIBDIR) && $(MAKE)
		LIBAIOUSB_CPPDEP	:= $(LIBCLASSAIOUSB)	
	else
		LIBCLASSAIOUSB := -lclassaiousb			# use default library search path
	endif
endif

ifndef AIOUSBJAVALIBDIR
	AIOUSBJAVALIBDIR := ../../java
endif

CPP			:= g++
CC			:= gcc
JAVAC			:= javac


COMPILE_MACROS		:= $(MACROS)
LIBUSB 			:= -lusb-1.0
COMMONOPTS 		:= -Wall -pthread -fPIC $(COMPILE_MACROS)

REVERSE_CAL_DEPS	:= reverse_cal_table.o lib/TestCaseSetup.o 
COMMON_DEPS             := lib/TestCaseSetup.o


LIBRARY_DEPS		:= $(LIBAIOUSB_CDEP) $(LIBAIOUSB_CPPDEP)

OBJFILE_OPTS		:= -c 
INCPATH			:= -Ilib $(CFLAGS)
LIBPATH			:= -Llib $(LDFLAGS)


TARGETS			:= $(wildcard *.cpp)
OBJECT_FILES		:= $(patsubst %.cpp,%.o,$(TARGETS))
JAVA_TARGETS		:= $(wildcard *.java)
JAVA_OBJECT_FILES	:= $(patsubst %.java,%.class,$(JAVA_TARGETS)) \
				  $(patsubst %.java,%.jar,$(JAVA_TARGETS))

FINAL_TARGETS		:= $(patsubst %.cpp,%,$(TARGETS))

ALL_CLEAN_TARGETS	:=	$(JAVA_OBJECT_FILES) \
			  	$(OBJECT_FILES) \
			  	$(COMMON_DEPS) \
				$(FINAL_TARGETS)


#*********************************  RULES  **********************************

.PNONY: all
.SUFFIXES:

all: $(FINAL_TARGETS)

#
# Debugging targets
#
test	:
	@echo "JAVA FILES=$(JAVA_OBJECT_FILES)"
	@echo "LIBRARY_DEPS=$(LIBRARY_DEPS)"
	@echo "CLEAN TARGETS=$(ALL_CLEAN_TARGETS)"
	@echo "CLASSLIB_DIR=$(AIOUSBCLASSLIBDIR)"
	@echo "AIOUSBLIB_DIR= $(AIOUSBLIBDIR)"
	@echo "AIOLIB=$(LIBCLASSAIOUSB)"
	@echo "LIBRARY_DEPS=$(LIBRARY_DEPS)"
	@echo '$(call CLASSLIB_BUILD)'
	@echo '$(call AIOUSBLIB_BUILD)'

# @echo "CPPDEPS=	$(LIBAIOUSB_CPPDEP)"
# @echo "CDEPS=$(LIBAIOUSB_CDEP)"



clean:
	$(RM) $(ALL_CLEAN_TARGETS)

#
# This sets up the dependencies for our .a builds
#

$(eval $(call AIOUSBLIB_BUILD))
$(eval $(call CLASSLIB_BUILD))



Sample.jar: Sample.class
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/aiousb.jar" > manifest
	jar cmfe manifest Sample.jar Sample Sample.class
	rm manifest

Sample.class: Sample.java
	$(JAVAC) -cp $(AIOUSBJAVALIBDIR)/aiousb.jar Sample.java

extcal: extcal.cpp
	$(CPP) $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) extcal.cpp $(LIBCLASSAIOUSB) $(LIBAIOUSBCPP) $(LIBUSB) -o extcal

Extcal.jar: Extcal.class
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/aiousb.jar" > manifest
	jar cmfe manifest Extcal.jar Extcal Extcal.class
	rm manifest

Extcal.class: Extcal.java
	$(JAVAC) -cp $(AIOUSBJAVALIBDIR)/aiousb.jar Extcal.java

mostlyclean:
	-rm -f sample.o extcal.o Sample.class Extcal.class


lib/TestCaseSetup.o: lib/TestCaseSetup.cpp lib/TestCaseSetup.h  $(LIBRARY_DEPS)
	$(CPP) $(DEBUGOPTS) $(LIBPATH) $(OBJFILE_OPTS) $(COMMONOPTS) $(CPPOPTS) $<  $(LIBAIOUSBCPP) $(LIBUSB) -o $@

prom_read_test: prom_read_test.cpp
	$(CPP) $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) $< $(LIBAIOUSBCPP) $(LIBUSB) -o $@

#
# Smart Generic rules that can build any NEW C or C++ file that 
# you add to this directory
#


%.o:%.cpp $(COMMON_DEPS) $(LIBRARY_DEPS)
	$(CPP) $(DEBUGOPTS) $(OBJFILE_OPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(LIBAIOUSBCPP) $(LIBUSB) -o $@

%:%.c	$(COMMON_DEPS)
	$(CC) -std=gnu99  -D_GNU_SOURCE $(DEBUGOPTS) $(LIBPATH) $(COMMONOPTS) $(COPTS) $< $(LIBAIOUSB) $(LIBUSB) -lm -o $@

%:%.cpp	$(COMMON_DEPS) $(LIBRARY_DEPS) 
	$(CPP) $(LDFLAGS) $(DEBUGOPTS) $(OBJFILE_OPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(LIBAIOUSBCPP) $(LIBUSB) -o $@


%:%.o
	$(CPP) $(DEBUGOPTS) $(LIBPATH) $(COMMONOPTS) $(CPPOPTS) $< $(COMMON_DEPS) $(LIBAIOUSBCPP) $(LIBUSB) -o $@


