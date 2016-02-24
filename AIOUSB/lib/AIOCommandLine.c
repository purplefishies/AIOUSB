#include "AIOCommandLine.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

AIOCommandLineOptions AIO_DEFAULT_CMDLINE_OPTIONS = {
                           -1,                            /* int64_t num_scans; */
                           10000,                         /* int64_t default_num_scans; */
                           -1,                            /* unsigned num_channels; */      
                           16,                            /* unsigned default_num_channels; */      
                           -1,                            /* unsigned num_oversamples; */   
                           0,                             /* unsigned default_num_oversamples; */   
                           AD_GAIN_CODE_0_5V ,            /* int gain_code; */              
                           -1,                            /* int clock_rate; */             
                           10000,                         /* int default_clock_rate;  */
                           (char*)"output.txt",                  /* char *outfile; */              
                           0,                             /* int reset; */                  
                           AIODEFAULT_LOG_LEVEL,          /* int debug_level; */            
                           0,                             /* int number_ranges; */          
                           0,                             /* int verbose; */                
                           -1,                            /* int start_channel; */          
                           0,                             /* int default_start_channel; */          
                           -1,                            /* int end_channel; */            
                           15,                            /* int default_end_channel; */            
                           -1,                            /* int index; */                  
                           -1,                            /* int block_size; */             
                           0,                             /* int with_timing; */            
                           0,                             /* int slow_acquire; */           
                           2048,                          /* int buffer_size; */            
                           100,                           /* int rate_limit; */             
                           0,                             /* int physical; */               
                           0,                             /* int counts; */                 
                           0,                             /* int calibration; */            
                           2,                             /* int repeat */
                           NULL,
                           (char *)"{\"DeviceIndex\":0,\"base_size\":2048,\"block_size\":65536,\"debug\":\"false\",\"hz\":10000,\"num_channels\":16,\"num_oversamples\":0,\"num_scans\":1024,\"testing\":\"false\",\"timeout\":1000,\"type\":2,\"unit_size\":2}",
                           (char*)"{\"channels\":[{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"}],\"calibration\":\"Normal\",\"trigger\":{\"reference\":\"sw\",\"edge\":\"rising-edge\",\"refchannel\":\"single-channel\"},\"start_channel\":\"0\",\"end_channel\":\"15\",\"oversample\":\"0\",\"timeout\":\"1000\",\"clock_rate\":\"1000\"}",
                           NULL
};



AIOArgument *NewAIOArgument()
{
    AIOArgument *tmp = (AIOArgument *)calloc(sizeof(AIOArgument),1);
    if ( !tmp ) 
        return tmp;
    return tmp;
}

