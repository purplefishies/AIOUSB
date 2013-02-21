#
# $RCSfile: Makefile,v $
# $Revision: 1.10 $
# $Date: 2009/11/30 23:47:26 $
# jEdit:tabSize=4:indentSize=4:collapseFolds=1:
#
# make file for AIOUSB library sample program
#

ifdef DEBUG
	DEBUGOPTS = -ggdb
	ifdef AIOUSBLIBDIR
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousbcppdbg.a
	else
		LIBAIOUSB := -laiousbdbg				# use default library search path
	endif
else
	DEBUGOPTS = -DNDEBUG
	ifdef AIOUSBLIBDIR
		LIBAIOUSB := $(AIOUSBLIBDIR)/libaiousbcpp.a
	else
		LIBAIOUSB := -laiousb					# use default library search path
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
	g++ $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) sample.cpp $(LIBAIOUSB) -l$(LIBUSB) -o sample

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
