/**
 * @file   burst_test.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 */

#include <stdio.h>
#include <aiousb.h>
#include <stdio.h>
#include <string.h>
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
  int startchannel;
  int endchannel;
};


void process_cmd_line( struct opts *, int argc, char *argv[] );
/* void process_with_single_buf( AIOContinuousBuf *buf , FILE *fp); */
/* void process_with_single_buf( struct options *opts, AIOContinuousBuf *buf , FILE *fp); */
/* void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf); */
void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize);


int 
main(int argc, char *argv[] ) 
{
    struct opts options = {100000, 0, AD_GAIN_CODE_0_5V , 4000000 , 10000 , "output.txt", 0, 0, 0, 15 };
    AIOContinuousBuf *buf = 0;
    int keepgoing = 1;
    unsigned read_count = 0;
    unsigned short tobuf[32768] = {0};
    unsigned tobufsize = 32768;

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    unsigned short *tmp = (unsigned short *)malloc(sizeof(unsigned short)*(options.buffer_size+1)*options.number_channels);
    if( !tmp ) {
      fprintf(stderr,"Can't allocate memory for temporary buffer \n");
      _exit(1);
    }
    process_cmd_line( &options, argc, argv );

    AIOUSB_Init();
    GetDevices();
    buf = (AIOContinuousBuf *)NewAIOContinuousBufForCounts( 0, (options.buffer_size+1), options.number_channels );
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
    /* New simpler interface */
    AIOContinuousBuf_InitConfiguration( buf );
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf , options.gain_code , AIOUSB_FALSE );
    AIOContinuousBuf_SetOverSample( buf, 0 );
    AIOContinuousBuf_SetStartAndEndChannel( buf, options.startchannel, options.endchannel );
    
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

    /**
     * in this example we read bytes in blocks of our core number_channels parameter. 
     * the channel order
     */
    while ( keepgoing ) {
      /**
       * You can optionally read values
       * retval = AIOContinuousBufRead( buf, tmp, tmpsize );
       */
      fprintf(stderr,"Waiting : total=%u, readpos=%d, writepos=%d\n", read_count, AIOContinuousBufGetReadPosition(buf), AIOContinuousBufGetWritePosition(buf));
      sleep(1);
      if( read_count > options.max_count || buf->status != RUNNING ) {
        printf("Exit reason: read_count=%d, max=%d, RUNNING=%d,but_status=%d\n",read_count, options.max_count, (int)RUNNING,(int)buf->status );
        keepgoing = 0;
        printf("Exiting from main\n");
        AIOContinuousBufEnd(buf);
        retval = buf->exitcode;
      }

    }
    fprintf(stderr,"Predrain Waiting : total=%u, readpos=%d, writepos=%d\n", read_count, AIOContinuousBufGetReadPosition(buf), AIOContinuousBufGetWritePosition(buf));
    int pos = 0;
    unsigned short *here;
   
    /** 
     * Use the recommended API for reading data out of 
     * the counts buffer
     */
    printf("Numscans=%d\n",AIOContinuousBufCountScansAvailable(buf)  );
    /* _exit(0); */
    while (  AIOContinuousBufCountScansAvailable(buf) > 0 ) {
      printf("Read=%d,Write=%d,size=%d,Avail=%d\n",
             get_read_pos(buf),
             get_write_pos(buf),
             buffer_size(buf),
             AIOContinuousBufCountScansAvailable(buf));
      retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf ,tobufsize, 32768 );
      if ( retval < AIOUSB_SUCCESS ) {
          printf("not ok - ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf));
          keepgoing = 0;
      } else {
      /* unsigned short *tmpbuf = (unsigned short *)&tobuf[0]; */
        read_count += retval;
          for( int i = 0, ch = 0 ; i < retval; i ++, ch = ((ch+1)% AIOContinuousBuf_NumberChannels(buf)) ) {
            fprintf(fp,"%u,",tobuf[i] );
            if( (i+1) % AIOContinuousBuf_NumberChannels(buf) == 0 ) {
              fprintf(fp,"\n");
            }
          }
      }
    }



    fclose(fp);
    fprintf(stderr,"Test completed...exiting\n");
    /* retval = ( retval >= 0 ? 0 : - retval ); */
    /* return(retval); */
    return 0;
}

