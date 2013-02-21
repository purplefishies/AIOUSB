/*
 * $RCSfile: sample.cpp,v $
 * $Revision: 1.17 $
 * $Date: 2009/11/26 21:48:06 $
 * :tabSize=4:collapseFolds=1:
 *
 * AIOUSB library sample program
 */


// {{{ notes and build instructions
/*
 * This source code looks best with a tab width of 4.
 *
 * All the API functions that DO NOT begin "AIOUSB_" are standard API functions, largely
 * documented in http://accesio.com/MANUALS/USB%20Software%20Reference.pdf. The functions
 * that DO begin with "AIOUSB_" are "extended" API functions added to the Linux
 * implementation. Source code lines in this sample program that are prefixed with the
 * comment "/ * API * /" highlight calls to the AIOUSB API.
 *
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
// }}}

// {{{ includes
#include <aiousb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
using namespace AIOUSB;
// }}}

int main( int argc, char **argv ) {
	const int DEVICES_REQUIRED = 2;				// change this to 1 if only one device
	const int BITS_PER_BYTE = 8;
	const int MAX_DIO_BYTES = 4;				// a modest little assumption for convenience
	const int MASK_BYTES = ( MAX_DIO_BYTES + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
	const int MAX_NAME_SIZE = 20;

	static struct DeviceInfo {
		unsigned char outputMask[ MASK_BYTES ];
		unsigned char readBuffer[ MAX_DIO_BYTES ];		// image of data read from board
		unsigned char writeBuffer[ MAX_DIO_BYTES ];		// image of data written to board
		char name[ MAX_NAME_SIZE + 2 ];
		unsigned long productID;
		unsigned long nameSize;
		unsigned long numDIOBytes;
		unsigned long numCounters;
		__uint64_t serialNumber;
		int index;
	} deviceTable[ DEVICES_REQUIRED ];

	printf(
		"USB-DIO-32 sample program version 1.17, 26 November 2009\n"
		"  AIOUSB library version %s, %s\n"
		"  This program demonstrates communicating with %d USB-DIO-32 devices on\n"
		"  the same USB bus. For simplicity, it uses the first %d such devices\n"
		"  found on the bus.\n"
/*API*/	, AIOUSB_GetVersion(), AIOUSB_GetVersionDate()
		, DEVICES_REQUIRED, DEVICES_REQUIRED
	);

	/*
	 * MUST call AIOUSB_Init() before any meaningful AIOUSB functions;
	 * AIOUSB_GetVersion() above is an exception
	 */
