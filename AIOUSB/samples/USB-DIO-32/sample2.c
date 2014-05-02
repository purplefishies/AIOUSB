/**
 * @file   AIOUSB_Core.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @ingroup samples
 * @brief  
 *
 * @note All the API functions that DO NOT begin "AIOUSB_" are standard API functions, largely
 * documented in the <a href="USB Software Reference"> http://accesio.com/MANUALS/USB%20Software%20Reference.pdf</a>. 
 * The functions that DO begin with "AIOUSB_" are "extended" API functions added to the Linux
 * implementation. Source code lines in this sample program that are prefixed with the
 * comment highlight calls to the AIOUSB API.
 *
 * @see Compilation 
 * @see CmakeCompilation
 */

/**
 * @ref libusb 
 * LIBUSB (http://www.libusb.org/) must be installed on the Linux box (the AIOUSB code
 * was developed using libusb version 1.0.3). After installing libusb, it may also be
 * necessary to set an environment variable so that the libusb and aiousb header files can
 * be located:
 *
 *     export CPATH=/usr/local/include/libusb-1.0/:/usr/local/include/aiousb/
 *
 * Once libusb is installed properly, it should be possible to compile the sample program
 * using the simple command:
 *
 * @ref make_C_sample
 *
 *
 * @ref 
 *
 *     make
 *
 * Alternatively, one can "manually" compile the sample program using the command:
 *
 *     g++ sample.cpp -laiousb -lusb-1.0 -o sample
 *
 * or, to enable debug features
 *
 *     g++ -ggdb sample.cpp -laiousbdbg -lusb-1.0 -o sample
 */

#include <aiousb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


