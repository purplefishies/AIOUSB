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

print """Relay sample program %s
This program demonstrates communicating with %d USB-DIO-32 devices on + 
AIOUSB library version %s, %s
the same USB bus. For simplicity, it uses the first %d such devices 
found on the bus""" % (  "$Format: %ad$", number_devices, AIOUSB_GetVersion(), AIOUSB_GetVersionDate() , number_devices )

result = AIOUSB_Init()
if result != AIOUSB_SUCCESS:
    sys.exit("Unable to initialize USB devices")
deviceMask = GetDevices()
index = 0

while deviceMask > 0 and len(devices) < number_devices :
    if (deviceMask & 1 ) != 0:
        obj = GetDeviceInfo( index )
        if obj.PID == USB_IIRO_16 or obj.PID == USB_IIRO_8 or obj.PID == USB_IDIO_8 or obj.PID == USB_IDIO_16:
            devices.append( Device( index=index, productID=obj.PID, numDIOBytes=obj.DIOBytes,numCounters=obj.Counters ))
    index += 1
    deviceMask >>= 1
try:
    device = devices[0]
except IndexError:
    print """No devices were found. Please make sure you have at least one 
ACCES I/O Products USB device plugged into your computer"""
    sys.exit(1)

AIOUSB_SetCommTimeout( device.index, 1000 )
AIOChannelMaskSetMaskFromStr( device.outputMask, "1111" )


for port in range(0x20):
    print "Using value %d" % (port)
    DIOBufSetIndex( device.writeBuffer, port, 1 );
    print DIOBufToString( device.writeBuffer )
    result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )
    time.sleep(1/6.0)
