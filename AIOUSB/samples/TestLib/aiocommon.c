#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include "aiocommon.h"
#include "aiousb.h"


struct opts AIO_OPTIONS = {100000, 16, 0, AD_GAIN_CODE_0_5V , 10000 , "output.txt", 0, AIODEFAULT_LOG_LEVEL, 0, 0, 0,15, -1, -1, 0, 0,0, 
                           "{\"DeviceIndex\":1,\"base_size\":512,\"block_size\":65536,\"debug\":\"false\",\"hz\":10000,\"num_channels\":16,\"num_oversamples\":0,\"num_scans\":1024,\"testing\":\"false\",\"timeout\":1000,\"type\":2,\"unit_size\":2}",
                           NULL
};

/*----------------------------------------------------------------------------*/
struct channel_range *get_channel_range(char *optarg )
{
    int i = 0;
  
    typedef enum { 
        BEGIN = 0,
        SCHANNEL,
        ECHANNEL,
        GAIN,
    } MODE;
    int pos;
    char buf[BUFSIZ];
    struct channel_range *tmp = (struct channel_range *)malloc( sizeof(struct channel_range) );
    if ( !tmp ) {
        fprintf(stdout,"Unable to create a new channel range\n");
        return NULL;
    }
    MODE mode = BEGIN;
    for ( i = 0; i < strlen(optarg); i ++ ) {
        if ( mode == BEGIN && isdigit(optarg[i] ) ) {
            pos = i;
            mode = SCHANNEL;
        } else if ( mode == SCHANNEL && isdigit(optarg[i])  ) {
      
        } else if ( mode == SCHANNEL && optarg[i] == '-' ) {
            mode = ECHANNEL;
            strncpy(&buf[0], &optarg[pos], i - pos );
            buf[i-pos] = 0;
            tmp->start_channel = atoi(buf);
            i ++ ;
            pos = i;
        } else if ( mode == SCHANNEL ) {
            fprintf(stdout,"Unknown flag while parsing Start_channel: '%c'\n", optarg[i] );
            free(tmp);
            return NULL;
        } else if ( mode == ECHANNEL && isdigit(optarg[i] ) ) {
      
        } else if ( mode == ECHANNEL && optarg[i] == '=' ) {
            mode = GAIN;
            strncpy(&buf[0], &optarg[pos], i - pos );
            buf[i-pos] = 0;
            tmp->end_channel = atoi(buf);
            i ++;
            strncpy(&buf[0], &optarg[i],strlen(optarg));
            tmp->gaincode = atoi( buf );
            break;
        } else {
            fprintf(stdout,"Unknown flag while parsing End_channel: '%c'\n", optarg[i] );
            free(tmp);
            return NULL;
        }
    }
    return tmp;
}

/*----------------------------------------------------------------------------*/
/**
 * @desc Simple command line parser sets up testing features
 */