void process_with_single_buf( struct opts *opts, AIOContinuousBuf *buf , FILE *fp, unsigned short *tobuf, unsigned short tobufsize)
{
  /* int keepgoing = 1; */
  /* int read_count = 0; */
  /* AIORET_TYPE retval= 0; */
  /* while ( keepgoing ) { */
  /*   /\** */
  /*    * You can optionally read values */
  /*    * retval = AIOContinuousBufRead( buf, tmp, tmpsize ); */
  /*    *\/ */
  /*   fprintf(stderr,"Waiting : total=%u, readpos=%d, writepos=%d\n",  */
  /*           read_count, */
  /*           AIOContinuousBufGetReadPosition(buf),  */
  /*           AIOContinuousBufGetWritePosition(buf) */
  /*           ); */
  /*   sleep(1); */
  /*   if( read_count > opts->max_count || buf->status != RUNNING ) { */
  /*     printf("Exit reason: read_count=%d, max=%d, RUNNING=%d,but_status=%d\n", */
  /*            read_count,  */
  /*            opts->max_count,  */
  /*            (int)RUNNING, */
  /*            (int)buf->status ); */
  /*     keepgoing = 0; */
  /*     printf("Exiting from main\n"); */
  /*     AIOContinuousBufEnd(buf); */
  /*     retval = buf->exitcode; */
  /*   } */
  /*   sleep(5); */
  /* } */
}


/* unsigned count = 0; */
/* unsigned delta = AIOContinuousBuf_NumberChannels(buf)*512; */
/* unsigned char *data = (unsigned char *)malloc( delta ); */
/* memset(&data[0],(unsigned char)1,delta); */
/* for( count = 0; count < 2*buf->size; ) { */
/*     /\* unsigned char *cur = (unsigned char *)&(buf->buffer[count]); *\/ */
/*     unsigned char *cur = ((unsigned char *)(&buf->buffer[0])+count); */
/*     unsigned  tmpcount =  ( (2*buf->size - count) < delta ? (2*buf->size-count) : delta ); */
/*     count += tmpcount; */
/*     /\* unsigned char data[delta]; *\/ */
/*     printf("%u\n",count); */
/*     memcpy(cur, data, tmpcount  ); */
/* } */
/* free(data); */
/* printf("After\n"); */
/* _exit(0); */
/* if( tmpbuf[i] != usdata[i] ) { */
/*   printf("not ok - got %u,  not %u\n", tmpbuf[i],  usdata[i] ); */
/*   failed ++; */
/* } */

/* while ( keepgoing && AIOContinuousBufCountsAvailable(buf) ) { */
/*         retval = AIOContinuousBufReadAvailableCounts( buf, (unsigned short *)tmp ); */
/*         if( retval < AIOUSB_SUCCESS ) { */
/*             fprintf(stderr,"ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf) ); */
/*             keepgoing = 0; */
/*         } else { */
/*             unsigned short *tmpbuf = (unsigned short *)&tmp[0]; */
/*             for( int i = 0 ; i < retval ; i ++ ) { */
/*                 fprintf(fp,"%u,",tmpbuf[i] ); */
/*                 if( (i+1) % AIOContinuousBuf_NumberChannels(buf) == 0 ) { */
/*                     fprintf(fp,"\n"); */
/*                 } */
/*             } */
/*         } */
/*     } */
/* } */
/* while (  AIOContinuousBufCountsAvailable(buf)  ) { */
/*     /\* printf("Reading\n"); *\/ */
/*     retval = AIOContinuousBufReadAvailableCounts( buf, (unsigned short *)tmp ); */
/*     if ( retval < AIOUSB_SUCCESS ) { */
/*         fprintf(stderr,"ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf) ); */
/*     } else { */
/*         unsigned short *tmpbuf = (unsigned short *)&tmp[0]; */
/*         for( int i = 0 ; i < retval; i ++ ) { */
/*             fprintf(fp,"%u,",tmpbuf[i] ); */
/*             if( (i+1) % AIOContinuousBuf_NumberChannels(buf) == 0 ) { */
/*                 fprintf(fp,"\n"); */
/*             } */
/*         } */
/*     } */
/* } */






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
void process_cmd_line( struct opts *options, int argc, char *argv [] ) 
{
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
      /* {"single",        no_argument,       0,  's' }, */
      {"startchannel", required_argument, 0,  's' },
      {"endchannel",  required_argument, 0, 'e' },
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
        case 's':
          options->startchannel = atoi(optarg);
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
    if( options->startchannel && options->endchannel && options->number_channels ) {
      fprintf(stderr,"Error: you can only specify -startchannel & -endchannel OR  --startchannel & --numberchannels\n");
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
}

