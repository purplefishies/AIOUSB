#include "AIOCommandLine.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

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


#ifdef __cplusplus
}
#endif
