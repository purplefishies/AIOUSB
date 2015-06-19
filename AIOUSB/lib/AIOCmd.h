#ifndef _AIOCMD_H
#define _AIOCMD_H

#include "AIOTypes.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif

typedef struct aio_cmd {
    int stop_scan;
    int stop_scan_arg;
    int channel;
    unsigned num_scans;
    unsigned num_channels;
    unsigned num_samples;
} AIOCmd;

AIOCmd *NewAIOCmdFromJSON( const char *str );
AIOCmd *NewAIOCmd();
AIORET_TYPE DeleteAIOCmd( AIOCmd *cmd );


#ifdef __aiousb_cplusplus
}
#endif

#endif
