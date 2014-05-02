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

print """USB-DIO-32 sample program version 1.17, 26 November 2009
This program demonstrates communicating with %d USB-DIO-32 devices on + 
AIOUSB library version %s, %s
the same USB bus. For simplicity, it uses the first %d such devices 
found on the bus""" % ( number_devices, AIOUSB_GetVersion(), AIOUSB_GetVersionDate() , number_devices )
         
result = AIOUSB_Init()
if result != AIOUSB_SUCCESS:
    sys.exit("Unable to initialize USB devices")

deviceMask = GetDevices()

AIOUSB_ListDevices();

productId = new_ulp()  
ulp_assign(productId,0)
QueryDeviceInfo(0, productId,new_ulp(),"",new_ulp(), new_ulp())


while( deviceMask != 0 && devicesFound < DEVICES_REQUIRED ) {
        if( ( deviceMask & 1 ) != 0 ) {

            device = &deviceTable[ devicesFound ];
            device->nameSize = MAX_NAME_SIZE;
            result = QueryDeviceInfo( index, &device->productID,
                                      &device->nameSize, 
                                      device->name, 
                                      &device->numDIOBytes, 
                                      &device->numCounters 
                                      );
            if( result == AIOUSB_SUCCESS ) {
                if( device->productID == USB_DIO_32 ) { // found a USB-DIO-32
                    device->index = index;
                    devicesFound++;
                }
            } else
              printf( "Error '%s' querying device at index %d\n", 
                      AIOUSB_GetResultCodeAsString( result ), 
                      index 
                      );
        }
        index++;
        deviceMask >>= 1;
    }

    if( devicesFound < DEVICES_REQUIRED ) {
        printf( "Error: You need at least %d devices connected to run this sample\n", DEVICES_REQUIRED );
        goto abort;
    }
    unsigned port, pattern;
    AIOUSB_BOOL correct, allCorrect;

    for( index = 0; index < devicesFound; index++ ) {
        device = &deviceTable[ index ];
        result = GetDeviceSerialNumber( device->index, &device->serialNumber );
        if( result == AIOUSB_SUCCESS )
            printf( "Serial number of device at index %d: %llx\n", device->index, ( long long ) device->serialNumber );
        else
            printf( "Error '%s' getting serial number of device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index );
    }

    /*
     * DIO configuration
     */
    device = &deviceTable[ 0 ];							// select first device
    AIOUSB_SetCommTimeout( device->index, 1000 );		// set timeout for all USB operations
    /*
     * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f"
     * here since there are only 4 ports)
     */
    memset( device->outputMask, 0xff, MASK_BYTES );
    
    for( port = 0; port < device->numDIOBytes; port++ )
        device->writeBuffer[ port ] = 0x11 * ( port + 1 );	// write unique pattern to each port
    
    result = DIO_Configure( device->index, AIOUSB_FALSE, device->outputMask, device->writeBuffer ); /**< AIOUSB_FALSE ='s bTristate */
    
    # if( result == AIOUSB_SUCCESS )
    #     printf( "Device at index %d successfully configured\n", device->index );
    # else
    #     printf( "Error '%s' configuring device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index );
    
    if( devicesFound > 1 ) {
            device = &deviceTable[ 1 ];						// select second device
            AIOUSB_SetCommTimeout( device->index, 1000 );	// set timeout for all USB operations
            /*
             * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f"
             * here since there are only 4 ports)
             */
            memset( device->outputMask, 0xff, MASK_BYTES );

            for( port = 0; port < device->numDIOBytes; port++ )
                device->writeBuffer[ port ] = 0x66 - port;	// write unique pattern to each port

            result = DIO_Configure( device->index, AIOUSB_FALSE /* bTristate */, device->outputMask, device->writeBuffer );

            if( result == AIOUSB_SUCCESS )
                printf( "Device at index %d successfully configured\n", device->index );
            else
                printf( "Error '%s' configuring device at index %d\n", 
                        AIOUSB_GetResultCodeAsString( result ), 
                        device->index 
                        );
    }

    /*
     * DIO read
     */
    for( index = 0; index < devicesFound; index++ ) {
            device = &deviceTable[ index ];
            result = DIO_ReadAll( device->index, device->readBuffer );

            if( result != AIOUSB_SUCCESS ) {
                printf( "Error '%s' reading inputs from device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index );
                break;
            }
            printf( "Read the following values from device at index %d:", device->index );
            correct = AIOUSB_TRUE;
            for( port = 0; port < device->numDIOBytes; port++ ) {
                if( device->readBuffer[ port ] != device->writeBuffer[ port ] )
                    correct = AIOUSB_FALSE;
                printf( " %#x", device->readBuffer[ port ] );
            }
            printf( correct ? " (correct)\n" : " (INCORRECT)\n" );
    }
    
    /*
     * DIO write (board LEDs should flash vigorously during this test)
     */
    printf( "Writing patterns to devices:" );
    fflush( stdout );				// must do for "real-time" feedback
    allCorrect = AIOUSB_TRUE;
    for( pattern = 0x00; pattern <= 0xf0; pattern += 0x10 ) {
        
        printf( " %#x", pattern );
        fflush( stdout );			// must do for "real-time" feedback
        
        for( index = 0; index < devicesFound; index++ ) {
                device = &deviceTable[ index ];
                for( port = 0; port < device->numDIOBytes; port++ )
                    device->writeBuffer[ port ] = pattern + index * 0x04 + port;

                result = DIO_WriteAll( device->index, device->writeBuffer );
                if ( result != AIOUSB_SUCCESS ) {
                    printf( "Error '%s' writing outputs to device at index %d\n" , AIOUSB_GetResultCodeAsString( result ), device->index );

                    goto abort;
                }

                result = DIO_ReadAll( device->index, device->readBuffer ); // verify values written
                if ( result != AIOUSB_SUCCESS ) {
                    printf( "Error '%s' reading inputs from device at index %d\n" , AIOUSB_GetResultCodeAsString( result ), device->index );
                    goto abort;
                }

                if( result == AIOUSB_SUCCESS ) {
                    correct = AIOUSB_TRUE;
                    for( port = 0; port < device->numDIOBytes; port++ ) {
                        if( device->readBuffer[ port ] != device->writeBuffer[ port ] ) {
                            allCorrect = correct = AIOUSB_FALSE;
                            break;
                        }
                    }
                    if( ! correct ) {
                        printf( "Error in data read back from device at index %d\n", device->index );
                        goto abort;
                    }
                    
                }
        }
        sleep( 1 );
    }
abort:
        printf( allCorrect ? "\nAll patterns written were read back correctly\n" : "\n" );
  
exit_sample:
    AIOUSB_Exit();


    return ( int ) result;
} 

