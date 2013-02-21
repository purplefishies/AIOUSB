#
# $RCSfile: Makefile,v $
# $Revision: 1.8 $
# $Date: 2010/01/29 18:35:43 $
# jEdit:tabSize=4:indentSize=4:collapseFolds=1:
#
# make file for AIOUSB library sample program
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

LIBUSB := -lusb-1.0
COMMONOPTS := -Wall -pthread -fPIC

.PNONY: all
all: sample receiver Sample.jar

sample: sample.cpp
	g++ $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) sample.cpp $(LIBAIOUSBCPP) $(LIBUSB) -o sample

receiver: receiver.cpp
	g++ $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) receiver.cpp $(LIBAIOUSBCPP) $(LIBUSB) -o receiver

Sample.jar: Sample.class
	@echo "Class-Path: $(AIOUSBJAVALIBDIR)/aiousb.jar" > manifest
	jar cmfe manifest Sample.jar Sample Sample*.class
	rm manifest

Sample.class: Sample.java
	javac -cp $(AIOUSBJAVALIBDIR)/aiousb.jar Sample.java

mostlyclean:
	-rm -f sample.o receiver.o Sample*.class

clean:
	-rm -f sample sample.o receiver receiver.o Sample.jar Sample*.class

# end of file
