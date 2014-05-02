/** 
 *  $Date $Format: %ad$$
 *  $Author $Format: %an <%ae>$$
 *  $Release $Format: %t$$
 *  @desc Sample program to run the USB-IDIO-16 
 */
#include "aiousb.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char *argv[] )
{
  unsigned long productId, nameSize, numDIOBytes, numCounters;
  const int MAX_NAME_SIZE = 20;
  char name[ MAX_NAME_SIZE + 2 ];
  int stopval;

    unsigned long deviceIndex = 0;
    AIOUSB_BOOL deviceFound = AIOUSB_FALSE;
  printf(
         "USB-AI16-16A sample program\n"
         "  AIOUSB library version %s, %s\n"
         "  This program demonstrates controlling a USB-AI16-16A device on\n"
         "  the USB bus. For simplicity, it uses the first such device found\n"
         "  on the bus.\n"
         , AIOUSB_GetVersion(), AIOUSB_GetVersionDate()
         );

  /*
   * MUST call AIOUSB_Init() before any meaningful AIOUSB functions;
   * AIOUSB_GetVersion() above is an exception
   */
  unsigned long result = AIOUSB_Init();

  /*
   * call GetDevices() to obtain "list" of devices found on the bus
   */
  unsigned long deviceMask = GetDevices();
  if( deviceMask != 0 ) {
    /*
     * at least one ACCES device detected, but we want one of a specific type
     */
    AIOUSB_ListDevices();				// print list of all devices found on the bus

    while( deviceMask != 0 ) {
      if( ( deviceMask & 1 ) != 0 ) {
        // found a device, but is it the correct type?
        nameSize = MAX_NAME_SIZE;
        result = QueryDeviceInfo( deviceIndex, &productId, &nameSize, name, &numDIOBytes, &numCounters );
        if( result == AIOUSB_SUCCESS ) {
          if( productId >= 32792 || productId == 32796 ) {
            // found a USB-AI16-16A family device
            deviceFound = AIOUSB_TRUE;
            break;				// from while()
          }	// if( productId ...
        } else
          printf( "Error '%s' querying device at index %lu\n"
                  , AIOUSB_GetResultCodeAsString( result ), deviceIndex );
      }	// if( ( deviceMask ...
      deviceIndex++;
      deviceMask >>= 1;
    }	// while( deviceMask ...
  }

  if( deviceFound != AIOUSB_TRUE ) { 
    printf("Card with board id '0x%x' is not supported by this sample\n", (int)productId );
    _exit(1);
  }

  if ( productId  == 32792 ) { 
    stopval = 16;
  } else { 
    stopval = 8;
  }

  int timeout = 1000;
  AIOUSB_Reset( deviceIndex );
  AIOUSB_SetCommTimeout( deviceIndex, timeout );
  unsigned outData = 15;
  /* outData = new_usp() */
  /* usp_assign(outData, 15 ); */
  DIO_WriteAll( deviceIndex, &outData );
  /* unsigned readData; */
  /* readData = new_usp(); */
  /* usp_assign(readData ,0); */
  if( 0 ) { 
    for ( outData = 0; outData < 255; outData ++ ) {
      DIO_WriteAll( deviceIndex, &outData );
      usleep(40000);
    }
  
    for ( outData = 0; outData < stopval; outData ++ ) {
      unsigned output =  (int)pow(2,(double)outData);
      DIO_WriteAll( deviceIndex, &output );
      sleep(1);
    }
  }
  DIOBuf *buf= NewDIOBuf(0);
  DIO_ReadAll( deviceIndex, buf );
  printf("Buf: %s\n", DIOBufToString( buf ) );
}
/* for i=0:255 */
/*     val=sprintf('Sending %d\n',i); */
/*     usp_assign(outData, i ); */
/*     result = DIO_WriteAll( deviceIndex, outData ); */
/*     ## result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',i) ); */
/*     ## printf(val); */
/*     disp(val); */
/*     pause(0.04); */
/* end */

/* for i=0:stopval */
/*     val=sprintf('Sending %d',2^i); */
/*     usp_assign(outData, 2^i ); */
/*     DIO_WriteAll(deviceIndex, outData ); */
/*     ## result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',2^i) ); */
/*     disp(val); */
/*     pause(1);     */
/* end */
/* usp_assign(outData, 32411 ); */
/* DIO_WriteAll(deviceIndex, outData ); */
/* disp('Reading back data'); */
/* result = DIO_Read1( deviceIndex, 16+1,readData ); */
/* printf("Value read back was '%d'\n", usp_value(readData)); */
/* ## exit(); */
