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
#include <unistd.h>
#include <math.h>
#include <AIODataTypes.h>
#include "AIOUSB_Log.h"
#include <getopt.h>
#include <ctype.h>
#include <time.h>

#define  _FILE_OFFSET_BITS 64  

struct channel_range {
  int startchannel;
  int endchannel;
  int gaincode;
};


struct opts {
    int num_scans;
    int number_channels;
    int gain_code;
    unsigned max_count;
    int clock_rate;
    char *outfile;
    int reset;
    int startchannel;
    int endchannel;
    int number_ranges;
    int slowacquire;
    int with_timing;
    int verbose;
    int debug_level;
    int index;
    int block_size;
    struct channel_range **ranges;
};


void process_cmd_line( struct opts *, int argc, char *argv[] );
void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize);
void process_with_looping_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize);

struct channel_range *get_channel_range( char *optarg );

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
    struct opts options = {100000, 0, AD_GAIN_CODE_0_5V , 4000000 , 10000 , (char*)"output.txt", 0, 0, 15 , 0, 0, 0, 0, (AIO_DEBUG_LEVEL)7, -1, -1, NULL };
    AIOContinuousBuf *buf = 0;
    struct timespec foo , bar;

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int *indices;
    int num_devices;
    process_cmd_line( &options, argc, argv );

    int tobufsize = (options.num_scans+1)*options.number_channels*20;
    uint16_t *tobuf = (uint16_t *)malloc(sizeof(uint16_t)*tobufsize);

    unsigned short *tmp = (unsigned short *)malloc(sizeof(unsigned short)*(options.num_scans+1)*options.number_channels);
    if( !tmp ) {
      fprintf(stderr,"Can't allocate memory for temporary buffer \n");
      _exit(1);
    }

    AIOUSB_Init();
    AIOUSB_ListDevices();

    AIOUSB_FindDevices( &indices, &num_devices, find_ai_board );
   
    if ( num_devices <= 0 ) {
        fprintf(stderr,"No devices were found\n");
        exit(1);
    } else {
        if ( options.index < 0 ) 
            options.index = indices[0];
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

    buf = (AIOContinuousBuf *)NewAIOContinuousBufForCounts( options.index, options.num_scans, options.number_channels );
    if( !buf ) {
      fprintf(stderr,"Can't allocate memory for temporary buffer \n");
      _exit(1);
    }

    AIOContinuousBufSetDeviceIndex( buf, options.index ); /* Assign the first matching device for this sample */

    if( options.reset ) {
        fprintf(stderr,"Resetting device at index %d\n",buf->DeviceIndex );
        AIOContinuousBufResetDevice( buf );
        _exit(0);
    }
    FILE *fp = fopen(options.outfile,"w");
    if( !fp ) {
      fprintf(stderr,"Unable to open '%s' for writing\n", options.outfile );
      _exit(1);
    }

    /**
     * 2. Setup the Config object for Acquisition, either the more complicated 
     *    part in comments (BELOW) or using a simple interface.
     */
    /* New simpler interface */
    AIOContinuousBufInitConfiguration( buf );

    if ( options.slowacquire ) {
        unsigned char bufData[64];
        unsigned long bytesWritten = 0;
        GenericVendorWrite( 0, 0xDF, 0x0000, 0x001E, bufData, &bytesWritten  );
    }

    AIOContinuousBufSetOverSample( buf, 0 );
    AIOContinuousBufSetStartAndEndChannel( buf, options.startchannel, options.endchannel );
    if( !options.number_ranges ) { 
        AIOContinuousBufSetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    } else {
        for ( int i = 0; i < options.number_ranges ; i ++ ) {
            AIOContinuousBufSetChannelRange( buf, 
                                             options.ranges[i]->startchannel, 
                                             options.ranges[i]->endchannel,
                                             options.ranges[i]->gaincode
                                             );
        }
    }
    AIOContinuousBufSaveConfig(buf);
    
    if ( retval < AIOUSB_SUCCESS ) {
        printf("Error setting up configuration\n");
        _exit(1);
    }
  
    if( options.block_size < 0 ) { 
        options.block_size = 1024*64;
    }

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
     * in this example we read bytes in blocks of our core number_channels parameter. 
     * the channel order
     */
    if ( options.with_timing ) 
        clock_gettime( CLOCK_MONOTONIC_RAW, &bar );


#if 1
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
            /* sleep(1); */
        }
        
    }
#endif


    fclose(fp);
    fprintf(stdout,"Test completed...exiting\n");

    return 0;
}

void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize)
{

}


void print_usage(int argc, char **argv,  struct option *options)
{
    fprintf(stdout,"%s - Options\n", argv[0] );
    for ( int i =0 ; options[i].name != NULL ; i ++ ) {
      fprintf(stdout,"\t-%c | --%s ", (char)options[i].val, options[i].name);
      if( options[i].has_arg == optional_argument ) {
        fprintf(stdout, " [ ARG ]\n");
      } else if( options[i].has_arg == required_argument ) {
        fprintf(stdout, " ARG\n");
      } else {
        fprintf(stdout,"\n");
      }
    }
}

/** 
 * @desc Simple command line parser sets up testing features
 */