/*API*/	unsigned long result = AIOUSB_Init();
	if( result == AIOUSB_SUCCESS ) {
		/*
		 * call GetDevices() to obtain "list" of devices found on the bus
		 */
/*API*/	unsigned long deviceMask = GetDevices();
		if( deviceMask != 0 ) {
			/*
			 * at least one ACCES device detected, but we want devices of a specific type
			 */
/*API*/		AIOUSB_ListDevices();				// print list of all devices found on the bus

			/*
			 * search for required number of USB-DIO-32 devices
			 */
			int devicesFound = 0;
			int index = 0;
			struct DeviceInfo *device;
			while(
				deviceMask != 0
				&& devicesFound < DEVICES_REQUIRED
			) {
				if( ( deviceMask & 1 ) != 0 ) {
					// found a device, but is it the correct type?
					device = &deviceTable[ devicesFound ];
					device->nameSize = MAX_NAME_SIZE;
/*API*/				result = QueryDeviceInfo( index, &device->productID
						, &device->nameSize, device->name, &device->numDIOBytes, &device->numCounters );
					if( result == AIOUSB_SUCCESS ) {
						if( device->productID == USB_DIO_32 ) {
							// found a USB-DIO-32
							device->index = index;
							devicesFound++;
						}	// if( device->productID ...
					} else
						printf( "Error '%s' querying device at index %d\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ), index );
				}	// if( ( deviceMask ...
				index++;
				deviceMask >>= 1;
			}	// while( deviceMask ...

			if( devicesFound >= DEVICES_REQUIRED ) {
				unsigned port, pattern;
				AIOUSB_BOOL correct, allCorrect;

				for( index = 0; index < devicesFound; index++ ) {
					device = &deviceTable[ index ];
/*API*/				result = GetDeviceSerialNumber( device->index, &device->serialNumber );
					if( result == AIOUSB_SUCCESS )
						printf( "Serial number of device at index %d: %llx\n", device->index, ( long long ) device->serialNumber );
					else
						printf( "Error '%s' getting serial number of device at index %d\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ), device->index );
				}	// for( index ...

				/*
				 * demonstrate DIO configuration
				 */
				device = &deviceTable[ 0 ];							// select first device
/*API*/			AIOUSB_SetCommTimeout( device->index, 1000 );		// set timeout for all USB operations
				/*
				 * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f"
				 * here since there are only 4 ports)
				 */
				memset( device->outputMask, 0xff, MASK_BYTES );
				for( port = 0; port < device->numDIOBytes; port++ )
					device->writeBuffer[ port ] = 0x11 * ( port + 1 );	// write unique pattern to each port
/*API*/			result = DIO_Configure( device->index, AIOUSB_FALSE /* bTristate */, device->outputMask, device->writeBuffer );
				if( result == AIOUSB_SUCCESS )
					printf( "Device at index %d successfully configured\n", device->index );
				else
					printf( "Error '%s' configuring device at index %d\n"
/*API*/					, AIOUSB_GetResultCodeAsString( result ), device->index );

				if( devicesFound > 1 ) {
					device = &deviceTable[ 1 ];						// select second device
/*API*/				AIOUSB_SetCommTimeout( device->index, 1000 );	// set timeout for all USB operations
					/*
					 * set all ports to output mode (we could just write "device->outputMask[ 0 ] = 0x0f"
					 * here since there are only 4 ports)
					 */
					memset( device->outputMask, 0xff, MASK_BYTES );
					for( port = 0; port < device->numDIOBytes; port++ )
						device->writeBuffer[ port ] = 0x66 - port;	// write unique pattern to each port
/*API*/				result = DIO_Configure( device->index, AIOUSB_FALSE /* bTristate */, device->outputMask, device->writeBuffer );
					if( result == AIOUSB_SUCCESS )
						printf( "Device at index %d successfully configured\n", device->index );
					else
						printf( "Error '%s' configuring device at index %d\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ), device->index );
				}	// if( devicesFound ...

				/*
				 * demonstrate DIO read
				 */
				for( index = 0; index < devicesFound; index++ ) {
					device = &deviceTable[ index ];
/*API*/				result = DIO_ReadAll( device->index, device->readBuffer );
					if( result == AIOUSB_SUCCESS ) {
						printf( "Read the following values from device at index %d:", device->index );
						correct = AIOUSB_TRUE;
						for( port = 0; port < device->numDIOBytes; port++ ) {
							if( device->readBuffer[ port ] != device->writeBuffer[ port ] )
								correct = AIOUSB_FALSE;
							printf( " %#x", device->readBuffer[ port ] );
						}	// for( port ...
						printf(
							correct
								? " (correct)\n"
								: " (INCORRECT)\n"
						);
					} else
						printf( "Error '%s' reading inputs from device at index %d\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ), device->index );
				}	// for( index ...

				/*
				 * demonstrate DIO write (board LEDs should flash vigorously during this test)
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
/*API*/					result = DIO_WriteAll( device->index, device->writeBuffer );
						if( result == AIOUSB_SUCCESS ) {
							// verify values written
/*API*/						result = DIO_ReadAll( device->index, device->readBuffer );
							if( result == AIOUSB_SUCCESS ) {
								correct = AIOUSB_TRUE;
								for( port = 0; port < device->numDIOBytes; port++ ) {
									if( device->readBuffer[ port ] != device->writeBuffer[ port ] ) {
										allCorrect = correct = AIOUSB_FALSE;
										break;		// from for()
									}	// if( device->readBuffer[ ...
								}	// for( port ...
								if( ! correct )
									printf( "Error in data read back from device at index %d\n", device->index );
							} else {
								printf( "Error '%s' reading inputs from device at index %d\n"
/*API*/								, AIOUSB_GetResultCodeAsString( result ), device->index );
								goto abort;
							}	// if( result ...
						} else {
							printf( "Error '%s' writing outputs to device at index %d\n"
/*API*/							, AIOUSB_GetResultCodeAsString( result ), device->index );
							goto abort;
						}	// if( result ...
					}	// for( index ...
					sleep( 1 );
				}	// for( pattern ...
abort:;
				printf(
					allCorrect
						? "\nAll patterns written were read back correctly\n"
						: "\n"					// error messages already printed
				);
			} else
				printf( "Failed to find %d USB-DIO-32 devices\n", DEVICES_REQUIRED );
		} else
			printf( "No ACCES devices found on USB bus\n" );

		/*
		 * MUST call AIOUSB_Exit() before program exits,
		 * but only if AIOUSB_Init() succeeded
		 */
/*API*/	AIOUSB_Exit();
	}	// if( result ...
	return ( int ) result;
}	// main()


/* end of file */
