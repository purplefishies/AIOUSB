#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>

int 
main(int argc, char *argv[] ) 
{
  int bufsize = 100000;
  int number_channels = 16;
  AIOContinuousBuf *buf = 0;
  buf = (AIOContinuousBuf *)NewAIOContinuousBuf( bufsize , number_channels );
  int tmpsize = pow(512,(double)ceil( ((double)log((double)(bufsize/1000))) / log(16)));
  int keepgoing = 1;
  AIORET_TYPE retval;
  AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);

  AIOUSB_Init();
  GetDevices();

  /**
   * 1. Each buf should have a device index associated with it, so 
   */
  AIOContinuousBuf_SetDeviceIndex( buf, 0 );

  /**
   * 2. Setup the Config object for Acquisition
   */
  /* ADConfigBlock configBlock; */
  /* AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE ); */
  /* AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_0_5V, AIOUSB_FALSE ); */
  /* AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /\* 0x05 *\/ */
  /* AIOUSB_SetScanRange( &configBlock, 0, 15 ); */
  /* ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) ); */
  /* result = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size ); */
  /* or ... */
  retval = AIOContinuousBufSimpleSetupConfig( buf, AD_GAIN_CODE_0_5V );

  if ( retval < AIOUSB_SUCCESS ) {
    printf("Error setting up Simple Config\n");
    _exit(1);
  }
  
  /**
   * 3. Setup the sampling clock rate, in this case 
   *    10_000_000 / 1000
   */ 
  AIOContinuousBufSetClock( buf, 1000 );

  /**
   * 4. Start the Callback that fills up the 
   *    AIOContinuousBuf. This fires up an thread that 
   *    performs the acquistion, while you go about 
   *    doing other things.
   */ 
  AIOContinuousBufCallbackStart( buf );

  while ( keepgoing ) {
    /**
     * You can optionally read values
     * retval = AIOContinuousBufRead( buf, tmp, tmpsize ); 
     */

    sleep(1);
    printf("Waiting : readpos=%d, writepos=%d\n", 
           AIOContinuousBufGetReadPosition(buf),
           AIOContinuousBufGetWritePosition(buf)
           );

    if( AIOContinuousBufGetWritePosition(buf) > tmpsize ) {
      keepgoing = 0;
      AIOContinuousBufEnd( buf );
    }
  }
  while ( AIOContinuousBufGetReadPosition(buf) < AIOContinuousBufGetWritePosition(buf) ){ 
    /**
     * in this example we read bytes in blocks of 16 , to preserve
     * the channel order
     */
    retval = AIOContinuousBufRead( buf, tmp, 16 );
    if ( retval < AIOUSB_SUCCESS ) {
      printf("ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf) );
    } else {
      for ( int i = 0; i < 16 ; i ++ ) { 
        printf( "%f,", tmp[i] );
      }
      printf("\n");
    }
  }

  printf("Test completed...exiting\n");
  

}
