#ifndef _AIO_COMMAND_LINE_H
#define _AIO_COMMAND_LINE_H

#include "AIOTypes.h"
#include "ADCConfigBlock.h"
#include "AIOContinuousBuffer.h"
#include "AIOConfiguration.h"
#include "AIOUSB_Core.h"
#include "AIODeviceTable.h"
#include "AIOUSB_Properties.h"
#include "getopt.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

AIOArgument *NewAIOArgument();

typedef enum {
    INDEX_NUM = 0,
    ADCCONFIG_OPT,
    TIMEOUT_OPT,
    DEBUG_OPT,
    SETCAL_OPT,
    COUNT_OPT,
    SAMPLE_OPT,
    FILE_OPT,
    CHANNEL_OPT
} DeviceEnum;
 
PUBLIC_EXTERN AIOArgument *aiousb_getoptions( int argc, char **argv);

#ifdef __aiousb_cplusplus
}
#endif



#endif



