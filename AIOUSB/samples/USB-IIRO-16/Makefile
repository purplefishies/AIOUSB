#
# $RCSfile: Makefile,v $
# $Revision: 1.2 $
# $Date: 2009/11/30 23:46:44 $
# jEdit:tabSize=4:indentSize=4:collapseFolds=1:
#
# make file for AIOUSB library sample programs
#

ifdef DEBUG
	DEBUGOPTS = -ggdb
	ifdef AIOUSBLIBDIR
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousbcppdbg.a
	else
		LIBAIOUSB := -laiousbcppdbg				# use default library search path
	endif
	ifdef AIOUSBCLASSLIBDIR
		LIBCLASSAIOUSB := $(AIOUSBCLASSLIBDIR)/libclassaiousbdbg.a
	else
		LIBCLASSAIOUSB := -lclassaiousbdbg		# use default library search path
	endif
else
	DEBUGOPTS = -DNDEBUG
	ifdef AIOUSBLIBDIR
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousbcpp.a
	else
		LIBAIOUSB := -laiousbcpp				# use default library search path
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

LIBUSB := usb-1.0
COMMONOPTS := -Wall -pthread

.PNONY: all
all: sample Sample.jar

sample: sample.cpp
	g++ $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) sample.cpp $(LIBCLASSAIOUSB) $(LIBAIOUSB) -l$(LIBUSB) -o sample

Sample.jar: Sample.class
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/aiousb.jar" > manifest
	jar cmfe manifest Sample.jar Sample Sample.class
	rm manifest

Sample.class: Sample.java
	javac -cp $(AIOUSBJAVALIBDIR)/aiousb.jar Sample.java

mostlyclean:
	-rm -f sample.o Sample.class

clean:
	-rm -f sample sample.o Sample.jar Sample.class

# end of file
