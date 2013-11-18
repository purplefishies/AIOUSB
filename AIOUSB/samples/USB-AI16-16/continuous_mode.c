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
  int tmpsize = pow(512,(double)ceil( ((double)log((double)(bufsize/1000))) / log(16)));
  int keepgoing = 1;
  unsigned long result;
  AIORET_TYPE retval;
  AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);

  AIOUSB_Init();
  GetDevices();

  /* Each buf should have a device index associated with it */
  AIOContinuousBuf_SetDeviceIndex( buf, 0 );

  /* Setup the Config object */
  AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE );
  AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_0_5V, AIOUSB_FALSE );
  AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /* 0x05 */
  AIOUSB_SetScanRange( &configBlock, 0, 15 );
  ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) );

  result = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size );

  if ( result != AIOUSB_SUCCESS ) {
    printf("Error reading config\n");
  }

  AIOContinuousBufSetClock( buf, 1000 );
  AIOContinuousBufCallbackStartClocked( buf );

  while ( keepgoing ) {
    /* retval = AIOContinuousBufRead( buf, tmp, tmpsize ); */
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
    if( AIOContinuousBufGetWritePosition(buf) > tmpsize ) {
      keepgoing = 0;
      AIOContinuousBufEnd( buf );
    }
  }
  while ( AIOContinuousBufGetReadPosition(buf) < AIOContinuousBufGetWritePosition(buf) ){ 
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
