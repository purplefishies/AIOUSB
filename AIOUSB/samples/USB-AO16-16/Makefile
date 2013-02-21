#
# $RCSfile: Makefile,v $
# $Revision: 1.6 $
# $Date: 2009/11/25 22:50:24 $
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

LIBUSB := usb-1.0
COMMONOPTS := -Wall -pthread

sample: sample.cpp
	g++ $(DEBUGOPTS) $(COMMONOPTS) $(CPPOPTS) sample.cpp $(LIBAIOUSB) -l$(LIBUSB) -o sample

mostlyclean:
	-rm -f sample.o

clean:
	-rm -f sample sample.o

# end of file
