#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <AIODataTypes.h>
#include "AIOCountsConverter.h"
#include "AIOUSB_Log.h"
#include <getopt.h>

struct channel_range {
  int start_channel;
  int end_channel;
  int gaincode;
};

struct opts {
    unsigned num_scans;
    unsigned num_channels;
    unsigned num_oversamples;
    int gain_code;
    unsigned max_count;
    int clock_rate;
    char *outfile;
    int reset;
    int debug_level;
    int number_ranges;
    int verbose;
    int start_channel;
    int end_channel;
    int index;
    int block_size;
    struct channel_range **ranges;
};

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



int 
main(int argc, char *argv[] ) 
{
    struct opts options = {100000, 16, 0, AD_GAIN_CODE_0_5V , 4000000 , 10000 , "output.txt", 0, AIODEFAULT_LOG_LEVEL, 0, 0, 0,15, -1, -1, NULL };
    AIOContinuousBuf *buf = 0;
    unsigned read_count = 0;
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int *indices;
    int num_devices;

    process_cmd_line( &options, argc, argv );

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

    
    if ( num_devices <= 0 ) {
        fprintf(stderr,"No devices were found\n");
        exit(1);
    } else {
        fprintf(stderr,"Matching devices found at indices: ");
        options.index = ( options.index < 0 ? indices[0] : options.index );
        int i;
        for (i = 0; i < num_devices - 1; i ++ ) { 
            fprintf(stderr, "%d",indices[i] );
            if ( num_devices > 2 )
                fprintf(stderr,", "); 
        }
        fprintf(stderr, " and %d: Using index=%d \n",indices[i], options.index);
    }

    buf = NewAIOContinuousBufForVolts( options.index, options.num_scans , options.num_channels , options.num_oversamples );

    /**
     * 1. Each buf should have a device index associated with it, so 
     */
    AIOContinuousBufSetDeviceIndex( buf, options.index );


    if( !buf ) {
        fprintf(stderr,"Can't allocate memory for temporary buffer \n");
        exit(1);
    }
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
     * 
     * @brief Alternative setup for the AIOContinuousBuf oversamples, gain code
     *        and trigger modes
     * 
     * @code{.c}
     * ADConfigBlock configBlock;                                                                                
     * AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE );               
     * AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_0_5V, AIOUSB_FALSE );                        
     * AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); // 0x05
     * AIOUSB_SetScanRange( &configBlock, 0, 15 );                                                               
     * ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) );                                                     
     * result = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size ); 
     * @endcode
     */

    /**< New simpler interface */
    AIOContinuousBufInitConfiguration( buf );


    AIOContinuousBufSetOverSample( buf, options.num_oversamples );
    AIOContinuousBufSetStartAndEndChannel( buf, options.start_channel, options.end_channel );

    if( !options.number_ranges ) { 
        AIOContinuousBufSetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    } else {
        for ( int i = 0; i < options.number_ranges ; i ++ ) {
            retval = AIOContinuousBufSetChannelRange( buf, 
                                                      options.ranges[i]->start_channel, 
                                                      options.ranges[i]->end_channel,
                                                      options.ranges[i]->gaincode
                                                      );
            if ( retval != AIOUSB_SUCCESS ) {
                fprintf(stderr,"Error setting ChannelRange: %d\n", retval );
                exit(1);
            }
        }
    }
    AIOContinuousBufSaveConfig(buf);
#ifdef TEST_GET_CONFIG
    ADConfigBlock tmp;
    tmp.size = 20;
    ADC_GetConfig( 0, tmp.registers, &tmp.size );