int main( int argc, char **argv ) {
    const int DEVICES_REQUIRED = 2;				// change this to 1 if only one device
    const int BITS_PER_BYTE = 8;
    const int MAX_DIO_BYTES = 4;				// a modest little assumption for convenience
    const int MASK_BYTES = ( MAX_DIO_BYTES + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
    const int MAX_NAME_SIZE = 20;
    int devicesFound = 0;
    int index = 0;
    typedef enum { 
      SUCCESS = 0,
      USB_ERROR = -1,
      NO_DEVICE_FOUND = -2
    } EXIT_CODE;
    EXIT_CODE exit_code = SUCCESS;

/*     static struct DeviceInfo { */
/*       unsigned char outputMask[ MASK_BYTES ]; */
/*       unsigned char readBuffer[ MAX_DIO_BYTES ];		// image of data read from board */
/*       unsigned char writeBuffer[ MAX_DIO_BYTES ];		// image of data written to board */
/*       char name[ MAX_NAME_SIZE + 2 ]; */
/*       unsigned long productID; */
/*       unsigned long nameSize; */
/*       unsigned long numDIOBytes; */
/*       unsigned long numCounters; */
/*       unsigned long serialNumber; */
/*       int index; */
/*     } deviceTable[ DEVICES_REQUIRED ]; */

/*     printf( */
/*            "USB-DIO-32 sample program version 1.17, 26 November 2009\n" */
/*            "  AIOUSB library version %s, %s\n" */
/*            "  This program demonstrates communicating with %d USB-DIO-32 devices on\n" */
/*            "  the same USB bus. For simplicity, it uses the first %d such devices\n" */
/*            "  found on the bus.\n", */
/*            AIOUSB_GetVersion(),  */
/*            AIOUSB_GetVersionDate(),  */
/*            DEVICES_REQUIRED,  */
/*            DEVICES_REQUIRED */
/*            ); */

/*     unsigned long result = AIOUSB_Init(); /\* Call AIOUSB_Init() before any meaningful AIOUSB functions; *\/ */
/*     if( result != AIOUSB_SUCCESS ) { */
/*         printf("Can't initialize AIOUSB USB device\n"); */
/*         exit_code = USB_ERROR; */
/*     } */

/*     unsigned long deviceMask = GetDevices(); /\**< @ref GetDevices *\/ */
/*     if( deviceMask == 0 ) { */
/*         printf( "No ACCES devices found on USB bus\n" ); */
/*         exit_code = USB_ERROR; */
/*         goto exit_sample; */
/*     } */

/*     AIOUSB_ListDevices(); */

/*     /\* */
/*      * search for required number of USB-DIO-32 devices */
/*      *\/ */

/*     struct DeviceInfo *device; */
/*     while( deviceMask != 0 && devicesFound < DEVICES_REQUIRED ) { */
/*         if( ( deviceMask & 1 ) != 0 ) { */
/*             // found a device, but is it the correct type? */
/*             device = &deviceTable[ devicesFound ]; */
/*             device->nameSize = MAX_NAME_SIZE; */
/*             result = QueryDeviceInfo( index, &device->productID, */
/*                                       &device->nameSize,  */
/*                                       device->name,  */
/*                                       &device->numDIOBytes,  */
/*                                       &device->numCounters  */
/*                                       ); */
/*             if( result == AIOUSB_SUCCESS ) { */
/*                 if( device->productID == USB_DIO_32 ) { // found a USB-DIO-32 */
/*                     device->index = index; */
/*                     devicesFound++; */
/*                 } */
/*             } else */
/*               printf( "Error '%s' querying device at index %d\n",  */
/*                       AIOUSB_GetResultCodeAsString( result ),  */
/*                       index  */
/*                       ); */
/*         } */
/*         index++; */
/*         deviceMask >>= 1; */
/*     } */

/*     if( devicesFound < DEVICES_REQUIRED ) { */
/*         printf( "Error: You need at least %d devices connected to run this sample\n", DEVICES_REQUIRED ); */
/*         goto abort; */
/*     } */
/*     unsigned port, pattern; */
/*     AIOUSB_BOOL correct, allCorrect; */

/*     for( index = 0; index < devicesFound; index++ ) { */
/*         device = &deviceTable[ index ]; */
/*         result = GetDeviceSerialNumber( device->index, &device->serialNumber ); */
/*         if( result == AIOUSB_SUCCESS ) */
/*             printf( "Serial number of device at index %d: %llx\n", device->index, ( long long ) device->serialNumber ); */
/*         else */
/*             printf( "Error '%s' getting serial number of device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index ); */
/*     } */

/*     /\* */
/*      * DIO configuration */
/*      *\/ */
/*     device = &deviceTable[ 0 ];							// select first device */
/*     AIOUSB_SetCommTimeout( device->index, 1000 );		// set timeout for all USB operations */
/*     /\* */
/*      * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f" */
/*      * here since there are only 4 ports) */
/*      *\/ */
/*     memset( device->outputMask, 0xff, MASK_BYTES ); */
    
/*     for( port = 0; port < device->numDIOBytes; port++ ) */
/*         device->writeBuffer[ port ] = 0x11 * ( port + 1 );	// write unique pattern to each port */
    
/*     result = DIO_Configure( device->index, AIOUSB_FALSE, device->outputMask, device->writeBuffer ); /\**< AIOUSB_FALSE ='s bTristate *\/ */
    
/*     if( result == AIOUSB_SUCCESS ) */
/*         printf( "Device at index %d successfully configured\n", device->index ); */
/*     else */
/*         printf( "Error '%s' configuring device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index ); */
    
/*     if( devicesFound > 1 ) { */
/*             device = &deviceTable[ 1 ];						// select second device */
/*             AIOUSB_SetCommTimeout( device->index, 1000 );	// set timeout for all USB operations */
/*             /\* */
/*              * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f" */
/*              * here since there are only 4 ports) */
/*              *\/ */
/*             memset( device->outputMask, 0xff, MASK_BYTES ); */

/*             for( port = 0; port < device->numDIOBytes; port++ ) */
/*                 device->writeBuffer[ port ] = 0x66 - port;	// write unique pattern to each port */

/*             result = DIO_Configure( device->index, AIOUSB_FALSE /\* bTristate *\/, device->outputMask, device->writeBuffer ); */

/*             if( result == AIOUSB_SUCCESS ) */
/*                 printf( "Device at index %d successfully configured\n", device->index ); */
/*             else */
/*                 printf( "Error '%s' configuring device at index %d\n",  */
/*                         AIOUSB_GetResultCodeAsString( result ),  */
/*                         device->index  */
/*                         ); */
/*     } */

/*     /\* */
/*      * DIO read */
/*      *\/ */
/*     for( index = 0; index < devicesFound; index++ ) { */
/*             device = &deviceTable[ index ]; */
/*             result = DIO_ReadAll( device->index, device->readBuffer ); */

/*             if( result != AIOUSB_SUCCESS ) { */
/*                 printf( "Error '%s' reading inputs from device at index %d\n", AIOUSB_GetResultCodeAsString( result ), device->index ); */
/*                 break; */
/*             } */
/*             printf( "Read the following values from device at index %d:", device->index ); */
/*             correct = AIOUSB_TRUE; */
/*             for( port = 0; port < device->numDIOBytes; port++ ) { */
/*                 if( device->readBuffer[ port ] != device->writeBuffer[ port ] ) */
/*                     correct = AIOUSB_FALSE; */
/*                 printf( " %#x", device->readBuffer[ port ] ); */
/*             } */
/*             printf( correct ? " (correct)\n" : " (INCORRECT)\n" ); */
/*     } */
    
/*     /\* */
/*      * DIO write (board LEDs should flash vigorously during this test) */
/*      *\/ */
/*     printf( "Writing patterns to devices:" ); */
/*     fflush( stdout );				// must do for "real-time" feedback */
/*     allCorrect = AIOUSB_TRUE; */
/*     for( pattern = 0x00; pattern <= 0xf0; pattern += 0x10 ) { */
        
/*         printf( " %#x", pattern ); */
/*         fflush( stdout );			// must do for "real-time" feedback */
        
/*         for( index = 0; index < devicesFound; index++ ) { */
/*                 device = &deviceTable[ index ]; */
/*                 for( port = 0; port < device->numDIOBytes; port++ ) */
/*                     device->writeBuffer[ port ] = pattern + index * 0x04 + port; */

/*                 result = DIO_WriteAll( device->index, device->writeBuffer ); */
/*                 if ( result != AIOUSB_SUCCESS ) { */
/*                     printf( "Error '%s' writing outputs to device at index %d\n" , AIOUSB_GetResultCodeAsString( result ), device->index ); */

/*                     goto abort; */
/*                 } */

/*                 result = DIO_ReadAll( device->index, device->readBuffer ); // verify values written */
/*                 if ( result != AIOUSB_SUCCESS ) { */
/*                     printf( "Error '%s' reading inputs from device at index %d\n" , AIOUSB_GetResultCodeAsString( result ), device->index ); */
/*                     goto abort; */
/*                 } */

/*                 if( result == AIOUSB_SUCCESS ) { */
/*                     correct = AIOUSB_TRUE; */
/*                     for( port = 0; port < device->numDIOBytes; port++ ) { */
/*                         if( device->readBuffer[ port ] != device->writeBuffer[ port ] ) { */
/*                             allCorrect = correct = AIOUSB_FALSE; */
/*                             break; */
/*                         } */
/*                     } */
/*                     if( ! correct ) { */
/*                         printf( "Error in data read back from device at index %d\n", device->index ); */
/*                         goto abort; */
/*                     } */
                    
/*                 } */
/*         } */
/*         sleep( 1 ); */
/*     } */
/*  abort: */
/*         printf( allCorrect ? "\nAll patterns written were read back correctly\n" : "\n" ); */
  
/* exit_sample: */
/*     AIOUSB_Exit(); */


    return ( int ) 0;
} 

