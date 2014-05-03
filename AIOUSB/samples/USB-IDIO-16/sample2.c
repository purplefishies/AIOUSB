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

    DIO_WriteAll( deviceIndex, &outData );

    if( 1 ) { 
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
    outData = 0x5465;
    DIO_WriteAll(deviceIndex, &outData );

    DIOBuf *buf= NewDIOBuf(0);
    int cdat;
    DIO_ReadAll( deviceIndex, buf );
    printf("Binary was: %s\n", DIOBufToString( buf ) );
    printf("Hex was: %s\n", DIOBufToHex( buf ) );
    DIO_Read8( deviceIndex, 0, &cdat  );
    printf("Single data was : hex:%x, int:%d\n", (int)cdat, (int)cdat );
    DIO_Read8( deviceIndex, 1, &cdat  );
    printf("Single data was : hex:%x, int:%d\n", (int)cdat, (int)cdat );
    DIO_Read8( deviceIndex, 2, &cdat  );
    printf("Single data was : hex:%x, int:%d\n", (int)cdat, (int)cdat );
    DIO_Read8( deviceIndex, 3, &cdat   );
    printf("Single data was : hex:%x, int:%d\n", (int)cdat, (int)cdat );

    int val=0;
    for ( int i = 7 ; i >= 0 ; i-- ) {
      DIO_Read1(deviceIndex,i, &val);
      printf("%d", val );
    }
    printf("\n-----\n");
    for ( int i = 15 ; i >= 8 ; i -- ) {
      DIO_Read1(deviceIndex,i, &val);
      printf("%d", val );
    }
    printf("\n");
    AIOUSB_Exit();
    DeleteDIOBuf( buf );
}