#endif

    if ( retval < AIOUSB_SUCCESS ) {
        printf("Error setting up configuration\n");
        exit(1);
    }
  
    /**
     * 3. Setup the sampling clock rate, in this case 
     *    10_000_000 / 1000
     */ 
    if( options.block_size < 0 ) { 
        options.block_size = 1024*64;
    }

    if ( options.clock_rate < 1000 ) { 
        AIOContinuousBufSetStreamingBlockSize( buf, 512 );
    } else  {
        AIOContinuousBufSetStreamingBlockSize( buf, options.block_size );
    }

    AIOContinuousBufSetClock( buf, options.clock_rate );
    /**
     * 4. Start the Callback that fills up the 
     *    AIOContinuousBuf. This fires up an thread that 
     *    performs the acquistion, while you go about 
     *    doing other things.
     */ 
    AIOContinuousBufCallbackStart( buf );

    int scans_remaining;
    int scans_read = 0;
    int tobufsize =  options.num_channels*(options.num_oversamples+1)*options.num_scans*sizeof(double);

    double *tobuf = (double*)malloc( tobufsize );

    while ( buf->status == RUNNING || read_count < options.num_scans ) {

        if ( (scans_remaining = AIOContinuousBufCountScansAvailable(buf) ) > 0 ) { 

            if ( scans_remaining > 0 ) { 

                scans_remaining = MIN( scans_remaining, options.num_scans - read_count );

                if ( options.verbose )
                    fprintf(stdout,"Waiting : total=%u, readpos=%d, writepos=%d\n", read_count, 
                            AIOContinuousBufGetReadPosition(buf), AIOContinuousBufGetWritePosition(buf));

                scans_read = AIOContinuousBufReadIntegerScanCounts( buf, (uint16_t*)tobuf, tobufsize, AIOContinuousBufNumberChannels(buf)*scans_remaining );

                read_count += scans_read;

                for( int scan_count = 0; scan_count < scans_read ; scan_count ++ ) { 

                    for( int ch = 0 ; ch < AIOContinuousBufNumberChannels(buf); ch ++ ) {
                        fprintf(fp,"%lf,",tobuf[scan_count*AIOContinuousBufNumberChannels(buf)+ch] );
                        if( (ch+1) % AIOContinuousBufNumberChannels(buf) == 0 ) {
                            fprintf(fp,"\n");
                        }
                    }
                }
            }
        } else {
        }

    }

    fclose(fp);
    fprintf(stderr,"Test completed...exiting\n");
    retval = ( retval >= 0 ? 0 : - retval );
    return(retval);
}

/*----------------------------------------------------------------------------*/
void print_usage(int argc, char **argv,  struct option *options)
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

/*----------------------------------------------------------------------------*/
/**
 * @desc Simple command line parser sets up testing features
 */
void process_cmd_line( struct opts *options, int argc, char *argv [] ) {
    int c;
    int error = 0;
    int option_index = 0;
    
    static struct option long_options[] = {
        {"debug"            , required_argument, 0,  'D'   },
        {"num_scans"        , required_argument, 0,  'b'   },
        {"num_channels"     , required_argument, 0,  'n'   },
        {"num_oversamples"  , required_argument, 0,  'O'   },
        {"gaincode"         , required_argument, 0,  'g'   },
        {"clockrate"        , required_argument, 0,  'c'   },
        {"help"             , no_argument      , 0,  'h'   },
        {"index"            , required_argument, 0,  'i'   },
        {"maxcount"         , required_argument, 0,  'm'   },
        {"range"            , required_argument, 0,  'R'   },
        {"reset"            , no_argument,       0,  'r'   },
        {"verbose"          , no_argument,       0,  'V'   },
        {"block_size"       , required_argument, 0, 'B' },
        {0                  , 0,                 0,   0    }
    };
    while (1) { 
        struct channel_range *tmp;
        c = getopt_long(argc, argv, "B:D:b:O:n:g:c:m:hR:Vi:", long_options, &option_index);
        if( c == -1 )
            break;
        switch (c) {
        case 'R':
            if( !( tmp = get_channel_range(optarg)) ) {
                fprintf(stdout,"Incorrect channel range spec, should be '--range START-END=GAIN_CODE', not %s\n", optarg );
                exit(0);
            }

            options->ranges = (struct channel_range **)realloc( options->ranges , (++options->number_ranges)*sizeof(struct channel_range*)  );

            options->ranges[options->number_ranges-1] = tmp;
            break;
        case 'B':
            options->block_size = atoi( optarg );
            break;
        case 'D':
            options->debug_level = (AIO_DEBUG_LEVEL)atoi(optarg);
            AIOUSB_DEBUG_LEVEL  = options->debug_level;
            break;
        case 'h':
            print_usage(argc, argv, long_options );
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
        case 'm':
            options->max_count = atoi(optarg);
            break;
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
            print_usage(argc, argv, long_options);
            exit(1);
        }
        if( options->num_channels == 0 ) {
            fprintf(stderr,"Error: You must specify num_channels > 0: %d\n", options->num_channels );
            print_usage(argc, argv, long_options);
            exit(1);
        }

    }

    if ( options->number_ranges == 0 ) { 
        if ( options->start_channel && options->end_channel && options->num_channels ) {
            fprintf(stdout,"Error: you can only specify -start_channel & -end_channel OR  --start_channel & --numberchannels\n");
            print_usage(argc, argv, long_options );
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


/** 
 * @desc Parses arguments of the form   START_CHANNEL-END_CHANNEL=GAIN_CODE
 * 
 * @param optarg 
 * 
 * @return 
 */
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
