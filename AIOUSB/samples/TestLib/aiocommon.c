#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include "aiocommon.h"
#include "AIOUSB_Log.h"


struct opts AIO_OPTIONS = {100000, 16, 0, AD_GAIN_CODE_0_5V , 4000000 , 10000 , "output.txt", 0, AIODEFAULT_LOG_LEVEL, 0, 0, 0,15, -1, -1, 0, 0, NULL };

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
        {"outfile"          , required_argument, 0,  'o'   },
        {"verbose"          , no_argument,       0,  'V'   },
        {"block_size"       , required_argument, 0,  'B'   },
        {"timing"           , no_argument      , 0,  'T'   },
        {0                  , 0,                 0,   0    }
    };
    while (1) { 
        struct channel_range *tmp;
        c = getopt_long(argc, argv, "B:D:b:O:n:g:c:o:m:hR:TVi:", long_options, &option_index);
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
        case 'o':
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
            print_aio_usage(argc, argv, long_options);
            exit(1);
        }
        if( options->num_channels == 0 ) {
            fprintf(stderr,"Error: You must specify num_channels > 0: %d\n", options->num_channels );
            print_aio_usage(argc, argv, long_options);
            exit(1);
        }

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
