#
# $Revision$
# $Date$
# $Author$
#
# make file for AIOUSB library sample programs
#
ifdef DEBUG
	DEBUGOPTS 	+= -ggdb
	DEBUGSTEM 	:= dbg
	DEBUG_ARGS	:= DEBUG=-DDEBUG
else
	DEBUGOPTS	+= -DNDEBUG
	DEBUGSTEM	:=
endif

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Regular C related Build
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
ifdef AIOUSBLIBDIR
	LIBAIOUSB 		:= $(AIOUSBLIBDIR)/libaiousb$(DEBUGSTEM).a
	LIBAIOUSBCPP 		:= $(AIOUSBLIBDIR)/libaiousbcpp$(DEBUGSTEM).a

	ALL_AIOUSBLIBDIR_CFILES	:= $(wildcard $(AIOUSBLIBDIR)/*.c )
	LIBAIOUSB_CDEP		:= $(LIBAIOUSB) $(wildcard $(AIOUSBLIBDIR)/*.c )
	CFLAGS			+= -I$(AIOUSBLIBDIR) 

#       Dynamic Build Rule for aiousblib
	AIOUSBLIB_BUILD		= $(LIBAIOUSB): $(ALL_AIOUSBLIBDIR_CFILES) ; cd $(AIOUSBLIBDIR) && $(MAKE) $(DEBUG_ARGS)
else
	LIBAIOUSB 		:= -laiousbdbg			# use default library search path
	LIBAIOUSBCPP 		:= -laiousbcppdbg		# "     "      "       "      "
endif

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# C++ Related Build
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
ifdef AIOUSBCLASSLIBDIR
	LIBCLASSAIOUSB 		:= $(AIOUSBCLASSLIBDIR)/libclassaiousbdbg.a
	ALL_AIOUSBLIBDIR_CFILES	:= $(wildcard $(AIOUSBLIBDIR)/*.c )
	ALL_CLASSLIB_CPPFILES	:= $(wildcard $(AIOUSBCLASSLIBDIR)/*.c)
	LIBAIOUSB_CPPDEP	:= $(LIBCLASSAIOUSB) $(ALL_AIOUSBLIBDIR_CFILES)  $(ALL_CLASSLIB_CPPFILES)

	CFLAGS			+=  -I$(AIOUSBCLASSLIBDIR)
#       Dynamic Build rule for C++ lib
	CLASSLIB_BUILD  	=  $(LIBCLASSAIOUSB): $(LIBAIOUSB) $(ALL_AIOUSBLIBDIR_CFILES)  $(ALL_CLASSLIB_CPPFILES) ; cd $(AIOUSBCLASSLIBDIR) && $(MAKE) $(DEBUG_ARGS)
else
	LIBCLASSAIOUSB 		:= -lclassaiousbdbg		# use default library search path
endif

ifndef AIOUSBJAVALIBDIR
	AIOUSBJAVALIBDIR := ../../lib/wrappers/java
endif

CPP			:= g++
CC			:= gcc
JAVAC			:= javac

include $(AIO_LIB_DIR)/wrappers/WrapperTargets.inc


COMPILE_MACROS		:= $(MACROS)
LIBUSB 			:= -lusb-1.0
COMMONOPTS 		:= -Wall -pthread -fPIC $(COMPILE_MACROS)

REVERSE_CAL_DEPS	:= reverse_cal_table.o lib/TestCaseSetup.o 
COMMON_DEPS             := lib/TestCaseSetup.o


LIBRARY_DEPS		:= $(LIBAIOUSB_CDEP) $(LIBAIOUSB_CPPDEP)

OBJFILE_OPTS		:= -c 
INCPATH			:= -Ilib $(CFLAGS)
LIBPATH			:= -Llib $(LDFLAGS)

CPPLIBS			+= $(LIBCLASSAIOUSB) $(LIBAIOUSBCPP)  $(LIBUSB) -lm $(LIBS)
CLIBS			+= $(LIBAIOUSB) $(LIBUSB) -lm $(LIBS)

TARGETS			:= $(wildcard *.cpp)
CTARGETS		:= $(wildcard *.c)
OBJECT_FILES		:= $(patsubst %.cpp,%.o,$(TARGETS)) $(patsubst %.c,%.c.o,$(CTARGETS))
JAVA_TARGETS		:= $(wildcard *.java)
JAVA_OBJECT_FILES	:= $(patsubst %.java,%.class,$(JAVA_TARGETS)) \
				  $(patsubst %.java,%.jar,$(JAVA_TARGETS))

FINAL_TARGETS		:= $(patsubst %.cpp,%,$(TARGETS))  $(patsubst %.c,%,$(CTARGETS))

ALL_CLEAN_TARGETS	:=	$(JAVA_OBJECT_FILES) \
			  	$(OBJECT_FILES) \
			  	$(COMMON_DEPS) \
				$(FINAL_TARGETS)

CPPFLAGS        := -D__aiousb_cplusplus

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
	@echo "BUILD_DEPS=$(LIBAIOUSB_CPPDEP)"

clean:
	$(RM) $(ALL_CLEAN_TARGETS)
	$(RM) *.csv
#
# This sets up the dependencies for our .a builds
#

$(eval $(call AIOUSBLIB_BUILD))
$(eval $(call CLASSLIB_BUILD))

$(AIO_LIB_DIR)/wrappers/java/$(JAVA_BUILT):
	cd $(AIO_LIB_DIR)/wrappers && $(MAKE) -f GNUMakefile inplace_java

#Sample.jar: $(AIO_LIB_DIR)/wrappers/java/$(JAVA_BUILT) Sample.class
#	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/AIOUSB.jar" > manifest
#	jar cmfe manifest Sample.jar Sample Sample.class
#	rm manifest

%.jar: %.class $(AIO_LIB_DIR)/wrappers/java/$(JAVA_BUILT)
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/AIOUSB.jar" > manifest
	jar cmfe manifest $@ Sample $<
	rm manifest

%:%.class
	LD_LIBRARY_PATH=$${AIO_LIB_DIR}:$${PWD}/../../lib/wrappers/java  java -classpath "../../lib/wrappers/java/AIOUSB.jar:$$PWD" $@

# Sample.class: Sample.java
%.class:%.java
	$(JAVAC) -cp $(AIOUSBJAVALIBDIR)/AIOUSB.jar $<


Extcal.jar: Extcal.class
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/AIOUSB.jar" > manifest
	jar cmfe manifest Extcal.jar Extcal Extcal.class
	rm manifest

Extcal.class: Extcal.java
	$(JAVAC) -cp $(AIOUSBJAVALIBDIR)/AIOUSB.jar Extcal.java

mostlyclean:
	-rm -f sample.o extcal.o Sample.class Extcal.class


lib/TestCaseSetup.o: lib/TestCaseSetup.cpp lib/TestCaseSetup.h  $(LIBRARY_DEPS)
	$(CPP) $(DEBUGOPTS) $(CPPFLAGS) $(LIBPATH) $(INCPATH) $(OBJFILE_OPTS) $(COMMONOPTS) $(CPPOPTS) $<  $(LIBAIOUSBCPP) $(LIBUSB) -o $@

#
# Smart Generic rules that can build any NEW C or C++ file that 
# you add to this directory
#

#
# C++ Rules
#
%.o:%.cpp $(COMMON_DEPS) $(LIBRARY_DEPS)
	$(CPP) $(DEBUGOPTS) $(CPPFLAGS) $(OBJFILE_OPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(LIBPATH) -o $@

%:%.o	$(COMMON_DEPS) $(LIBRARY_DEPS) 
	$(CPP)  $(LDFLAGS) $(DEBUGOPTS) $(INCPATH) $(COMMONOPTS) $(CPPOPTS) $< $(COMMON_DEPS) $(CPPLIBS)  -o $@


#
# C Rules
#
%.c.o:%.c	$(COMMON_DEPS)
	$(CC) -std=gnu99  -D_GNU_SOURCE $(OBJFILE_OPTS) $(INCPATH) $(DEBUGOPTS) $(LIBPATH) $(COMMONOPTS) $(COPTS) $<  -o $@

%:%.c.o
	$(CC) $(DEBUGOPTS) $(LIBPATH) $(COMMONOPTS) $< $(CLIBS) -o $@