void process_aio_cmd_line( struct opts *options, int argc, char *argv [] )  
{
    int c;
    int error = 0;
    int option_index = 0;
    int query = 0;
    AIODisplayType display_type = BASIC;

    static struct option long_options[] = {
        {"debug"            , required_argument, 0,  'D'   },
        {"buffer_size"      , required_argument, 0,  'S'   },
        {"num_scans"        , required_argument, 0,  'N'   },
        {"num_channels"     , required_argument, 0,  'n'   },
        {"num_oversamples"  , required_argument, 0,  'O'   },
        {"gaincode"         , required_argument, 0,  'g'   },
        {"clockrate"        , required_argument, 0,  'c'   },
        {"help"             , no_argument      , 0,  'h'   },
        {"index"            , required_argument, 0,  'i'   },
        {"range"            , required_argument, 0,  'R'   },
        {"reset"            , no_argument,       0,  'r'   },
        {"outfile"          , required_argument, 0,  'f'   },
        {"verbose"          , no_argument,       0,  'V'   },
        {"block_size"       , required_argument, 0,  'B'   },
        {"timing"           , no_argument      , 0,  'T'   },
        {"query"            , no_argument      , 0,  'q'   },
        {"yaml"             , no_argument      , 0,  'Y'   },
        {"json"             , no_argument      , 0,  'J'   },
        {0                  , 0,                 0,   0    }
    };
    while (1) { 
        struct channel_range *tmp;
        c = getopt_long(argc, argv, "B:D:JN:R:S:TVYb:O:c:g:hi:m:n:o:q", long_options, &option_index);
        if( c == -1 )
            break;
        switch (c) {
        case 'R':
            if( !( tmp = get_channel_range(optarg)) ) {
                fprintf(stdout,"Incorrect channel range spec, should be '--range START-END=GAIN_CODE', not %s\n", optarg );
                exit(1);
            }

            options->ranges = (struct channel_range **)realloc( options->ranges , (++options->number_ranges)*sizeof(struct channel_range*)  );

            options->ranges[options->number_ranges-1] = tmp;
            break;
        case 'S':
            options->buffer_size = atoi( optarg );
            break;
        case 'B':
            options->block_size = atoi( optarg );
            break;
        case 'Y':
            display_type = YAML;
            break;
        case 'J':
            display_type = JSON;
            break;
        case 'q':
            query = 1;
            break;
        case 'D':
            options->debug_level = (AIO_DEBUG_LEVEL)atoi(optarg);
            AIOUSB_DEBUG_LEVEL  = options->debug_level;
            break;
        case 'f':
            options->outfile = strdup(optarg);
            break;
        case 'h':
            print_aio_usage(argc, argv, long_options );
            exit(1);
            break;
        case 'i':
            options->index = atoi(optarg);
            break;
        case 'V':
            options->verbose = 1;
            break;
        case 'n':
            options->num_channels = atoi(optarg);
            break;
        case 'O':
            options->num_oversamples = atoi(optarg);
            options->num_oversamples = ( options->num_oversamples > 255 ? 255 : options->num_oversamples );
            break;
        case 'g':
            options->gain_code = atoi(optarg);
            break;
        case 'r':
            options->reset = 1;
            break;
        case 'c':
            options->clock_rate = atoi(optarg);
            break;
        case 'N':
        case 'b':
            options->num_scans = atoi(optarg);
            if( options->num_scans <= 0 || options->num_scans > 1e8 ) {
                fprintf(stderr,"Warning: Buffer Size outside acceptable range (1,1e8), setting to 10000\n");
                options->num_scans = 10000;
            }
            break;
        default:
            fprintf(stderr, "Incorrect argument '%s'\n", optarg );
            error = 1;
            break;
        }
        if( error ) {
            print_aio_usage(argc, argv, long_options);
            exit(1);
        }
        if( options->num_channels == 0 ) {
            fprintf(stderr,"Error: You must specify num_channels > 0: %d\n", options->num_channels );
            print_aio_usage(argc, argv, long_options);
            exit(1);
        }

    }
    if ( query ) {
        AIOUSB_Init();
        AIOUSB_ShowDevices( display_type );
        exit(0);
    }

    if ( options->number_ranges == 0 ) { 
        if ( options->start_channel && options->end_channel && options->num_channels ) {
            fprintf(stdout,"Error: you can only specify -start_channel & -end_channel OR  --start_channel & --numberchannels\n");
            print_aio_usage(argc, argv, long_options );
            exit(1);
        } else if ( options->start_channel && options->num_channels ) {
            options->end_channel = options->start_channel + options->num_channels - 1;
        } else if ( options->num_channels ) {
            options->start_channel = 0;
            options->end_channel = options->num_channels - 1;
        } else {
            options->num_channels = options->end_channel - options->start_channel  + 1;
        }
    } else {
        int min = -1, max = -1;
        for( int i = 0; i < options->number_ranges ; i ++ ) {
            if ( min == -1 )
                min = options->ranges[i]->start_channel;
            if ( max == -1 ) 
                max = options->ranges[i]->end_channel;

            min = ( options->ranges[i]->start_channel < min ?  options->ranges[i]->start_channel : min );
            max = ( options->ranges[i]->end_channel > max ?  options->ranges[i]->end_channel : max );
        }
        options->start_channel = min;
        options->end_channel = max;
        options->num_channels = (max - min + 1 );
    }
}

