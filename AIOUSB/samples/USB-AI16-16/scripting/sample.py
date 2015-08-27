import sys
import time
import math
sys.path.append('build/lib.linux-x86_64-2.7')

import AIOUSB
from AIOUSB import *

class Device:
    name = ""
    serialNumber = 0
    index = 0
    numDIOBytes = 0
    numCounters = 0
    productID = 0
    def __init__(self, **kwds):
        self.__dict__.update(kwds)


MAX_NAME_SIZE = 20;

deviceIndex = 0;
deviceFound = AIOUSB_FALSE

CAL_CHANNEL = 5
MAX_CHANNELS = 128
NUM_CHANNELS = 16
NUM_OVERSAMPLES = 10
NUM_SCANS = 100000
BULK_BYTES = NUM_SCANS * NUM_CHANNELS * 2 * (NUM_OVERSAMPLES+1);
CLOCK_SPEED = 500000 / ( NUM_CHANNELS * (NUM_OVERSAMPLES+1) );

print """USB-AI16-16A sample program version 1.110, 17 November 2014
AIOUSB library version %s, %s
This program demonstrates controlling a USB-AI16-16A device on
the USB bus. For simplicity, it uses the first such device found
on the bus.
""" % ( AIOUSB_GetVersion(), AIOUSB_GetVersionDate() )

result = AIOUSB_Init()
if result != AIOUSB_SUCCESS:
    print "Error running AIOUSB_Init()..."
    sys.exit(1)

deviceMask = GetDevices()
if deviceMask == 0:
    print "No ACCES devices found on USB bus\n"
    sys.exit(1)

number_devices = 1
devices = []
AIOUSB_ListDevices()
index = 0

while deviceMask > 0 and len(devices) < number_devices :
    if (deviceMask & 1 ) != 0:
        obj = AIODeviceInfoGet( index )
        if obj.PID == USB_AIO16_16A or obj.PID == USB_DIO_16A :
            devices.append( Device( index=index, productID=obj.PID, numDIOBytes=obj.DIOBytes,numCounters=obj.Counters ))
    index += 1
    deviceMask >>= 1
try:
    device = devices[0]
except IndexError:
    print """No devices were found. Please make sure you have at least one 
ACCES I/O Products USB device plugged into your computer"""
    sys.exit(1)


deviceIndex = device.index

AIOUSB_Reset( deviceIndex );
print "Setting timeout"
AIOUSB_SetCommTimeout( deviceIndex, 1000 );

AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB_TRUE );

serialNumber = 0

[ndevice,result] = AIODeviceTableGetDeviceAtIndex( deviceIndex )

if result < AIOUSB_SUCCESS:
    print "Error: Can't get device\n"
    exit(1)


cb = AIOUSBDeviceGetADCConfigBlock( ndevice )

AIOUSB_SetAllGainCodeAndDiffMode( cb, AD_GAIN_CODE_10V, AIOUSB_FALSE );
ADCConfigBlockSetCalMode( cb, AD_CAL_MODE_NORMAL );
ADCConfigBlockSetTriggerMode( cb, 0 );
ADCConfigBlockSetScanRange( cb, 2, 13 );
ADCConfigBlockSetOversample( cb, 0 );


AIOUSBDeviceWriteADCConfig( ndevice, cb ); # Write the config block to the device

print "A/D settings successfully configured"

retval = 0
retval = ADC_SetCal(deviceIndex, ":AUTO:")

if retval != AIOUSB_SUCCESS:
    print "Error '%s' performing automatic A/D calibration" % ( AIOUSB_GetResultCodeAsString( retval ) )
    sys.exit(0)

ADCConfigBlockSetScanRange( cb, CAL_CHANNEL, CAL_CHANNEL )
ADCConfigBlockSetTriggerMode( cb , 0 )
ADCConfigBlockSetCalMode( cb , AD_CAL_MODE_GROUND )
AIOUSBDeviceWriteADCConfig( ndevice, cb )

# A better API is coming soon, so you won't have to do
# this to get Data
counts = new_ushortarray( 16 )
result = ADC_GetScan( deviceIndex, counts );

if retval < AIOUSB_SUCCESS:
    print "Error '%s' attempting to read ground counts\n" % ( AIOUSB_GetResultCodeAsString( result ) )
else:
    print "Ground counts = %u (should be approx. 0)" % ( ushort_getitem( counts, CAL_CHANNEL) )


ADC_ADMode( deviceIndex, 0 , AD_CAL_MODE_REFERENCE ) # TriggerMode
result = ADC_GetScan( deviceIndex, counts );
if result < AIOUSB_SUCCESS:
    print "Error '%s' attempting to read reference counts" % ( AIOUSB_GetResultCodeAsString( result ) )
else:
    print "Reference counts = %u (should be approx. 65130)" % ( ushort_getitem( counts, CAL_CHANNEL) )

gainCodes = [0 for x in range(0,16)]

# 
# demonstrate scanning channels and measuring voltages
# 
for channel in range(0,len(gainCodes)):
    gainCodes[channel] = AD_GAIN_CODE_0_10V


ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );


ADC_SetOversample( deviceIndex, NUM_OVERSAMPLES );
ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
ADC_ADMode( deviceIndex, 0 , AD_CAL_MODE_NORMAL );

volts = [0.0 for x in range(0,16)]
for i in range(0,1):
    result = ADC_GetScanV( deviceIndex, volts );
    for j in range(0,len(result)):
        print "  Channel %2d = %6.6f" % ( j, result[j] )


[result,voltage] = ADC_GetChannelV( deviceIndex, CAL_CHANNEL );
print "Result from A/D channel %d was %s " % (CAL_CHANNEL, voltage )





