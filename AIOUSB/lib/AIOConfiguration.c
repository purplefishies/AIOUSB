#include "AIOConfiguration.h"
#include "AIOContinuousBuffer.h"
#include "ADCConfigBlock.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
AIOConfiguration *NewAIOConfiguration( ) 
{
    AIOConfiguration *tmp = (AIOConfiguration *)calloc(sizeof(AIOConfiguration),1);
    if ( !tmp )
        goto out_NewAIOConfiguration;
    out_NewAIOConfiguration:
    return tmp;
}

void DeleteAIOConfiguration(AIOConfiguration *config )
{
    free(config);
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOConfigurationInitialize( AIOConfiguration *config )
{
    AIO_ASSERT_RET( AIOUSB_ERROR_INVALID_AIOCONFIGURATION, config );

    config->timeout               = 1000;
    config->number_scans          = 0;
    config->discard_first_sample  = 0;
    config->device_index          = -1;
    config->calibration           = AD_SET_CAL_AUTO;
    config->output_file           = NULL;
    config->file_handle           = stdout;
    config->file_name             = NULL;
    memset(&config->setting,0,sizeof(AIOSetting));
    config->configure             = NULL;
    config->run                   = NULL;
    config->type                  = NO_CONFIG;
    config->debug                 = AIOUSB_FALSE;
    return AIOUSB_SUCCESS;
}

AIORET_TYPE AIOArgumentInitialize( AIOArgument *arg )
{
    AIO_ASSERT_RET( AIOUSB_ERROR_INVALID_AIOARGUMENT, arg );

    arg->threaded = AIOUSB_FALSE;
    arg->debug = AIOUSB_FALSE;
    arg->size = &arg->actual_size;
    AIOConfigurationInitialize( &arg->config );

    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOConfigurationSetTimeout( AIOConfiguration *config, unsigned timeout )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ASSERT_RET( AIOUSB_ERROR_INVALID_CONFIG, config );
   
    switch ( config->type ) {
    case AIOCONTINUOUSBUF_CONFIG:
        retval = AIOContinuousBufSetTimeout( &config->setting.aiobuf, timeout );
        break;
    case ADCCONIGBLOCK_CONFIG:
    default:
        retval = ADCConfigBlockSetTimeout( &config->setting.adc , timeout );
        break;
    }
    config->timeout = timeout;
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOConfigurationSetDebug( AIOConfiguration *config, AIOUSB_BOOL debug )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ASSERT_RET( AIOUSB_ERROR_INVALID_CONFIG, config );

    switch ( config->type ) {
    case AIOCONTINUOUSBUF_CONFIG:
        AIOContinuousBufSetDebug( &config->setting.aiobuf, debug );
        break;
    case ADCCONIGBLOCK_CONFIG:
        ADCConfigBlockSetDebug( &config->setting.adc , debug );
    default:
        break;
    }
    config->debug = debug;
    return AIOUSB_SUCCESS;
}

#ifdef __cplusplus
}
#endif

