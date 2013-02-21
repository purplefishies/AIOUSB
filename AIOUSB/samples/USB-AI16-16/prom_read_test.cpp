/*
 * $RCSfile: sample.cpp,v $
 * $Revision: 1.26 $
 * $Date: 2009/12/22 22:23:35 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
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
#include <unistd.h>
using namespace AIOUSB;
// }}}

int main( int argc, char **argv ) {
  printf("Sample test for Checking the Calibration on the board: %s, %s", AIOUSB_GetVersion(), AIOUSB_GetVersionDate());

  unsigned long result = AIOUSB_Init();

  if( result == AIOUSB_SUCCESS ) {
          
    unsigned long deviceMask = GetDevices();
    if( deviceMask != 0 ) {
      /*
       * at least one ACCES device detected, but we want one of a specific type
       */
      AIOUSB_ListDevices();				// print list of all devices found on the bus
      const int MAX_NAME_SIZE = 20;
      char name[ MAX_NAME_SIZE + 2 ];
      unsigned long productID, nameSize, numDIOBytes, numCounters;
      unsigned long deviceIndex = 0;

      // Extra variables that we might use

      bool deviceFound = false;


      while( deviceMask != 0 ) {
        if( ( deviceMask & 1 ) != 0 ) {
          // found a device, but is it the correct type?
          nameSize = MAX_NAME_SIZE;
          result = QueryDeviceInfo( deviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
          if( result == AIOUSB_SUCCESS ) {
            if(
               productID >= USB_AI16_16A
               && productID <= USB_AI12_128E
               ) {
              // found a USB-AI16-16A family device
              deviceFound = true;
              break;				// from while()
            }	// if( productID ...
          } else
            printf( "Error '%s' querying device at index %lu\n"
                    /*API*/						, AIOUSB_GetResultCodeAsString( result ), deviceIndex );
        }	// if( ( deviceMask ...
        deviceIndex++;
        deviceMask >>= 1;
      }	// while( deviceMask ...


      // Going to apply the new check that was developed
      if( deviceFound ) {

        GetHiRef( deviceIndex );
        // At this point we need to start adding functionality to 
        // The system at hand.
       
        AIOUSB_Reset( deviceIndex );
        AIOUSB_SetCommTimeout( deviceIndex, 1000 );
        AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB_TRUE );

        __uint64_t serialNumber;
        result = GetDeviceSerialNumber( deviceIndex, &serialNumber );
        if( result == AIOUSB_SUCCESS )
          printf( "Serial number of device at index %lu: %llx\n", deviceIndex, ( long long ) serialNumber );
        else
          printf( "Error '%s' getting serial number of device at index %lu\n"
                  , AIOUSB_GetResultCodeAsString( result ), deviceIndex );

			
        ADConfigBlock configBlock;
        AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
        AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
        AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL );
        AIOUSB_SetTriggerMode( &configBlock, 0 );
        AIOUSB_SetScanRange( &configBlock, 2, 13 );
        AIOUSB_SetOversample( &configBlock, 0 );
        result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );

        TestADConfigBlock *TestADConfigBlock = new TestADConfigBlock( );

        result = test_setup_device( 

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
          if( result == AIOUSB_SUCCESS )
            printf( "Automatic calibration completed successfully\n" );
          else
            printf( "Error '%s' performing automatic A/D calibration\n"
                    , AIOUSB_GetResultCodeAsString( result ) );

          /*
           * verify that A/D ground calibration is correct
           */
          ADC_SetOversample( deviceIndex, 0 );
          ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL );
          ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_GROUND );
        }
      }
    } else {
      printf( "No ACCES devices found on USB bus\n" );
    }
    AIOUSB_Exit();
  }
  return ( int ) result;
}	// main()


/* end of file */



