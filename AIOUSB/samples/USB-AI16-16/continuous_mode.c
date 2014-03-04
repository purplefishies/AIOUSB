#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>
#include <getopt.h>
struct opts {
  int buffer_size;
  int number_channels;
  int gain_code;
  unsigned max_count;
  int clock_rate;
  char *outfile;
  int reset;
};


void process_cmd_line( struct opts *, int argc, char *argv[] );

int 
main(int argc, char *argv[] ) 
{
    struct opts options = {100000, 16, AD_GAIN_CODE_0_5V , 4000000 , 10000 , "output.txt", 0 };
    /* int number_channels = 16; */
    AIOContinuousBuf *buf = 0;
    int keepgoing = 1;
    unsigned read_count = 0;
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*options.buffer_size);
    if( !tmp ) {
        fprintf(stderr,"Can't allocate memory for temporary buffer \n");
        _exit(1);
    }
    process_cmd_line( &options, argc, argv );

    AIOUSB_Init();
    GetDevices();
    buf = (AIOContinuousBuf *)NewAIOContinuousBuf( 0, options.buffer_size , options.number_channels );
    if( !buf ) {
        fprintf(stderr,"Can't allocate memory for temporary buffer \n");
        _exit(1);
    }
    if( options.reset ) {
        AIOContinuousBuf_ResetDevice( buf );
        _exit(0);
    }
    FILE *fp = fopen(options.outfile,"w");
    if( !fp ) {
        fprintf(stderr,"Unable to open '%s' for writing\n", options.outfile );
        _exit(1);
    }
  

    /**
     * 1. Each buf should have a device index associated with it, so 
     */
    AIOContinuousBuf_SetDeviceIndex( buf, 0 );

    /**
     * 2. Setup the Config object for Acquisition, either the more complicated 
     *    part in comments (BELOW) or using a simple interface.
     */
    /* ADConfigBlock configBlock; */
    /* AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE ); */
    /* AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_0_5V, AIOUSB_FALSE ); */
    /* AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /\* 0x05 *\/ */
    /* AIOUSB_SetScanRange( &configBlock, 0, 15 ); */
    /* ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) ); */
    /* result = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size ); */
    /* or ... */
    /* New simpler interface */
    AIOContinuousBuf_InitConfiguration( buf );
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    AIOContinuousBuf_SetOverSample( buf, 0 );

    if ( retval < AIOUSB_SUCCESS ) {
        printf("Error setting up configuration\n");
        _exit(1);
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

    while ( keepgoing ) {
        /**
         * You can optionally read values
         * retval = AIOContinuousBufRead( buf, tmp, tmpsize ); 
         */
        fprintf(stderr,"Waiting : total=%u, readpos=%d, writepos=%d\n", read_count, AIOContinuousBufGetReadPosition(buf), AIOContinuousBufGetWritePosition(buf));
        if( read_count > options.max_count || buf->status != RUNNING ) {
            keepgoing = 0;
            AIOContinuousBufEnd(buf);
            retval = buf->exitcode;
        }
        sleep(1);
        /**
         * in this example we read bytes in blocks of our core number_channels parameter. 
         * the channel order
         */
        /* while ( keepgoing && (AIOContinuousBufAvailableReadSize(buf) > 16*AIOContinuousBuf_NumberChannels(buf) ) ) { */
        while ( keepgoing && (AIOContinuousBufAvailableReadSize(buf) > AIOContinuousBuf_NumberChannels(buf) ) ) {
            /* retval = AIOContinuousBufRead( buf, tmp, 16*AIOContinuousBuf_NumberChannels(buf) ); */
            retval = AIOContinuousBufRead( buf, tmp, AIOContinuousBuf_NumberChannels(buf) );
            if ( retval < AIOUSB_SUCCESS ) {
                fprintf(stderr,"ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf) );
                keepgoing = 0;
            } else {
                read_count += (unsigned)retval;
                /* for ( int i = 0; i < 16*AIOContinuousBuf_NumberChannels(buf) ; i ++ ) {  */
                for ( int i = 0; i < AIOContinuousBuf_NumberChannels(buf) ; i ++ ) { 
                    fprintf(fp, "%f,", tmp[i] );
                    if(  (i+1) % 16 == 0 ) 
                      fprintf(fp,"\n");
                }
            }
        }
    }
    fclose(fp);
    fprintf(stderr,"Test completed...exiting\n");
    retval = ( retval >= 0 ? 0 : - retval );
    return(retval);
}

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

/** 
 * @desc Simple command line parser sets up testing features
 */
void process_cmd_line( struct opts *options, int argc, char *argv [] ) {
    int c;
    /* int digit_optind = 0; */
    int error = 0;
    /* int this_option_optind = optind ? optind : 1; */
    int option_index = 0;
    
    static struct option long_options[] = {
      {"buffersize",   required_argument, 0,  'b' },
      {"numchannels",  required_argument, 0,  'n' },
      {"gaincode",     required_argument, 0,  'g' },
      {"clockrate",    required_argument, 0,  'c' },
      {"help",         no_argument      , 0,  'h' },
      {"maxcount",     required_argument, 0,  'm' },
      {"reset",        no_argument,       0,  'r' },
      {0,         0,                 0,  0 }
    };
    while (1) { 
        c = getopt_long(argc, argv, "b:n:g:c:m:h", long_options, &option_index);
        if( c == -1 )
          break;
        switch (c) {
        case 'h':
          print_usage(argc, argv, long_options );
          _exit(1);
          break;
        case 'n':
          options->number_channels = atoi(optarg);
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
          /* printf("option b\n"); */
          options->buffer_size = atoi(optarg);
          if( options->buffer_size <= 0 || options->buffer_size > 1e8 ) {
              fprintf(stderr,"Warning: Buffer Size outside acceptable range (1,1e8), setting to 10000\n");
              options->buffer_size = 10000;
          }
          break;
        default:
          fprintf(stderr, "Incorrect argument '%s'\n", optarg );
          error = 1;
          break;
        }
        if( error ) {
            print_usage(argc, argv, long_options);
            _exit(1);
        }
    }
}


