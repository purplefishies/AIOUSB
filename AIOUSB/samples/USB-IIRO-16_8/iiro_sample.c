/** 
 *  @file iiro_sample.c
 *  @date $Format: %ad$$
 *  @author $Format: %an <%ae>$
 *  @release $Format: %h$
 *  @brief Sample program to demonstrate the behavior of the 
 *         USB-IIRO-8 or USB-IIRO-16 cards
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "aiousb.h"
#include "aiocommon.h"

#define RATE_LIMIT(product) \
    do {                                                                \
        if( product >= USB_IIRO_16 && product <= USB_IDIO_16 )          \
            sleep(1);                                                   \
    } while( 0 );                                                       \

AIOUSB_BOOL fnd( AIOUSBDevice *dev ) { 
    if ( dev->ProductID >=  USB_IIRO_16 && dev->ProductID <= USB_IDIO_16 ) { 
        return AIOUSB_TRUE;
    } else {
        return AIOUSB_FALSE;
    }
}

int main(int argc, char *argv[] )
{
    struct opts options = AIO_OPTIONS;
    unsigned long productID, nameSize, numDIOBytes, numCounters;
    int stopval;
    int *indices;
    int num_devices;
    AIORET_TYPE retval;
    int MAX_NAME_SIZE = 20;
    char name[ MAX_NAME_SIZE + 2 ];
    unsigned long deviceIndex = 0;

    nameSize = MAX_NAME_SIZE;


    process_aio_cmd_line( &options, argc, argv );

    AIOUSB_Init();
    AIOUSB_ListDevices();
    AIOUSB_FindDevices( &indices, &num_devices, fnd );

    aio_list_devices( &options, indices, num_devices );

    if( num_devices <= 0 ) {
        fprintf(stderr,"No USB-IIRO cards were found\n" );
        exit(1);
    }
    
    retval = QueryDeviceInfo( options.index, &productID, &nameSize, name, &numDIOBytes, &numCounters );


    if ( productID  == 0x8018 || productID == USB_IIRO_16  ) { 
        stopval = 16;
    } else { 
        stopval = 8;
    }

    int timeout = 1000;
    AIOUSB_Reset( deviceIndex );
    AIOUSB_SetCommTimeout( deviceIndex, timeout );
    unsigned outData = 15;

    retval = DIO_WriteAll( deviceIndex, &outData );


    if( (productID >= USB_IIRO_16 && productID <= USB_IIRO_4 )) {
        goto walkingbit_test;
    }
    /* Speed test for IDIOs not for mechanical switching !! */
    struct timeval start;
    struct timeval now;
    for( int i = 0; i < 50; i ++ ) { 
        int tot = 0;
        gettimeofday( &start, NULL ) ;
        if( 1 ) { 
            for ( outData = 0; outData < stopval ; outData ++  ) {
                DIO_WriteAll( deviceIndex, &outData );
                RATE_LIMIT( productID );
                tot ++;
            }
  
            for ( outData = 0; outData < stopval; outData ++ , tot ++ ) {
                unsigned output = 1 << outData;
                DIO_WriteAll( deviceIndex, &output );
                RATE_LIMIT( productID );
                tot++;
            }
            gettimeofday( &now, NULL ) ;
            printf("%d: num=%d delta=%ld\n", i, tot, (now.tv_usec - start.tv_usec ) + (now.tv_sec - start.tv_sec)*1000000 );
        }
    }
 
 walkingbit_test:
   outData = 0x0; 
   retval = DIO_WriteAll(deviceIndex, &outData );
   unsigned output = 0;
   for ( outData = 0; outData < stopval; outData ++ ) {
       output |= (unsigned)(1 << outData);
       retval = DIO_WriteAll( deviceIndex, &output );
       RATE_LIMIT( productID );
   }

   DIOBuf *buf= NewDIOBuf(0);
   unsigned char cdat;
   retval = DIO_ReadIntoDIOBuf( deviceIndex, buf );
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

   unsigned char val=0;
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
   return (int)retval;
}

