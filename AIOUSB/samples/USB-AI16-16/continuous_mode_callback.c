#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <AIODataTypes.h>
#include "AIOCountsConverter.h"
#include "AIOUSB_Log.h"
#include "aiocommon.h"
#include <getopt.h>

struct channel_range *get_channel_range( char *optarg );
void process_cmd_line( struct opts *, int argc, char *argv[] );

AIOUSB_BOOL fnd( AIOUSBDevice *dev ) { 
    if ( dev->ProductID >= USB_AI16_16A && dev->ProductID <= USB_AI12_128E ) { 
        return AIOUSB_TRUE;
    } else if ( dev->ProductID >=  USB_AIO16_16A && dev->ProductID <= USB_AIO12_128E ) {
        return AIOUSB_TRUE;
    } else {
        return AIOUSB_FALSE;
    }
}

FILE *fp;

AIORET_TYPE capture_data( AIOContinuousBuf *buf ) { 
    unsigned short tobuf[1024];
    int num_samples_to_read = AIOContinuousBufGetNumberChannels(buf)*(1+AIOContinuousBufGetOversample(buf));
    int data_read = AIOContinuousBufPopN( buf, tobuf, num_samples_to_read );
    for ( int i = 0; i < data_read / 2 ;i ++ ) { 
        fprintf(fp,"%u,",tobuf[i] );
    }
    fprintf(fp,"\n");
    fflush(fp);
    return (AIORET_TYPE)data_read;
}


int 
main(int argc, char *argv[] ) 
{
    struct opts options = AIO_OPTIONS;
    AIOContinuousBuf *buf = 0;
    
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int *indices;
    int num_devices;

    process_aio_cmd_line( &options, argc, argv );

    AIOUSB_Init();
    AIOUSB_ListDevices();

#ifdef __GNUC__
    AIOUSB_FindDevices( &indices, &num_devices, LAMBDA( AIOUSB_BOOL, (AIOUSBDevice *dev), { 
                if ( dev->ProductID >= USB_AI16_16A && dev->ProductID <= USB_AI12_128E ) { 
                    return AIOUSB_TRUE;
                } else if ( dev->ProductID >=  USB_AIO16_16A && dev->ProductID <= USB_AIO12_128E ) {
                    return AIOUSB_TRUE;
                } else {
                    return AIOUSB_FALSE;
                }
            } ) 
        );
#else
    AIOUSB_FindDevices( &indices, &num_devices, fnd );
#endif

    if( (retval = aio_list_devices( &options, indices, num_devices ) != AIOUSB_SUCCESS )) 
        exit(retval);

    if ( (buf = NewAIOContinuousBufFromJSON( options.aiobuf_json )) == NULL )
        exit(AIOUSB_ERROR_INVALID_AIOCONTINUOUS_BUFFER);

    /* This fn allows us to merge the original JSON settings with 
     * some explicity command line options
     */
    if ( (retval = aio_override_aiobuf_settings( buf, &options )) != AIOUSB_SUCCESS )
        exit(retval);

    fp = fopen(options.outfile,"w");
    if( !fp ) {
        fprintf(stderr,"Unable to open '%s' for writing\n", options.outfile );
        exit(1);
    }

    AIOCmd cmd = {.channel = AIO_PER_SCANS};

    AIOContinuousBufInitiateCallbackAcquisition(buf); /* Start the acquisition */
#if __GNUC__
    AIOContinuousBufCallbackStartCallbackAcquisition( buf, &cmd, capture_data );
#else
    AIOContinuousBufCallbackStartCallbackAcquisition( buf, &cmd, LAMBDA( AIORET_TYPE, (AIOContinuousBuf *buf), {
                unsigned short tobuf[1024];
                int num_samples_to_read = AIOContinuousBufGetNumberChannels(buf)*(1+AIOContinuousBufGetOversample(buf));
                int data_read = AIOContinuousBufPopN( buf, tobuf, num_samples_to_read );
                for ( int i = 0; i < data_read / 2 ;i ++ ) { 
                    fprintf(fp,"%u,",tobuf[i] );
                }
                fprintf(fp,"\n");
                fflush(fp);
                return (AIORET_TYPE)data_read;
                })
        );
#endif

    fclose(fp);
    fprintf(stderr,"Test completed...exiting\n");
    retval = ( retval >= 0 ? 0 : - retval );
    return(retval);
}
