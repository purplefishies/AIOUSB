/*
 * $RCSfile: sample2.c,v $
 * $Revision: 1.3 $
 * $Date: 2009/12/22 22:23:35 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * AIOUSB library sample program
 */


#include <aiousb.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

int main( int argc, char **argv ) {
  
  unsigned long deviceMask;
  unsigned long result;
  __uint64_t serialNumber;
  double clockHz;
  const int BULK_BYTES = 100000; /* scans */
  unsigned long bytesRemaining = BULK_BYTES;
 
  printf(
         "USB-AI16-16A sample program version 1.3, 22 December 2009\n"
         "  AIOUSB library version %s, %s\n"
         "  This program demonstrates controlling a USB-AI16-16A device on\n"
         "  the USB bus. For simplicity, it uses the first such device found\n"
         "  on the bus.\n",AIOUSB_GetVersion(), AIOUSB_GetVersionDate() );

  AIOUSB_Init();

  /*
   * call GetDevices() to obtain "list" of devices found on the bus
   */
  GetDevices();

  /*
   * at least one ACCES device detected, but we want one of a specific type
   */
  AIOUSB_ListDevices(); // print list of all devices found on the bus
  const int MAX_NAME_SIZE = 20;
  char name[ MAX_NAME_SIZE + 2 ];
  unsigned long productID, nameSize, numDIOBytes, numCounters;
  unsigned long deviceIndex = 0;
  AIOUSB_BOOL deviceFound = AIOUSB_FALSE;
  while( deviceMask != 0 ) {
    if( ( deviceMask & 1 ) != 0 ) {
      // found a device, but is it the correct type?
      nameSize = MAX_NAME_SIZE;
      result = QueryDeviceInfo( deviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
      if( result == AIOUSB_SUCCESS ) {
        if(
           productID >= USB_AI16_16A
           && productID <= USB_AIO12_128E
           ) {
          // found a USB-AI16-16A family device
          deviceFound = AIOUSB_TRUE;
          break;				// from while()
        }	// if( productID ...
      } else
        printf( "Error '%s' querying device at index %lu\n"
                , AIOUSB_GetResultCodeAsString( result ), deviceIndex );
    }	// if( ( deviceMask ...
    deviceIndex++;
    deviceMask >>= 1;
  }	// while( deviceMask ...


  AIOUSB_Reset( deviceIndex );
  AIOUSB_SetCommTimeout( deviceIndex, 1000 );
  AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB_TRUE );

			
  result = GetDeviceSerialNumber( deviceIndex, &serialNumber );
			

  /*
   * demonstrate A/D configuration; there are two ways to configure the A/D;
   * one way is to create an ADConfigBlock instance and configure it, and then
   * send the whole thing to the device using ADC_SetConfig(); the other way
   * is to use the discrete API functions such as ADC_SetScanLimits(), which
   * send the new settings to the device immediately; here we demonstrate the
   * ADConfigBlock technique; below we demonstrate use of the discrete functions
   */
  ADConfigBlock configBlock;



  AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
  AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
  AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL );
  AIOUSB_SetTriggerMode( &configBlock, 0 );
  AIOUSB_SetScanRange( &configBlock, 2, 13 );
  AIOUSB_SetOversample( &configBlock, 0 );
  result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );



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
    result = ADC_SetCal( deviceIndex, ":AUTO:" );
				
    /*
     * verify that A/D ground calibration is correct
     */


    ADC_SetOversample( deviceIndex, 0 );
    ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL );
    ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_GROUND );
    result = ADC_GetScan( deviceIndex, counts );


    /*
     * verify that A/D reference calibration is correct
     */
    ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_REFERENCE );
    result = ADC_GetScan( deviceIndex, counts );



    ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
    ADC_SetOversample( deviceIndex, 10 );
    ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
    ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );


    result = ADC_GetScanV( deviceIndex, volts );
				
    /*
     * demonstrate reading a single channel in volts
     */
    result = ADC_GetChannelV( deviceIndex, CAL_CHANNEL, &volts[ CAL_CHANNEL ] );
				

    /*
     * demonstrate bulk acquire
     */
    AIOUSB_Reset( deviceIndex );
    ADC_SetOversample( deviceIndex, 10 );
    ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
    AIOUSB_SetStreamingBlockSize( deviceIndex, 100000 );

    const int BULK_BYTES = 100000 /* scans */
      * NUM_CHANNELS
      * sizeof( unsigned short ) /* bytes / sample */
      * 11 /* 1 sample + 10 oversamples */;
    const double CLOCK_SPEED = 100000;	// Hz
    unsigned short *const dataBuf = ( unsigned short * ) malloc( BULK_BYTES );
				

    CTR_StartOutputFreq( deviceIndex, 0, &clockHz );

    /*
     * configure A/D for timer-triggered acquisition
     */
    ADC_ADMode( deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );

					
    AIOUSB_SetMiscClock( deviceIndex, CLOCK_SPEED );
    result = ADC_BulkAcquire( deviceIndex, BULK_BYTES, dataBuf );
					
						
    result = ADC_BulkPoll( deviceIndex, &bytesRemaining );
    /*
     * turn off timer-triggered mode
     */
    ADC_ADMode( deviceIndex, 0, AD_CAL_MODE_NORMAL );

						
									
    AIOUSB_GetResultCodeAsString( result ) ;
		
  /*
   * MUST call AIOUSB_Exit() before program exits,
   * but only if AIOUSB_Init() succeeded
   */
    AIOUSB_Exit();
  }	// if( result ...
  return ( int ) result;
}	// main()


/* end of file */