/*----------------------------------------------------------------------------*/
AIOArgument *aiousb_getoptions( int argc, char **argv)
{
    AIOArgument *retargs = NULL;
    AIOUSB_BOOL found_one = AIOUSB_FALSE;
    int num_retargs = 0;
    /**
     * command --device index=0,adcconfig=$(cat foo.json),timeout=1000,debug=1
     *         --device index=1,adcconfig=$(cat foo.json),timeout=1000,channel=2=+-2V,sample=getscan,file=foo.csv
     *         --device index=2,aiocontinuousbuf=$(cat cb.json),timeout=2000,setcal=auto,count=3000,sample=getscanv,file=bar.csv
     *         --device index=2,aiocontinuousbuf=$(cat cb.json),timeout=2000,setcal=normal,count=3000,sample=getscanv,file=bar.csv
     *         --device index=3,timeout=1000,setcal=normal,numscans=1000,sample=getscanv,file=output.csv
     *
     * The optional count=3000,action=getscanv will fireup a separate thread and run this and generate 
     */
    /* AIOUSB_Init(); */
    int option_index = 0;
    static struct option long_options[] = {
        {"foo"              , no_argument       , 0,  0 },
        {"aiolistdevices"   , optional_argument , 0,  0 },
        {"adcconfig"        , required_argument , 0,  0 },
        {"aiodevice_index"  , required_argument , 0,  0 },
        {"aiousbdevice"     , required_argument , 0,  0 },
        {"aiodevice"        , required_argument , 0,  0 },
        {"aiotimeout"       , required_argument , 0,  0 },
        {"aiodebug"         , required_argument , 0,  0 },
        {"aiosetcal"        , required_argument , 0,  0 },
        {"aionumscans"      , required_argument , 0,  0 },
        {"aiochannelspec"   , required_argument , 0,  0 },
        {"aiofunction"      , required_argument , 0,  0 },
        {"aiooutfile"       , required_argument , 0,  0 },
        {0                  , 0                 , 0,  0 }
    };

    /* Can I make this parameter skip through ? */
    AIOConfiguration *config = NULL;

    while ( 1 ) { 
        int c = getopt_long(argc, argv, "", long_options, &option_index);
        /* printf("%d\n", c ); */
        if (c == -1)
            break;
        switch (c) {
        case 0:
    
            if ( strcmp( long_options[option_index].name,"aiolistdevices" ) == 0 ) {
                AIOUSB_Init();
                AIODisplayType type;
                if ( optarg ) 
                    if ( strcasecmp(optarg, "terse") == 0 ) {
                        type = TERSE;
                    } else if ( strcasecmp( optarg, "json" ) == 0 ) {
                        type = JSON;
                    } else if ( strcasecmp( optarg, "yaml" ) == 0 ) { 
                        type = YAML;
                    } else {
                        type = BASIC;
                    }
                else
                    type = BASIC;

                AIOUSB_ShowDevices( type );
                exit(1);

            } else if ( strcmp( long_options[option_index].name,"aiodevice_index" ) == 0 ||
                        strcmp( long_options[option_index].name,"aiodevice" ) == 0 ) {
                found_one = AIOUSB_TRUE;
                if ( !config ) {
                    config = NewAIOConfiguration();
                    if ( !config ) {
                        fprintf(stderr,"Can't create new AIOconfiguration object..exiting\n");
                        exit(1);
                    }
                    AIOConfigurationInitialize( config );
                } else {
                    num_retargs ++;
                    retargs = (AIOArgument *)realloc(retargs, num_retargs*sizeof(AIOArgument));

                    AIOArgumentInitialize( &retargs[num_retargs-1] );
                    
                    memcpy(&retargs[num_retargs-1].config, config , sizeof(AIOConfiguration));
                    retargs[num_retargs-1].size = retargs[0].size;
                    retargs[0].size = &retargs[0].actual_size;
                    retargs[0].actual_size = num_retargs;
                    free(config);
                    config = NewAIOConfiguration();
                    AIOConfigurationInitialize( config );
                }

                config->device_index = atoi(optarg);
            } else if ( strcmp( long_options[option_index].name,"aiousbdevice" ) == 0 ) {
                AIOUSBDevice *usb = (AIOUSBDevice *)NewAIOUSBDeviceFromJSON( optarg );
                if ( !usb ) {
                    fprintf(stderr,"Error parsing JSON object from '%10s...'\n", optarg );
                    exit(1);
                }


            } else if ( strcmp( long_options[option_index].name,"adcconfig" ) == 0 ) {
                if ( !config ) {
                    fprintf(stderr,"Error: you must first specify --aiodevice_index  ## followed by --adcconfig. Exiting...\n");
                    exit(1);
                }
                ADCConfigBlock *adc = (ADCConfigBlock *)NewADCConfigBlockFromJSON( optarg );
                if (!adc ) {
                    
                } 
                config->type = ADCCONIGBLOCK_CONFIG;
                if (!adc ) { 
                    fprintf(stderr,"Error reading JSON for '%s'\n", optarg );
                    exit(1);
                }
                ADCConfigBlockCopy( &config->setting.adc , adc );
                free(adc);
            } else if ( strcmp( long_options[option_index].name,"aiotimeout" ) == 0 ) {
                AIOConfigurationSetTimeout( config, atoi(optarg) );
            } else if ( strcmp( long_options[option_index].name,"aiodebug" ) == 0 ) { 
                AIOConfigurationSetDebug( config, atoi(optarg) == 0 ? 0 : 1 );
            } else if ( strcmp( long_options[option_index].name,"aiosetcal" ) == 0 ) { 
                if ( strcasecmp( optarg, "none" ) == 0 ) {
                    config->calibration = AD_SET_CAL_NORMAL;
                } else if ( strcasecmp( optarg, "auto" ) == 0 ) {
                    config->calibration = AD_SET_CAL_AUTO;
                } else if ( strncasecmp( optarg, "file=",5 ) == 0 ) {
                    config->calibration = AD_SET_CAL_MANUAL;
                    config->calibration_file = ( strchr(optarg,'=') + 1);
                }
            } else if ( strcmp( long_options[option_index].name,"aionumscans" ) == 0 ) { 
                config->number_scans = atoi( optarg );
            } else if ( strcmp( long_options[option_index].name,"aiochannelspec" ) == 0 ) { 

            } else if ( strcmp( long_options[option_index].name,"aiofunction" ) == 0 ) { 
                if( strcasecmp( optarg, "getscanv" ) == 0 ) {
                    config->scan_type = AD_SCAN_GETSCANV;
                } else if ( strcasecmp( optarg, "getscan" ) == 0 ) {
                    config->scan_type = AD_SCAN_GETSCAN;
                } else if ( strcasecmp( optarg, "getchannelv" ) == 0 ) {
                    config->scan_type = AD_SCAN_GETCHANNELV;
                } else if ( strcasecmp( optarg, "continuous" ) == 0 ) {
                    config->scan_type = AD_SCAN_CONTINUOUS;
                } else if ( strcasecmp( optarg, "bulkacquire" ) == 0 ) {
                    config->scan_type = AD_SCAN_BULKACQUIRE;
                } else {
                    fprintf(stderr,"Can't recongize function '%s'\n", optarg );
                    exit(2);
                }
            } else if ( strcmp(long_options[option_index].name,"aiooutfile" )) {
                config->file_name = strdup(optarg);
            }
        }
    }

    if ( found_one && (!retargs || config->device_index != retargs[num_retargs-1].config.device_index ) ) {
        num_retargs ++;
        retargs = (AIOArgument *)realloc(retargs, num_retargs*sizeof(AIOArgument));
        retargs[0].size = &(retargs[0].actual_size);
        AIOArgumentInitialize( &retargs[num_retargs-1] );
                    
        memcpy(&retargs[num_retargs-1].config, config , sizeof(AIOConfiguration));
        retargs[num_retargs-1].size = retargs[0].size;
        retargs[0].actual_size = num_retargs;
        free(config);
    }

    return retargs;
}


