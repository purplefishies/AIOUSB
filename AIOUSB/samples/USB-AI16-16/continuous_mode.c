#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>
#include "AIOCountsConverter.h"
#include "AIOUSB_Log.h"
#include <getopt.h>

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
};


void process_cmd_line( struct opts *, int argc, char *argv[] );

int 
main(int argc, char *argv[] ) 
{
    struct opts options = {100000, 16, 10, AD_GAIN_CODE_0_5V , 4000000 , 10000 , "output.txt", 0, AIODEFAULT_LOG_LEVEL };
    AIOContinuousBuf *buf = 0;
    int keepgoing = 1;
    unsigned read_count = 0;
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    
    process_cmd_line( &options, argc, argv );

    AIOUSB_Init();
    GetDevices();

    buf = NewAIOContinuousBufForVolts( 0, options.num_scans , options.num_channels , options.num_oversamples );


    if( !buf ) {
        fprintf(stderr,"Can't allocate memory for temporary buffer \n");
        exit(1);
    }
    if( options.reset ) {
        AIOContinuousBufResetDevice( buf );
        exit(0);
    }
    FILE *fp = fopen(options.outfile,"w");
    if( !fp ) {
        fprintf(stderr,"Unable to open '%s' for writing\n", options.outfile );
        exit(1);
    }
  
    /**
     * 1. Each buf should have a device index associated with it, so 
     */
    AIOContinuousBufSetDeviceIndex( buf, 0 );

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
    AIOContinuousBufSetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    AIOContinuousBufSetOverSample( buf, options.num_oversamples );

    if ( retval < AIOUSB_SUCCESS ) {
        printf("Error setting up configuration\n");
        exit(1);
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

    int scans_remaining;
    int scans_read = 0;
    int tobufsize =  options.num_channels*(options.num_oversamples+1)*options.num_scans*sizeof(double);

    double *tobuf = (double*)malloc( tobufsize );

    while ( buf->status == RUNNING || read_count < options.num_scans ) {

        if ( (scans_remaining = AIOContinuousBufCountScansAvailable(buf) ) ) { 

            if ( scans_remaining ) { 

                scans_read = AIOContinuousBufReadIntegerScanCounts( buf, (uint16_t*)tobuf, tobufsize, AIOContinuousBufNumberChannels(buf)*AIOContinuousBufCountScansAvailable(buf) );

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
    /* int digit_optind = 0; */
    int error = 0;
    /* int this_option_optind = optind ? optind : 1; */
    int option_index = 0;
    
    static struct option long_options[] = {
        {"debug"            , required_argument, 0,  'D' },
        {"num_scans"        , required_argument, 0,  'b' },
        {"num_channels"     , required_argument, 0,  'n' },
        {"num_oversamples"  , required_argument, 0,  'O' },
        {"gaincode"         , required_argument, 0,  'g' },
        {"clockrate"        , required_argument, 0,  'c' },
        {"help"             , no_argument      , 0,  'h' },
        {"maxcount"         , required_argument, 0,  'm' },
        {"reset"            , no_argument,       0,  'r' },
        {0                  , 0,                 0,   0  }
    };
    while (1) { 
        c = getopt_long(argc, argv, "D:b:O:n:g:c:m:h", long_options, &option_index);
        if( c == -1 )
          break;
        switch (c) {
        case 'D':
            options->debug_level = (AIO_DEBUG_LEVEL)atoi(optarg);
            AIOUSB_DEBUG_LEVEL  = options->debug_level;
            break;
        case 'h':
          print_usage(argc, argv, long_options );
          exit(1);
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
}


