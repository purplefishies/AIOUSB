#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <AIOTypes.h>

int 
main(int argc, char *argv[] ) 
{
  int bufsize = 100000;
  AIOContinuousBuf *buf = NewAIOContinuousBuf( bufsize );
  ADConfigBlock configBlock;
  int tmpsize = pow(16,(double)ceil( ((double)log((double)(bufsize/1000))) / log(16)));
  int keepgoing = 1;
  unsigned long result;
  AIORET_TYPE retval;
  AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);
  /* int ntest_count = 0; */

  AIOUSB_Init();
  GetDevices();

  /* Each buf should have a device index associated with it */
  AIOContinuousBuf_SetDeviceIndex( buf, 0 );

  /* Setup the Config object */
  AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE );
  AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_0_5V, AIOUSB_FALSE );
  /* AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL ); */
  /* AIOUSB_SetTriggerMode( &configBlock, 0 ); */
  AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /* 0x05 */

  AIOUSB_SetScanRange( &configBlock, 0, 15 );
  result = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size );

  /* AIOUSB_SetOversample( &configBlock, 0 ); */
  if ( result != AIOUSB_SUCCESS ) {
    printf("Error reading config\n");
  }
  ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) );
  AIOContinuousBufSetClock( buf, 1000 );
  AIOContinuousBufCallbackStartClocked( buf );

  while ( keepgoing ) {
    retval = AIOContinuousBufRead( buf, tmp, tmpsize );
    sleep(1);
    printf("Waiting : readpos=%d, writepos=%d\n", 
           AIOContinuousBufGetReadPosition(buf),
           AIOContinuousBufGetWritePosition(buf)
           );
    /* AIOUSB_INFO("Waiting : readpos=%d, writepos=%d\n", get_read_pos(buf),get_write_pos(buf)); */
    if( retval < AIOUSB_SUCCESS ) {
      printf("Error found reading: ...exiting\n");
      break;
    }
    if( AIOContinuousBufGetReadPosition(buf) > 4000 ) {
      keepgoing = 0;
      AIOContinuousBufEnd( buf );
    }
  }
  printf("Test completed...exiting\n");
  

}
