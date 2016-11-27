#!/usr/bin/python
# @file   sample.py
# @author $Format: %an <%ae>$
# @date   $Format: %ad$
# @release $Format: %t$
# @ingroup samples
# @brief Python version of the C/C++ samples that demonstrate reading / writing from the IIRO and IDIO cards
#/

import sys
import time
from AIOUSB import *

MAX_DIO_BYTES = 32

#
# Simple class for keeping track of deviecs found
#
class Device:
    outputMask = NewAIOChannelMaskFromStr("1111")
    readBuffer = DIOBuf(MAX_DIO_BYTES )
    writeBuffer = DIOBuf( MAX_DIO_BYTES )
    name = ""
    serialNumber = 0
    index = 0
    numDIOBytes = 0
    numCounters = 0
    productID = 0
    def __init__(self, **kwds):
        self.__dict__.update(kwds)


devices = []                    # Array of our Devices
number_devices = 1
outputMask = NewAIOChannelMaskFromStr("1111")
writeBuffer = DIOBuf( MAX_DIO_BYTES )

print """
USB-IIRO sample program version %s, %s 
This program demonstrates communicating using the USB-IIRO product.
AIOUSB library version %s, %s the same USB bus. It uses the 
first device found found on the bus
""" % ( "$Format: %t$", "$Format: %ad$", AIOUSB_GetVersion(), AIOUSB_GetVersionDate() )

AIOUSB_Init()

def find_iiro(obj):
    if obj.PID == USB_IDIO_16 or obj.PID == USB_IDIO_8 :
        return True

indices = AIOUSB_FindDevices( find_iiro )

if not indices:
    print """No USB-IIRO devices were found. Please make sure you have at least one 
ACCES I/O Products USB device plugged into your computer"""
    sys.exit(1)

device = indices[0]

AIOUSB_SetCommTimeout( device, 1000 )
AIOChannelMaskSetMaskFromStr( outputMask, "1111" )


for port in range(0x20):
    print "Using value %d" % (port)
    DIOBufSetIndex( writeBuffer, port, 1 );
    print str( writeBuffer )
    result = DIO_Configure( device, AIOUSB_FALSE, outputMask , writeBuffer )
    time.sleep(1/6.0)
