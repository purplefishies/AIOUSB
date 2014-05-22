#!/usr/bin/python
# @file   sample.py
# @author $Format: %an <%ae>$
# @date   $Format: %ad$
# @release $Format: %t$
# @ingroup samples
# @brief Python version of the C/C++ samples that demonstrate reading / writing from the DIO-32 card
#
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
    productID = new_ulp()
    nameSize = new_ulp()
    numDIOBytes = new_ulp()
    numCounters = new_ulp()
    serialNumber = 0
    index = 0

    def __init__(self, **kwds):
        self.__dict__.update(kwds)


devices = []                    # Array of our Devices
number_devices = 1

print """USB-DIO-32 sample program version 1.17, 26 November 2009
This program demonstrates communicating with %d USB-DIO-32 devices on + 
AIOUSB library version %s, %s
the same USB bus. For simplicity, it uses the first %d such devices 
found on the bus""" % ( number_devices, AIOUSB_GetVersion(), AIOUSB_GetVersionDate() , number_devices )
         
result = AIOUSB_Init()
if result != AIOUSB_SUCCESS:
    sys.exit("Unable to initialize USB devices")

devicesFound = 0
deviceMask = GetDevices()
index = 0


AIOUSB_ListDevices()

while deviceMask > 0 and len(devices) < number_devices :
    if (deviceMask & 1 ) != 0:
        productId = new_ulp()  
        nameSize= new_ulp()
        name = ""
        numDIOBytes = new_ulp()
        numCounters = new_ulp()
        QueryDeviceInfo(index , productId, nameSize, name, numDIOBytes, numCounters)

        if ulp_value(productId) == USB_DIO_32:
            devices.append( Device(index=index,productID=productId, nameSize=nameSize,  numDIOBytes=numDIOBytes, numCounters=numCounters))
    index += 1
    deviceMask >>= 1


device = devices[0]

AIOUSB_SetCommTimeout( device.index, 1000 )
AIOChannelMaskSetMaskFromStr( device.outputMask, "1111" )

for port in range(0x20):
    print "Using value %d" % (port)
    DIOBufSetIndex( device.writeBuffer, port, 1 );
    print DIOBufToString( device.writeBuffer )
    result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )
    #print "Result was %d" % (result)
    time.sleep(1/10.0)

# ONLY Port D
AIOChannelMaskSetMaskFromStr(device.outputMask, "1000" );
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
time.sleep(1/10.0)
# ONLY Port C
AIOChannelMaskSetMaskFromStr(device.outputMask, "0100" );
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
time.sleep(1/10.0);
# ONLY Port B
AIOChannelMaskSetMaskFromStr(device.outputMask, "0010" );
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
time.sleep(1/10.0);
# ONLY Port A
AIOChannelMaskSetMaskFromStr(device.outputMask, "0001" );
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
time.sleep(1/10.0);
# All Channels
AIOChannelMaskSetMaskFromStr(device.outputMask, "1111" );
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );

tmpbuf = list("00000000000000000000000000000000")

for i in range(len(tmpbuf)):
    if i >= 1:
        tmpbuf[i] = "0"

    tmpbuf[i] = '1'
    DIOBufReplaceBinString(device.writeBuffer, "".join(tmpbuf) )
    result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )    
    time.sleep(1/10.0)