void process_cmd_line( struct opts *options, int argc, char *argv [] ) 
{
    int c;
    /* int digit_optind = 0; */
    int error = 0;
    /* int this_option_optind = optind ? optind : 1; */
    int option_index = 0;
    
    static struct option long_options[] = {
         {"debug",        required_argument, 0,  'D' },
         {"buffersize",   required_argument, 0,  'b' },
         {"blocksize",    required_argument, 0,  'B' },
         {"numchannels",  required_argument, 0,  'n' },
         {"gaincode",     required_argument, 0,  'g' },
         {"clockrate",    required_argument, 0,  'c' },
         {"help",         no_argument      , 0,  'h' },
         {"maxcount",     required_argument, 0,  'm' },
         {"reset",        no_argument,       0,  'r' },
         {"startchannel", required_argument, 0,  's' },
         {"endchannel"  , required_argument, 0,  'e' },
         {"slowacquire" , no_argument      , 0,  'S' }, 
         {"range",        required_argument, 0,  'R' },
         {"timing",       no_argument      , 0,  'T' },
         {"verbose",      no_argument      , 0,  'V' },
         {"index"  ,      required_argument, 0,  'i' },

         {0,         0,                 0,  0 }
        };
    while (1) { 
      struct channel_range *tmp;
        c = getopt_long(argc, argv, "B:D:b:n:g:c:m:hTVi:", long_options, &option_index);
        if( c == -1 )
          break;
        switch (c) {
        case 'R':
          if( !( tmp = get_channel_range(optarg)) ) {
            fprintf(stdout,"Incorrect channel range spec, should be '--range START-END=GAIN_CODE', not %s\n", optarg );
            _exit(0);
          }

          options->ranges = (struct channel_range **)realloc( options->ranges , (++options->number_ranges)*sizeof(struct channel_range*)  );

          options->ranges[options->number_ranges-1] = tmp;
          break;
        case 'h':
            print_usage(argc, argv, long_options );
            _exit(1);
            break;
        case 'B':
            options->block_size = atoi(optarg);
            break;
        case 'n':
            options->number_channels = atoi(optarg);
            break;
        case 's':
            options->startchannel = atoi(optarg);
            break;
        case 'i':
            options->index = atoi(optarg);
            break;
        case 'V':
            options->verbose = 1;
            break;
        case 'T':
            options->with_timing = 1;
            break;
        case 'S':
            options->slowacquire = 1;
            break;
        case 'D':
            options->debug_level = (AIO_DEBUG_LEVEL)atoi(optarg);
            AIOUSB_DEBUG_LEVEL  = options->debug_level;
            break;
        case 'e':
          options->endchannel = atoi(optarg);
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
          options->num_scans = atoi(optarg);
          if( options->num_scans <= 0 || options->num_scans > 1e8 ) {
              fprintf(stdout,"Warning: Buffer Size outside acceptable range (1,1e8), setting to 10000\n");
              options->num_scans = 10000;
          }
          break;
        default:
          fprintf(stdout, "Incorrect argument '%s'\n", optarg );
          error = 1;
          break;
        }
        if( error ) {
            print_usage(argc, argv, long_options);
            _exit(1);
        }
    }
    if( options->number_ranges == 0 ) { 
      if( options->startchannel && options->endchannel && options->number_channels ) {
        fprintf(stdout,"Error: you can only specify -startchannel & -endchannel OR  --startchannel & --numberchannels\n");
        print_usage(argc, argv, long_options );
        _exit(1);
      } else if ( options->startchannel && options->number_channels ) {
        options->endchannel = options->startchannel + options->number_channels - 1;
      } else if ( options->number_channels ) {
        options->startchannel = 0;
        options->endchannel = options->number_channels - 1;
      } else {
        options->number_channels = options->endchannel - options->startchannel  + 1;
      }
    } else {
        int min = -1, max = -1;
        for( int i = 0; i < options->number_ranges ; i ++ ) {
            if ( min == -1 )
                min = options->ranges[i]->startchannel;
            if ( max == -1 ) 
                max = options->ranges[i]->endchannel;

            min = ( options->ranges[i]->startchannel < min ?  options->ranges[i]->startchannel : min );
            max = ( options->ranges[i]->endchannel > max ?  options->ranges[i]->endchannel : max );
        }
        options->startchannel = min;
        options->endchannel = max;
        options->number_channels = (max - min + 1 );
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
  if( !tmp ) {
    fprintf(stdout,"Unable to create a new channel range\n");
    return NULL;
  }
  MODE mode = BEGIN;
  for ( i = 0; i < strlen(optarg); i ++ ) {
    if( mode == BEGIN && isdigit(optarg[i] ) ) {
      pos = i;
      mode = SCHANNEL;
    } else if( mode == SCHANNEL && isdigit(optarg[i])  ) {
      
    } else if( mode == SCHANNEL && optarg[i] == '-' ) {
      mode = ECHANNEL;
      strncpy(&buf[0], &optarg[pos], i - pos );
      buf[i-pos] = 0;
      tmp->startchannel = atoi(buf);
      i ++ ;
      pos = i;
    } else if( mode == SCHANNEL ) {
      fprintf(stdout,"Unknown flag while parsing Start_channel: '%c'\n", optarg[i] );
      free(tmp);
      return NULL;
    } else if ( mode == ECHANNEL && isdigit(optarg[i] ) ) {
      
    } else if ( mode == ECHANNEL && optarg[i] == '=' ) {
      mode = GAIN;
      strncpy(&buf[0], &optarg[pos], i - pos );
      buf[i-pos] = 0;
      tmp->endchannel = atoi(buf);
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

