/*
 * $Date $Format: %ad$$
 * $Author $Format: %an <%ae>$
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
 *     gcc -std=gnu99 -D_GNU_SOURCE -Wall -pthread -fPIC sample2.c -laiousb -lusb-1.0 -lm -o sample2
 *
 * or, to enable debug features
 *
 *     gcc -ggdb -std=gnu99 -D_GNU_SOURCE -Wall -pthread -fPIC sample2.c -laiousbdbg -lusb-1.0 -lm -o sample2
 */
// }}}

// {{{ includes
#include <aiousb.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
// }}}

int main( int argc, char **argv ) {
	printf(
		"USB-AI16-16A sample program version 1.3, 22 December 2009\n"
		"  AIOUSB library version %s, %s\n"
		"  This program demonstrates controlling a USB-AI16-16A device on\n"
		"  the USB bus. For simplicity, it uses the first such device found\n"
		"  on the bus.\n"
/*API*/	, AIOUSB_GetVersion(), AIOUSB_GetVersionDate()
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
			 * at least one ACCES device detected, but we want one of a specific type
			 */
/*API*/		AIOUSB_ListDevices();				// print list of all devices found on the bus
			const int MAX_NAME_SIZE = 20;
			char name[ MAX_NAME_SIZE + 2 ];
			unsigned long productID, nameSize, numDIOBytes, numCounters;
			unsigned long deviceIndex = 0;
			AIOUSB_BOOL deviceFound = AIOUSB_FALSE;
			while( deviceMask != 0 ) {
				if( ( deviceMask & 1 ) != 0 ) {
					// found a device, but is it the correct type?
					nameSize = MAX_NAME_SIZE;
/*API*/				result = QueryDeviceInfo( deviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
					if( result == AIOUSB_SUCCESS ) {
						if(
							productID >= USB_AI16_16A
							&& productID <= USB_AI12_128E
						) {
							// found a USB-AI16-16A family device
							deviceFound = AIOUSB_TRUE;
							break;				// from while()
						}	// if( productID ...
					} else
						printf( "Error '%s' querying device at index %lu\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ), deviceIndex );
				}	// if( ( deviceMask ...
				deviceIndex++;
				deviceMask >>= 1;
			}	// while( deviceMask ...
			if( deviceFound ) {
/*API*/			AIOUSB_Reset( deviceIndex );
/*API*/			AIOUSB_SetCommTimeout( deviceIndex, 1000 );
/*API*/			AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB_TRUE );

				__uint64_t serialNumber;
/*API*/			result = GetDeviceSerialNumber( deviceIndex, &serialNumber );
				if( result == AIOUSB_SUCCESS )
					printf( "Serial number of device at index %lu: %llx\n", deviceIndex, ( long long ) serialNumber );
				else
					printf( "Error '%s' getting serial number of device at index %lu\n"
/*API*/					, AIOUSB_GetResultCodeAsString( result ), deviceIndex );

				/*
				 * demonstrate A/D configuration; there are two ways to configure the A/D;
				 * one way is to create an ADConfigBlock instance and configure it, and then
				 * send the whole thing to the device using ADC_SetConfig(); the other way
				 * is to use the discrete API functions such as ADC_SetScanLimits(), which
				 * send the new settings to the device immediately; here we demonstrate the
				 * ADConfigBlock technique; below we demonstrate use of the discrete functions
				 */
				ADConfigBlock configBlock;
/*API*/			AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
/*API*/			AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
/*API*/			AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL );
/*API*/			AIOUSB_SetTriggerMode( &configBlock, 0 );
/*API*/			AIOUSB_SetScanRange( &configBlock, 2, 13 );
/*API*/			AIOUSB_SetOversample( &configBlock, 0 );
/*API*/			result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );
				if( result == AIOUSB_SUCCESS ) {
					const int CAL_CHANNEL = 5;
					const int MAX_CHANNELS = 128;
					const int NUM_CHANNELS = 16;
					unsigned short counts[ MAX_CHANNELS ];
					double volts[ MAX_CHANNELS ];
					unsigned char gainCodes[ NUM_CHANNELS ];
					printf( "A/D settings successfully configured\n" );

					/*
					 * demonstrate automatic A/D calibration
					 */
/*API*/				result = ADC_SetCal( deviceIndex, ":AUTO:" );
					if( result == AIOUSB_SUCCESS )
						printf( "Automatic calibration completed successfully\n" );
					else
						printf( "Error '%s' performing automatic A/D calibration\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ) );

					/*
					 * verify that A/D ground calibration is correct
					 */
/*API*/				ADC_SetOversample( deviceIndex, 0 );
/*API*/				ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL );
/*API*/				ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_GROUND );
/*API*/				result = ADC_GetScan( deviceIndex, counts );
					if( result == AIOUSB_SUCCESS )
						printf( "Ground counts = %u (should be approx. 0)\n", counts[ CAL_CHANNEL ] );
					else
						printf( "Error '%s' attempting to read ground counts\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ) );

					/*
					 * verify that A/D reference calibration is correct
					 */
/*API*/				ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_REFERENCE );
/*API*/				result = ADC_GetScan( deviceIndex, counts );
					if( result == AIOUSB_SUCCESS )
						printf( "Reference counts = %u (should be approx. 65130)\n", counts[ CAL_CHANNEL ] );
					else
						printf( "Error '%s' attempting to read reference counts\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ) );

					/*
					 * demonstrate scanning channels and measuring voltages
					 */
					for( int channel = 0; channel < NUM_CHANNELS; channel++ )
						gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
/*API*/				ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
/*API*/				ADC_SetOversample( deviceIndex, 10 );
/*API*/				ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
/*API*/				ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
/*API*/				result = ADC_GetScanV( deviceIndex, volts );
					if( result == AIOUSB_SUCCESS ) {
						printf( "Volts read:\n" );
						for( int channel = 0; channel < NUM_CHANNELS; channel++ )
							printf( "  Channel %2d = %f\n", channel, volts[ channel ] );
					} else
						printf( "Error '%s' performing A/D channel scan\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result ) );

					/*
					 * demonstrate reading a single channel in volts
					 */
/*API*/				result = ADC_GetChannelV( deviceIndex, CAL_CHANNEL, &volts[ CAL_CHANNEL ] );
					if( result == AIOUSB_SUCCESS )
						printf( "Volts read from A/D channel %d = %f\n", CAL_CHANNEL, volts[ CAL_CHANNEL ] );
					else
						printf( "Error '%s' reading A/D channel %d\n"
/*API*/						, AIOUSB_GetResultCodeAsString( result )
							, CAL_CHANNEL );

					/*
					 * demonstrate bulk acquire
					 */
/*API*/				AIOUSB_Reset( deviceIndex );
/*API*/				ADC_SetOversample( deviceIndex, 10 );
/*API*/				ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
/*API*/				AIOUSB_SetStreamingBlockSize( deviceIndex, 100000 );
					const int BULK_BYTES = 100000 /* scans */
						* NUM_CHANNELS
						* sizeof( unsigned short ) /* bytes / sample */
						* 11 /* 1 sample + 10 oversamples */;
					const double CLOCK_SPEED = 100000;	// Hz
					unsigned short *const dataBuf = ( unsigned short * ) malloc( BULK_BYTES );
					if( dataBuf != 0 ) {
						/*
						 * make sure counter is stopped
						 */
						double clockHz = 0;
/*API*/					CTR_StartOutputFreq( deviceIndex, 0, &clockHz );

						/*
						 * configure A/D for timer-triggered acquisition
						 */
/*API*/					ADC_ADMode( deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );

						/*
						 * start bulk acquire; ADC_BulkAcquire() will take care of starting
						 * and stopping the counter; but we do have to tell it what clock
						 * speed to use, which is why we call AIOUSB_SetMiscClock()
						 */
/*API*/					AIOUSB_SetMiscClock( deviceIndex, CLOCK_SPEED );
/*API*/					result = ADC_BulkAcquire( deviceIndex, BULK_BYTES, dataBuf );
						if( result == AIOUSB_SUCCESS )
							printf( "Started bulk acquire of %d bytes\n", BULK_BYTES );
						else
							printf( "Error '%s' attempting to start bulk acquire of %d bytes\n"
/*API*/							, AIOUSB_GetResultCodeAsString( result )
								, BULK_BYTES );

						/*
						 * use bulk poll to monitor progress
						 */
						if( result == AIOUSB_SUCCESS ) {
							unsigned long bytesRemaining = BULK_BYTES;
							for( int seconds = 0; seconds < 100; seconds++ ) {
								sleep( 1 );
/*API*/							result = ADC_BulkPoll( deviceIndex, &bytesRemaining );
								if( result == AIOUSB_SUCCESS ) {
									printf( "  %lu bytes remaining\n", bytesRemaining );
									if( bytesRemaining == 0 )
										break;	// from for()
								} else {
									printf( "Error '%s' polling bulk acquire progress\n"
/*API*/									, AIOUSB_GetResultCodeAsString( result ) );
									break;		// from for()
								}	// if( result ...
							}	// for( int seconds ...

							/*
							 * turn off timer-triggered mode
							 */
/*API*/						ADC_ADMode( deviceIndex, 0, AD_CAL_MODE_NORMAL );

							/*
							 * if all the data was apparently received, scan it for zeros; it's
							 * unlikely that any of the data would be zero, so any zeros, particularly
							 * a large block of zeros would suggest that the data is not valid
							 */
							if(
								result == AIOUSB_SUCCESS
								&& bytesRemaining == 0
							) {
								AIOUSB_BOOL anyZeroData = AIOUSB_FALSE;
								int zeroIndex = -1;
								for( int index = 0; index < BULK_BYTES / ( int ) sizeof( unsigned short ); index++ ) {
									if( dataBuf[ index ] == 0 ) {
										anyZeroData = AIOUSB_TRUE;
										if( zeroIndex < 0 )
											zeroIndex = index;
									} else {
										if( zeroIndex >= 0 ) {
											printf( "  Zero data from index %d to %d\n", zeroIndex, index - 1 );
											zeroIndex = -1;
										}	// if( zeroIndex ...
									}	// if( dataBuf[ ...
								}	// for( int index ...
								if( anyZeroData == AIOUSB_FALSE )
									printf( "Successfully bulk acquired %d bytes\n", BULK_BYTES );
							} else
								printf( "Failed to bulk acquire %d bytes\n", BULK_BYTES );
						}	// if( result ...

						free( dataBuf );
					}	// if( dataBuf ...
				} else
					printf( "Error '%s' setting A/D configuration\n"
/*API*/					, AIOUSB_GetResultCodeAsString( result ) );
			} else
				printf( "Failed to find USB-AI16-16A device\n" );
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
