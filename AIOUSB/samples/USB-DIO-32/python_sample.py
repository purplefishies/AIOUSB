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
from AIOUSB import *

MAX_DIO_BYTES = 4
#
# Simple class for keeping track of deviecs found
#
class Device:
    outputMask = 0xff
    readBuffer = [0]*MAX_DIO_BYTES
    writeBuffer = [0]*MAX_DIO_BYTES
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

while deviceMask != 0 and len(devices) < number_devices :
    if (deviceMask & 1 ) != 0:
        productId = new_ulp()  
        nameSize= new_ulp()
        name = ""
        numDIOBytes = new_ulp()
        numCounters = new_ulp()
        QueryDeviceInfo(index , productId, nameSize, name, numDIOBytes, numCounters)

        if ulp_value(productId) == USB_DIO_32:
            devices.append( Device(index=index, 
                                   productID=productId,
                                   nameSize=nameSize, 
                                   numDIOBytes=numDIOBytes, 
                                   numCounters=numCounters))

        index += 1
        deviceMask >>= 1

device = devices[0]

AIOUSB_SetCommTimeout( device.index, 1000 )
device.outputMask = 0xff;

for port in range(0,ulp_value(device.numDIOBytes)):
    device.writeBuffer[ port ] = 0x11 * (port + 1)
 
    
result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask, device.writeBuffer ) # AIOUSB_FALSE ='s bTristate
if result != AIOUSB_SUCCESS:
    sys.exit("Error '%s' configuring device at index %d\n" % ( AIOUSB_GetResultCodeAsString( result ), device.index )


if devicesFound > 1:
    device = devices[ 0 ]						# select second device
    AIOUSB_SetCommTimeout( device.index, 1000 )	# set timeout for all USB operations

    for port in range(0,len(device.writeBuffer)):
        device.writeBuffer[ port ] = 0x66 - port # write unique pattern to each port

    result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask, device.writeBuffer )# AIOUSB_FALSE ='s bTristate 

    if result == AIOUSB_SUCCESS:
        print "Device at index %d successfully configured\n" % ( device.index )
    else:
        print "Error '%s' configuring device at index %d\n" % (AIOUSB_GetResultCodeAsString( result ), device.index )
    }

    #
    # DIO read
    #
    # for( index = 0 index < devicesFound index++ ) {
    #         device = &deviceTable[ index ]
    for device in devices:
        result = DIO_ReadAll( device.index, device.readBuffer )

        if result != AIOUSB_SUCCESS:
            print("Error '%s' reading inputs from device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device.index )
            break

            print( "Read the following values from device at index %d:" % ( device.index ))
            correct = AIOUSB_TRUE
            for( port = 0 port < device.numDIOBytes port++ ) {
                if( device.readBuffer[ port ] != device.writeBuffer[ port ] )
                    correct = AIOUSB_FALSE
                print " %#x" % ( device.readBuffer[ port ] )
            }
            print( correct ? " (correct)\n" : " (INCORRECT)\n" )
    }
    
    #
    # DIO write (board LEDs should flash vigorously during this test)
    #
    print "Writing patterns to devices:"
    
    allCorrect = AIOUSB_TRUE
    for( pattern = 0x00 pattern <= 0xf0 pattern += 0x10 ) {
        
        print  " %#x" % pattern
        
        # for( index = 0 index < devicesFound index++ ) {
        #         device = &deviceTable[ index ]
        #         for( port = 0 port < device.numDIOBytes port++ )
        #             device.writeBuffer[ port ] = pattern + index * 0x04 + port

            result = DIO_WriteAll( device.index, device.writeBuffer )
                if  result != AIOUSB_SUCCESS:
                    print "Error '%s' writing outputs to device at index %d\n" % (AIOUSB_GetResultCodeAsString( result ), device.index )

                    goto abort
                }

                result = DIO_ReadAll( device.index, device.readBuffer ) # verify values written
                if ( result != AIOUSB_SUCCESS ) {
                    print  "Error '%s' reading inputs from device at index %d\n" % ( AIOUSB_GetResultCodeAsString( result ), device.index )
                    goto abort
                }

                if( result == AIOUSB_SUCCESS ) {
                    correct = AIOUSB_TRUE
                    for( port = 0 port < device.numDIOBytes port++ ) {
                        if( device.readBuffer[ port ] != device.writeBuffer[ port ] ) {
                            allCorrect = correct = AIOUSB_FALSE
                            break
                        }
                    }
                    if( ! correct ) {
                        print  "Error in data read back from device at index %d\n" % ( device.index )
                        goto abort
                    }
                    
                }
        }
        sleep( 1 )
    }
abort:
        print  allCorrect ? "\nAll patterns written were read back correctly\n" : "\n" )
  
exit_sample:
    AIOUSB_Exit()


    return ( int ) result 
} 

##
# Trash
#
# while( deviceMask != 0 && devicesFound < DEVICES_REQUIRED ) {
#         if( ( deviceMask & 1 ) != 0 ) {
#             device = &deviceTable[ devicesFound ];
#             device.nameSize = MAX_NAME_SIZE;
#             result = QueryDeviceInfo( index, &device.productID,
#                                       &device.nameSize, 
#                                       device.name, 
#                                       &device.numDIOBytes, 
#                                       &device.numCounters 
#                                       );
#             if( result == AIOUSB_SUCCESS ) {
#                 if( device.productID == USB_DIO_32 ) { // found a USB-DIO-32
#                     device.index = index;
#                     devicesFound++;
#                 }
#             } else
#               print( "Error '%s' querying device at index %d\n", 
#                       AIOUSB_GetResultCodeAsString( result ), 
#                       index 
#                       );
#         }
#         index++;
#         deviceMask >>= 1;
#     }
# if( devicesFound < DEVICES_REQUIRED ) {
#     print( "Error: You need at least %d devices connected to run this sample\n", DEVICES_REQUIRED );
#     goto abort;
# }
# unsigned port, pattern;
# AIOUSB_BOOL correct, allCorrect;
# for( index = 0; index < devicesFound; index++ ) {
#     device = &deviceTable[ index ];
#     result = GetDeviceSerialNumber( device.index, &device.serialNumber );
#     if( result == AIOUSB_SUCCESS )
#         print( "Serial number of device at index %d: %llx\n", device.index, ( long long ) device.serialNumber );
#     else
#         print( "Error '%s' getting serial number of device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device.index );
# }
