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
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousbdbg.a
		LIBAIOUSBCPP := $(AIOUSBLIBDIR)/libaiousbcppdbg.a
	else
		LIBAIOUSB := -laiousbdbg				# use default library search path
		LIBAIOUSBCPP := -laiousbcppdbg			# use default library search path
	endif
	ifdef AIOUSBCLASSLIBDIR
		LIBCLASSAIOUSB := $(AIOUSBCLASSLIBDIR)/libclassaiousbdbg.a
	else
		LIBCLASSAIOUSB := -lclassaiousbdbg		# use default library search path
	endif
else
	DEBUGOPTS = -DNDEBUG
	ifdef AIOUSBLIBDIR
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousb.a
		LIBAIOUSBCPP := $(AIOUSBLIBDIR)/libaiousbcpp.a
	else
		LIBAIOUSB := -laiousb					# use default library search path
		LIBAIOUSBCPP := -laiousbcpp				# use default library search path
	endif
	ifdef AIOUSBCLASSLIBDIR
		LIBCLASSAIOUSB := $(AIOUSBCLASSLIBDIR)/libclassaiousb.a
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
LIBRARY_DEPS		:= $(AIOUSBCLASSLIBDIR)/libclassaiousb.a $(AIOUSBLIBDIR)/libaiousb.a

OBJFILE_OPTS		:= -c 
INCPATH			:= -Ilib
LIBPATH			:= -L/home/jdamon/Documents/Linux/USB/AIOUSB-Latest/USB/AIOUSB/lib/


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

bar	:
	@echo $(JAVA_OBJECT_FILES)
	@echo $(LIBRARY_DEPS)
	@echo $(ALL_CLEAN_TARGETS)

all: $(FINAL_TARGETS)

clean:
	$(RM) $(ALL_CLEAN_TARGETS)

sample2: sample2.c
	$(CC) -std=gnu99 -D_GNU_SOURCE $(DEBUGOPTS) $(COMMONOPTS) $(COPTS) sample2.c $(LIBAIOUSB) $(LIBUSB) -lm -o sample2

sample: sample.cpp
	$(CPP) $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) sample.cpp $(LIBAIOUSBCPP) $(LIBUSB) -o sample


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

%.o:%.cpp	$(COMMON_DEPS)
	$(CPP) $(DEBUGOPTS) $(OBJFILE_OPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(LIBAIOUSBCPP) $(LIBUSB) -o $@

%:%.c	$(COMMON_DEPS)
	$(CC) -std=gnu99 -D_GNU_SOURCE $(DEBUGOPTS) $(COMMONOPTS) $(COPTS) $< $(LIBAIOUSB) $(LIBUSB) -lm -o $@

%:%.o
	$(CPP) $(DEBUGOPTS) $(LIBPATH) $(COMMONOPTS) $(CPPOPTS) $< $(COMMON_DEPS) $(LIBAIOUSBCPP) $(LIBUSB) -o $@


reverse_cal_table: $(REVERSE_CAL_DEPS)
	$(CPP) $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS)  -Ilib  $(REVERSE_CAL_DEPS) $(LIBAIOUSBCPP) $(LIBUSB) -o $@

reverse_cal.o:reverse_cal.cpp $(REVERSE_CAL_DEPS)
	$(CPP) $(DEBUGOPTS) $(OBJFILE_OPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(LIBAIOUSBCPP) $(LIBUSB) -o $@	