/*----------------------------------------------------------------------------*/
void print_aio_usage(int argc, char **argv,  struct option *options)
{
    fprintf(stderr,"%s - Options\n", argv[0] );
    for ( int i =0 ; options[i].name != NULL ; i ++ ) {
        fprintf(stderr,"\t-%c | --%s ", (char)options[i].val, options[i].name);
        if( options[i].has_arg == optional_argument ) {
            fprintf(stderr, " [ ARG ]\n");
        } else if( options[i].has_arg == required_argument ) {
            fprintf(stderr, " ARG\n");
        } else {
            fprintf(stderr,"\n");
        }
    }
}

AIORET_TYPE aio_list_devices(struct opts *options, int *indices, int num_devices )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if ( num_devices <= 0 ) {
        fprintf(stderr,"No devices were found\n");
        retval = AIOUSB_ERROR_DEVICE_NOT_FOUND;
    } else {
        if ( options->index < 0 ) 
            options->index = indices[0];
        fprintf(stderr,"Matching devices found at indices: ");
        options->index = ( options->index < 0 ? indices[0] : options->index );
        int i;
        for (i = 0; i < num_devices - 1; i ++ ) { 
            fprintf(stderr, "%d",indices[i] );
            if ( num_devices > 2 )
                fprintf(stderr,", "); 
        }
        if ( num_devices > 1 ) 
            fprintf(stderr," and ");

        fprintf(stderr, "%d , Using index=%d \n",indices[i], options->index);
    }
    return retval;
}


AIORET_TYPE aio_override_aiobuf_settings( AIOContinuousBuf *buf, struct opts *options )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT( options );
    if ( options->index != AIOContinuousBufGetDeviceIndex( buf )) {
        AIOContinuousBufSetDeviceIndex( buf, options->index );
    }


    AIOUSBDevice *dev = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );

    ADCConfigBlock *config = AIOUSBDeviceGetADCConfigBlock( dev );


    if ( options->index != AIOContinuousBufGetDeviceIndex( buf ) ) {
        retval = AIOContinuousBufSetDeviceIndex( buf, options->index );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    }

    if ( options->buffer_size && options->buffer_size != AIOContinuousBufGetBufferSize(buf)) {
        int newbase = options->buffer_size / ( AIOContinuousBufGetUnitSize(buf)*AIOContinuousBufGetNumberChannels(buf)*(1+AIOContinuousBufGetOversample(buf)));
        if ( newbase <= 0 ) {
            fprintf(stderr,"Error: new buffersize is too small\n");
        } else {
            retval = AIOContinuousBufSetBaseSize( buf, newbase );
            AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
        }
    }

    if ( options->num_oversamples != AIOContinuousBufGetOversample( buf ) ) {
        retval = AIOContinuousBufSetOversample( buf, options->num_oversamples );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    }
    
    if ( options->start_channel != ADCConfigBlockGetStartChannel( config ) || 
         options->end_channel  != ADCConfigBlockGetEndChannel( config ) ) { 
        retval = AIOContinuousBufSetStartAndEndChannel( buf, options->start_channel, options->end_channel );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    }
    if ( options->num_scans != AIOContinuousBufGetNumberScans( buf ) ){
        retval = AIOContinuousBufSetNumberScans( buf, options->num_scans );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    }
    if( !options->number_ranges ) {
        retval = AIOContinuousBufSetAllGainCodeAndDiffMode( buf , options->gain_code , AIOUSB_FALSE );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    } else {
        for ( int i = 0; i < options->number_ranges ; i ++ ) {
            retval = AIOContinuousBufSetChannelRange( buf,
                                                      options->ranges[i]->start_channel,
                                                      options->ranges[i]->end_channel,
                                                      options->ranges[i]->gaincode
                                                      );
            if ( retval != AIOUSB_SUCCESS ) {
                fprintf(stderr,"Error setting ChannelRange: %d\n", retval );
                return retval;
            }
        }
    }

    if ( options->clock_rate ) { 
        retval = AIOContinuousBufSetClock( buf , options->clock_rate );
        AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );
    }

    retval = AIOContinuousBufSaveConfig(buf);
    AIO_ERROR_VALID_DATA( retval, retval == AIOUSB_SUCCESS );

    return retval;
}
