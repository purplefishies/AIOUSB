import sys
import time
import math
sys.path.append('build/lib.linux-x86_64-2.7')

import AIOUSB
from AIOUSB import *

class Device:
    # readBuffer = DIOBuf(MAX_DIO_BYTES )
    # writeBuffer = DIOBuf( MAX_DIO_BYTES )
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
deviceFound = AIOUSB.AIOUSB_FALSE

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
""" % ( AIOUSB.AIOUSB_GetVersion(), AIOUSB.AIOUSB_GetVersionDate() )

result = AIOUSB.AIOUSB_Init()
if result != AIOUSB.AIOUSB_SUCCESS:
    print "Error running AIOUSB_Init()..."
    sys.exit(1)

deviceMask = AIOUSB.GetDevices()
if deviceMask == 0:
    print "No ACCES devices found on USB bus\n"
    sys.exit(1)

number_devices = 1
devices = []
AIOUSB.AIOUSB_ListDevices()
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

serialNumber = AIOUSB.new_ulp()
result = GetDeviceSerialNumber( deviceIndex, serialNumber );
print "Serial number of device at index %d: %x" % ( deviceIndex, ulp_value( serialNumber ))

#
# demonstrate A/D configuration; there are two ways to configure the A/D;
# one way is to create an ADConfigBlock instance and configure it, and then
# send the whole thing to the device using ADC_SetConfig(); the other way
# is to use the discrete API functions such as ADC_SetScanLimits(), which
# send the new settings to the device immediately; here we demonstrate the
# ADConfigBlock technique; below we demonstrate use of the discrete functions

result = 0
ndevice,result = AIODeviceTableGetDeviceAtIndex( deviceIndex , result )
# print "Result was %d" % ( result )

cb = AIOUSBDeviceGetADCConfigBlock( ndevice )

# AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
AIOUSB_SetAllGainCodeAndDiffMode( cb, AD_GAIN_CODE_10V, AIOUSB_FALSE );
AIOUSB_SetCalMode( cb, AD_CAL_MODE_NORMAL );
AIOUSB_SetTriggerMode( cb, 0 );
AIOUSB_SetScanRange( cb, 2, 13 );
AIOUSB_SetOversample( cb, 0 );

ADC_WriteADConfigBlock( deviceIndex, cb )

#AIOUSBDeviceWriteConfig( device, cb )
#AIOUSB_WriteConfig( deviceIndex, cb )
#result = ADC_SetConfig( deviceIndex, cb.registers, cb.size );


print "A/D settings successfully configured"

#     /*
#      * demonstrate automatic A/D calibration
#      */
#    result = ADC_SetCal( deviceIndex, ":AUTO:" );
#    if( result == AIOUSB_SUCCESS )
#        printf( "Automatic calibration completed successfully\n" );
#    else
#        printf( "Error '%s' performing automatic A/D calibration\n", AIOUSB_GetResultCodeAsString( result ) );
retval = ADC_SetCal(deviceIndex, ":AUTO:")
if result != AIOUSB_SUCCESS:
    print "Error '%s' performing automatic A/D calibration" % ( AIOUSB_GetResultCodeAsString( result ) )
    sys.exit(0)



#     /*
#      * verify that A/D ground calibration is correct
#      */
ADC_SetOversample( deviceIndex, 0 );
ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL );
ADC_ADMode( deviceIndex, 0 , AD_CAL_MODE_GROUND );

counts = new_ushortarray( 16 )
result = ADC_GetScan( deviceIndex, counts );

if result != AIOUSB_SUCCESS:
    print "Error '%s' attempting to read ground counts\n" % ( AIOUSB_GetResultCodeAsString( result ) )
else:
    print "Ground counts = %u (should be approx. 0)" % ( ushort_getitem( counts, CAL_CHANNEL) )

# print "Counts=%s" % ( str(counts) )
# foocounts = [0 for x in range(1,16)]

# result = ADC_GetScan( deviceIndex, counts );
# if result != AIOUSB_SUCCESS:
#     print "Error '%s' attempting to read ground counts\n" % ( AIOUSB_GetResultCodeAsString( result ) )
# else:
#     print "Ground counts = %u (should be approx. 0)" % ( ushort_getitem( counts, CAL_CHANNEL) )


ADC_ADMode( deviceIndex, 0 , AD_CAL_MODE_REFERENCE ) # TriggerMode
result = ADC_GetScan( deviceIndex, counts );
if result != AIOUSB_SUCCESS:
    print "Error '%s' attempting to read reference counts" % ( AIOUSB_GetResultCodeAsString( result ) )
else:
    print "Reference counts = %u (should be approx. 65130)" % ( ushort_getitem( counts, CAL_CHANNEL) )

gainCodes = [0 for x in range(0,16)]

# 
# demonstrate scanning channels and measuring voltages
# 
for channel in range(0,len(gainCodes)):
    gainCodes[channel] = AD_GAIN_CODE_0_10V
    # gainCodes[channel] = AD_GAIN_CODE_10V

ADC_RangeAll( deviceIndex , gainCodes, AIOUSB_TRUE )
ADC_SetOversample( deviceIndex, NUM_OVERSAMPLES )
ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 )
ADC_ADMode( deviceIndex, 0 , AD_CAL_MODE_NORMAL )

print "Volts read:"

volts = [0.0 for x in range(0,16)]
for i in range(0,1):
    result = ADC_GetScanV( deviceIndex, volts );
    for j in range(0,len(result)):
        print "  Channel %2d = %6.6f" % ( j, result[j] )


# demonstrate reading a single channel in volts

result = ADC_GetChannelV( deviceIndex, CAL_CHANNEL, volts[ CAL_CHANNEL ] );

print "Result from A/D channel %d was %f " % (CAL_CHANNEL, result[0] )
result = ADC_GetChannelV( deviceIndex, 0 , volts[ CAL_CHANNEL ] );
print "Result from A/D channel %d was %f " % (  0 , result[0] )


# if result != AIOUSB_SUCCESS:
#     print "Error '%s' reading A/D channel %d" % (AIOUSB_GetResultCodeAsString( result ), CAL_CHANNEL )
#     sys.exit(0)
# else:
#     print("Volts read from A/D channel %d = %f" % ( CAL_CHANNEL, volts[ CAL_CHANNEL ] ))



# 
# demonstrate bulk acquire
# 

AIOUSB_Reset( deviceIndex );
ADC_SetOversample( deviceIndex, NUM_OVERSAMPLES );
ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );

AIOUSB_SetStreamingBlockSize( deviceIndex, 64*1024 );


print("Allocating %d Bytes" % ( BULK_BYTES ))
databuf = new_ushortarray( BULK_BYTES )



clockHz = 0;
CTR_StartOutputFreq( deviceIndex, 0, clockHz );
clockHz = 0;
CTR_StartOutputFreq( deviceIndex, 0, clockHz );


# 
#  configure A/D for timer-triggered acquisition
#
ADC_ADMode( deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );


#  /*
#   * start bulk acquire; ADC_BulkAcquire() will take care of starting
#   * and stopping the counter; but we do have to tell it what clock
#   * speed to use, which is why we call AIOUSB_SetMiscClock()
#   */
print("Using Clock speed %d to acquire data" % ( CLOCK_SPEED ))
AIOUSB_SetMiscClock( deviceIndex, CLOCK_SPEED );

result = ADC_BulkAcquire( deviceIndex, BULK_BYTES, databuf );

if result != AIOUSB_SUCCESS:
    print( "Error '%s' attempting to start bulk acquire of %d bytes\n" % (AIOUSB_GetResultCodeAsString( result ), BULK_BYTES ))
    sys.exit(1)
else:
    print( "Started bulk acquire of %d bytes" % ( BULK_BYTES ))

#     /*
#      * use bulk poll to monitor progress
#      */
#     if( result == AIOUSB_SUCCESS ) {
#         unsigned long bytesRemaining = BULK_BYTES;
#         for( int seconds = 0; seconds < 100; seconds++ ) {
#             sleep( 1 );
#             result = ADC_BulkPoll( deviceIndex, &bytesRemaining );
#             if( result == AIOUSB_SUCCESS ) {
#                 printf( "  %lu bytes remaining\n", bytesRemaining );
#                 if( bytesRemaining == 0 )
#                     break;
#             } else {
#                 printf( "Error '%s' polling bulk acquire progress\n", 
#                         AIOUSB_GetResultCodeAsString( result ) );
#                 sleep(1);
#                 break;
#             }
#         }

bytesRemaining = new_ulp()
ulp_assign( bytesRemaining, BULK_BYTES )

for i in range(0,60):
    time.sleep(1)
    result = ADC_BulkPoll( deviceIndex, bytesRemaining )
    if result != AIOUSB_SUCCESS:
        print("Error '%s' polling bulk acquire progress" % AIOUSB_GetResultCodeAsString( result ) )
    else:
        print( "  %lu bytes remaining" % ( ulp_value( bytesRemaining ) ))
        if ulp_value(bytesRemaining) == 0:
            break

sys.exit(0)