void AIOProcessCmdline( AIOCommandLineOptions *options, int argc, char **argv)
{
        int c;
    int error = 0;
    int option_index = 0;
    int query = 0;
    int dump_adcconfig = 0;
    AIODisplayType display_type = BASIC;

    static struct option long_options[] = {
        {"debug"            , required_argument, 0,  'D'   },
        {"dump"             , no_argument      , 0,   DUMP },
        {"dumpadcconfig"    , no_argument      , 0,   DUMP },
        {"buffer_size"      , required_argument, 0,  'S'   },
        {"num_scans"        , required_argument, 0,  'N'   },
        {"num_channels"     , required_argument, 0,  'n'   },
        {"num_oversamples"  , required_argument, 0,  'O'   },
        {"gaincode"         , required_argument, 0,  'g'   },
        {"clockrate"        , required_argument, 0,  'c'   },
        {"calibration"      , required_argument, 0 , 'C'   },
        {"help"             , no_argument      , 0,  'h'   },
        {"index"            , required_argument, 0,  'i'   },
        {"range"            , required_argument, 0,  'R'   },
        {"repeat"           , required_argument, 0,  REPEAT},
        {"reset"            , no_argument,       0,  'r'   },
        {"outfile"          , required_argument, 0,  'f'   },
        {"verbose"          , no_argument,       0,  'V'   },
        {"block_size"       , required_argument, 0,  'B'   },
        {"timing"           , no_argument      , 0,  'T'   },
        {"query"            , no_argument      , 0,  'q'   },
        {"ratelimit"        , required_argument, 0,  'L'   },
        {"physical"         , no_argument      , 0,  'p'   },
        {"counts"           , no_argument      , 0,   CNTS },         
        {"yaml"             , no_argument      , 0,  'Y'   },
        {"json"             , no_argument      , 0,  'J'   },
        {"jsonconfig"       , required_argument, 0,  JCONF },
        {0                  , 0,                 0,   0    }
    };
    while (1) { 
        AIOChannelRange *tmp;
        c = getopt_long(argc, argv, "B:C:D:JL:N:R:S:TVYb:O:c:g:hi:m:n:o:q", long_options, &option_index);
        if( c == -1 )
            break;
        switch (c) {
        case 'R':
            if( !( tmp = AIOGetChannelRange(optarg)) ) {
                fprintf(stdout,"Incorrect channel range spec, should be '--range START-END=GAIN_CODE', not %s\n", optarg );
                exit(1);
            }

            options->ranges = (AIOChannelRange **)realloc( options->ranges , (++options->number_ranges)*sizeof(AIOChannelRange *)  );

            options->ranges[options->number_ranges-1] = tmp;
            break;
        case 'S':
            options->buffer_size = atoi( optarg );
            break;
        case 'T':
            options->with_timing = 1;
            break;
        case 'B':
            options->block_size = atoi( optarg );
            break;
        case 'C':
            options->calibration = atoi( optarg );
            if ( !VALID_ENUM( ADCalMode, options->calibration ) ) {
                fprintf(stderr,"Error: calibration %d is not valid\n", options->calibration );
                fprintf(stderr,"Acceptable values are %d,%d,%d and %d\n",
                        AD_CAL_MODE_NORMAL,
                        AD_CAL_MODE_GROUND,
                        AD_CAL_MODE_REFERENCE,
                        AD_CAL_MODE_BIP_GROUND
                        );
                fprintf(stderr, "Using default AD_CAL_MODE_NORMAL\n");
                options->calibration = AD_CAL_MODE_NORMAL;
            }
            break;
        case 'Y':
            display_type = YAML;
            break;
        case 'J':
            display_type = JSON;
            break;
        case 'p':
                options->physical = 1;
                break;
        case 'L':
            options->rate_limit = atoi(optarg);
            break;
        case 'q':
            query = 1;
            break;
        case 'D':
            options->debug_level = (AIO_DEBUG_LEVEL)atoi(optarg);
            AIOUSB_DEBUG_LEVEL  = (AIO_DEBUG_LEVEL)options->debug_level;
            break;
        case DUMP:
            dump_adcconfig = 1;
            break;
        case CNTS:
            options->counts = 1;
            break;
        case JCONF:
            options->aiobuf_json = strdup( optarg );
            break;
        case REPEAT:
            options->repeat = atoi(optarg);
            break;
        case 'f':
            options->outfile = strdup(optarg);
            break;
        case 'h':
            AIOPrintUsage(argc, argv, long_options );
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
            options->num_scans = (int64_t)atoll(optarg);
            if( options->num_scans <= 0 ) {
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
            AIOPrintUsage(argc, argv, long_options);
            exit(1);
        }
        if( options->num_channels == 0 ) {
            fprintf(stderr,"Error: You must specify num_channels > 0: %d\n", options->num_channels );
            AIOPrintUsage(argc, argv, long_options);
            exit(1);
        }
    }

    if ( query ) {
        AIOUSB_Init();
        AIOUSB_ShowDevices( display_type );
        exit(0);
    }

    if ( dump_adcconfig ) { 
        if ( options->index == -1 ) { 
            fprintf(stderr,"Error: Can't dump adcconfiguration without specifying index ( -i INDEX_NUM ) of the device\nexiting...\n");
            exit(1);
        } else {
            AIOUSB_Init();
            /* AIOUSB_ShowDevices( display_type ); */
            ADCConfigBlock config;
            ADCConfigBlockInitializeDefault( &config );
            ADC_GetConfig( options->index, config.registers, &config.size );
            printf("%s\n",ADCConfigBlockToJSON(&config));
            exit(0);
        }
    } 

    if ( options->number_ranges == 0 ) { 
        if ( options->start_channel >= 0 && options->end_channel >=0  && options->num_channels ) {
            fprintf(stdout,"Error: you can only specify -start_channel & -end_channel OR  --start_channel & --numberchannels\n");
            AIOPrintUsage(argc, argv, long_options );
            exit(1);
        } else if ( options->start_channel >= 0 && options->num_channels >= 0 ) {
            options->end_channel = options->start_channel + options->num_channels - 1;
        } else if ( options->num_channels > 0 ) {
            options->start_channel = 0;
            options->end_channel = options->num_channels - 1;
        } else if ( options->num_channels < 0 && options->start_channel < 0 && options->end_channel < 0 ) {
            
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
void AIOPrintUsage(int argc, char **argv,  struct option *options)
{
    fprintf(stderr,"%s - Options\n", argv[0] );
    for ( int i =0 ; options[i].name != NULL ; i ++ ) {
        if ( options[i].val < 255 ) { 
            fprintf(stderr,"\t-%c | --%s ", (char)options[i].val, options[i].name);
        } else {
            fprintf(stderr,"\t     --%s ", options[i].name);
        }
        if( options[i].has_arg == optional_argument ) {
            fprintf(stderr, " [ ARG ]\n");
        } else if( options[i].has_arg == required_argument ) {
            fprintf(stderr, " ARG\n");
        } else {
            fprintf(stderr,"\n");
        }
    }
}

AIOChannelRange *AIOGetChannelRange(char *optarg )
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
    AIOChannelRange *tmp = (AIOChannelRange *)malloc( sizeof(AIOChannelRange) );
    if ( !tmp ) {
        fprintf(stdout,"Unable to create a new channel range\n");
        return NULL;
    }
    MODE mode = BEGIN;
    for ( i = 0; i < (int)strlen(optarg); i ++ ) {
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



#ifdef __cplusplus
}
#endif
