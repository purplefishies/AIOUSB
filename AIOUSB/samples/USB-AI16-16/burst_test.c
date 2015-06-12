/**
 * @file   burst_test.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %h$
 * @brief  
 */

#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>
#include "AIOUSB_Log.h"
#include "aiocommon.h"
#include <getopt.h>
#include <ctype.h>
#include <time.h>

#define  _FILE_OFFSET_BITS 64  


void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize);
void process_with_looping_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize);

AIOUSB_BOOL find_ai_board( AIOUSBDevice *dev ) { 
    if ( dev->ProductID >= USB_AI16_16A && dev->ProductID <= USB_AI12_128E ) { 
        return AIOUSB_TRUE;
    } else if ( dev->ProductID >=  USB_AIO16_16A && dev->ProductID <= USB_AIO12_128E ) {
        return AIOUSB_TRUE;
    } else {
            return AIOUSB_FALSE;
    }
}


int 
main(int argc, char *argv[] ) 
{
    struct opts options = AIO_OPTIONS;
    AIOContinuousBuf *buf = 0;
    struct timespec foo , bar;

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int *indices;
    int num_devices;
    process_aio_cmd_line( &options, argc, argv );

    int tobufsize = (options.num_scans+1)*options.num_channels*20;
    uint16_t *tobuf = (uint16_t *)malloc(sizeof(uint16_t)*tobufsize);

    unsigned short *tmp = (unsigned short *)malloc(sizeof(unsigned short)*(options.num_scans+1)*options.num_channels);
    if( !tmp ) {
      fprintf(stderr,"Can't allocate memory for temporary buffer \n");
      exit(1);
    }

    AIOUSB_Init();
    AIOUSB_ListDevices();

    AIOUSB_FindDevices( &indices, &num_devices, find_ai_board );
    aio_list_devices( &options, indices, num_devices ); /* will exit if no devices found */

    buf = (AIOContinuousBuf *)NewAIOContinuousBufForCounts( options.index, options.num_scans, options.num_channels );
    if( !buf ) {
      fprintf(stderr,"Can't allocate memory for temporary buffer \n");
      exit(1);
    }

    AIOContinuousBufSetDeviceIndex( buf, options.index ); /* Assign the first matching device for this sample */

    if( options.reset ) {
        fprintf(stderr,"Resetting device at index %d\n",buf->DeviceIndex );
        AIOContinuousBufResetDevice( buf );
        exit(0);
    }
    FILE *fp = fopen(options.outfile,"w");
    if( !fp ) {
      fprintf(stderr,"Unable to open '%s' for writing\n", options.outfile );
      exit(1);
    }

    /**
     * 2. Setup the Config object for Acquisition, either the more complicated 
     *    part in comments (BELOW) or using a simple interface.
     */
    /* New simpler interface */
    AIOContinuousBufInitConfiguration( buf );

    if ( options.slow_acquire ) {
        unsigned char bufData[64];
        unsigned long bytesWritten = 0;
        GenericVendorWrite( 0, 0xDF, 0x0000, 0x001E, bufData, &bytesWritten  );
    }

    AIOContinuousBufSetOversample( buf, 0 );
    AIOContinuousBufSetStartAndEndChannel( buf, options.start_channel, options.end_channel );
    if( !options.number_ranges ) { 
        AIOContinuousBufSetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    } else {
        for ( int i = 0; i < options.number_ranges ; i ++ ) {
            AIOContinuousBufSetChannelRange( buf, 
                                             options.ranges[i]->start_channel, 
                                             options.ranges[i]->end_channel,
                                             options.ranges[i]->gaincode
                                             );
        }
    }
    AIOContinuousBufSaveConfig(buf);
    
    if ( retval < AIOUSB_SUCCESS ) {
        printf("Error setting up configuration\n");
        exit(1);
    }
  
    options.block_size = ( options.block_size < 0 ? 1024*64 : options.block_size );
    if ( options.clock_rate < 1000 ) { 
        AIOContinuousBufSetStreamingBlockSize( buf, 512 );
    } else  {
        AIOContinuousBufSetStreamingBlockSize( buf, options.block_size );
    }

    /**
     * 3. Setup the sampling clock rate, in this case 
     *    10_000_000 / 1000
     */ 
    AIOContinuousBufSetClock( buf, options.clock_rate );
    /**
     * 4. Start the Callback that fills up the 
     *    AIOContinuousBuf. This fires up an thread that 
     *    performs the acquistion, while you go about 
     *    doing other things.
     */ 
    AIOContinuousBufCallbackStart( buf );

    /**
     * in this example we read bytes in blocks of our core num_channels parameter. 
     * the channel order
     */
    if ( options.with_timing ) 
        clock_gettime( CLOCK_MONOTONIC_RAW, &bar );


    int scans_remaining;
    int read_count = 0;
    int scans_read = 0;
    while ( buf->status == RUNNING || read_count < options.num_scans ) {

        if ( (scans_remaining = AIOContinuousBufCountScansAvailable(buf) ) > 0 ) { 

            if ( scans_remaining ) { 
                if ( options.with_timing )
                    clock_gettime( CLOCK_MONOTONIC_RAW, &foo );

                scans_read = AIOContinuousBufReadIntegerScanCounts( buf, tobuf, tobufsize, AIOContinuousBufNumberChannels(buf)*AIOContinuousBufCountScansAvailable(buf) );

                if ( options.with_timing )
                    clock_gettime( CLOCK_MONOTONIC_RAW, &bar );

                read_count += scans_read;

                if ( options.verbose )
                    fprintf(stdout,"Waiting : total=%u, readpos=%d, writepos=%d, scans_read=%d\n", read_count, 
                            AIOContinuousBufGetReadPosition(buf), AIOContinuousBufGetWritePosition(buf), scans_read);

                for( int scan_count = 0; scan_count < scans_read ; scan_count ++ ) { 
                    if( options.with_timing )
                        fprintf(fp ,"%d,%d,", (int)bar.tv_sec, (int)(( bar.tv_sec - foo.tv_sec )*1e9 + (bar.tv_nsec - foo.tv_nsec )));
                    for( int ch = 0 ; ch < AIOContinuousBufNumberChannels(buf); ch ++ ) {
                        fprintf(fp,"%u,",tobuf[scan_count*AIOContinuousBufNumberChannels(buf)+ch] );
                        if( (ch+1) % AIOContinuousBufNumberChannels(buf) == 0 ) {
                            fprintf(fp,"\n");
                        }
                    }
                }
            }
        } else {
            usleep(100);
        }
        
    }

    fclose(fp);
    fprintf(stdout,"Test completed...exiting\n");

    return 0;
}

void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize)
{

}

