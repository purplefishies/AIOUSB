#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>


using namespace AIOUSB;
using namespace std;


int 
main(int argc, char *argv[] ) 
{
  int bufsize = 10000000;
  unsigned tmpsize = 2048;
  int keepgoing = 1;
  AIORET_TYPE retval;
  AIOBufferType *tmpbuf = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);
  int count_read = 0;
  int fail_count = 0;
  int write_data = 0;
  FILE *fp = fopen("output.dat","w");
  if ( !fp ) {
    fprintf(stderr,"Can't open file 'output.dat' for writing\n");
    _exit(2);
  }
  AIOUSB_Init();
  GetDevices();
  AIOContinuousBuf *buf= NewAIOContinuousBuf( 0, bufsize , 16 );

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
   * 3. Setup the Divide by value, in this case 
   *    10_000_000 / 1000
   */ 
  AIOContinuousBufSetClock( buf, 30000 );

  /**
   * 4. Start the Callback that fills up the 
   *    AIOContinuousBuf. This fires up an thread that 
   *    performs the acquistion, while you go about 
   *    doing other things.
   */ 
  AIOContinuousBufCallbackStart( buf );

  printf("Using tmpsize: %d\n",tmpsize);
  while ( keepgoing ) {
    /**
     * You can optionally read values
     * retval = AIOContinuousBufRead( buf, tmpbuf, tmpsize ); 
     */
    printf("Waiting : readpos=%d, writepos=%d\n", 
           AIOContinuousBufGetReadPosition(buf),
           AIOContinuousBufGetWritePosition(buf)
           );
    
    if( AIOContinuousBufAvailableReadSize(buf) >= tmpsize ) {
      retval = AIOContinuousBufRead( buf, tmpbuf, tmpsize );
      if ( retval < AIOUSB_SUCCESS ) {
        printf("ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf) );
      } else if( retval == 0 ) {
        fail_count += 1;
        printf("Fail : %d\n", fail_count );
      }
      // else {
      // printf("Retval: %d\n", (int)retval );
      // };
      count_read += retval;
      if( fail_count > 5 ) {
        keepgoing = 0;
        AIOContinuousBufEnd( buf );
        fprintf(stderr,"Error: too many failed reads\n");
      } else { 
        for( int j = 0; j < retval / 16 ; j ++ ) {
          write_data = fwrite( &tmpbuf[j*16], sizeof(double), 16 , fp );
        }
        if( count_read > 300000 ) {
          keepgoing = 0;
          AIOContinuousBufEnd( buf );
        }
      }
    } else {
      sleep(1);
    }
  }
  printf("Test completed...exiting\n");
}
