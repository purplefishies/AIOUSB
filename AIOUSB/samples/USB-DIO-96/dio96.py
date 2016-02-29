#!/usr/bin/python
# @file   sample.py
# @author $Format: %an <%ae>$
# @date   $Format: %ad$
# @release $Format: %t$
# @ingroup samples
# @brief Python version of DIO96 sample
#
#/

import sys
import time
from AIOUSB import *

MAX_DIO_BYTES = 32

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

print """
USB-DIO-96 sample program version %s, %s 
This program demonstrates communicating using the USB-DIO-96 product.
AIOUSB library version %s, %s the same USB bus. It uses the 
first device found found on the bus
""" % ( "$Format: %t$", "$Format: %ad$", AIOUSB_GetVersion(), AIOUSB_GetVersionDate() )
         

result = AIOUSB_Init()
if result != AIOUSB_SUCCESS:
    sys.exit("Unable to initialize USB devices")
devicesFound = 0
deviceMask = GetDevices()
index = 0
AIOUSB_ListDevices()
while deviceMask > 0 and len(devices) < number_devices :
    if (deviceMask & 1 ) != 0:
        obj = AIODeviceInfoGet( index )
        if obj.PID == USB_DIO_96 :
            devices.append( Device( index=index, productID=obj.PID, numDIOBytes=obj.DIOBytes,numCounters=obj.Counters ))
    index += 1
    deviceMask >>= 1
try:
    device = devices[0]
except IndexError:
    print """No devices were found. Please make sure you have at least one 
ACCES I/O Products USB device plugged into your computer"""
    sys.exit(1)


# Ports 3 and 1 , all writable
mask = NewAIOChannelMaskFromStr("111000111000")

buf = DIOBuf(
"11111111" + 
"11111111" + 
"11111111" + 

"11011011" + # Group 1 Port A   00100100  corresponds to   11011011 Volts
"01010101" + #         Port B   10101010        "          01010101 Volts
"00001111" + #         Port C   11110000        "          00001111 Volts

"11111111" + 
"11111111" + 
"11111111" +

"11110000" + # Group 3 Port A   00001111        "          11110000 Volts
"11000011" + #         Port B   00111100        "          11000011 Volts
"00111100"   #         Port C   11000011        "          00111100 Volts
)

print "Buf is %s" % ( buf  )

DIO_ConfigureWithDIOBuf( 0 , AIOUSB_FALSE, mask, buf )

retval = DIO_ReadAllToDIOBuf(0, buf)
print "Retval was %d" % ( retval )

print "Read8: %s" % ( buf )

sys.exit(0)


for i in range(0,12):
    if i % 3 == 0:
        port = "A"
    elif i % 3 == 1:
        port = "B"
    else: 
        port = "C"

    print "Group %d Port %c:  Value: %s" % ( i / 3, port, "FOO" )

               # show_byte(DataRead[i])
sys.exit(0)


#tmpbuf = list("00000000000000000000000000000000")
# for i in range(len(tmpbuf)):
#     if i >= 1:
#         tmpbuf[i] = "0"
#     tmpbuf[i] = '1'
#     DIOBufReplaceBinString(device.writeBuffer, "".join(tmpbuf) )
#     result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )    
#     time.sleep(1/10.0)
# ONLY Port D
#AIOChannelMaskSetMaskFromStr(device.outputMask, "1000" );
#result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
#time.sleep(1/10.0)
# ONLY Port C
#AIOChannelMaskSetMaskFromStr(device.outputMask, "0100" );
#result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
#time.sleep(1/10.0);
# ONLY Port B
#AIOChannelMaskSetMaskFromStr(device.outputMask, "0010" );
#result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
#time.sleep(1/10.0);
# ONLY Port A
#AIOChannelMaskSetMaskFromStr(device.outputMask, "0001" );
#result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
#time.sleep(1/10.0);
# All Channels
#AIOChannelMaskSetMaskFromStr(device.outputMask, "1111" );
#result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );

# AIOUSB_SetCommTimeout( device.index, 1000 )
# AIOChannelMaskSetMaskFromStr( device.outputMask, "1111" )
# print "Mask was %s " % ( AIOChannelMaskToString( device.outputMask ))

# for port in range(0x20):
#     print "Using value %d" % (port)
#     DIOBufSetIndex( device.writeBuffer, port, 1 );
#     print DIOBufToString( device.writeBuffer )
#     result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )
#     #print "Result was %d" % (result)
#     time.sleep(1/10.0)
