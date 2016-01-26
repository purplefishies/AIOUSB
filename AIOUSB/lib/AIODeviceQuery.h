/**
 * @file   AIODeviceQuery.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * 
 * @brief  
 * 
 * 
 */

#ifndef _AIO_CONTINUOUS_BUFFER_H
#define _AIO_CONTINUOUS_BUFFER_H

#include "AIOTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libusb.h>
#include <string.h>
#include <math.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef struct AIODeviceQuery {
    unsigned long productID;
    unsigned long nameSize;
    char *name;
    unsigned long numDIOBytes;
    unsigned long numCounters;
} AIODeviceQuery;

/* BEGIN AIOUSB_API */

PUBLIC_EXTERN AIODeviceQuery *NewAIODeviceQuery( unsigned long DeviceIndex );
PUBLIC_EXTERN AIORET_TYPE DeleteAIODeviceQuery( AIODeviceQuery *devq );
PUBLIC_EXTERN AIORET_TYPE AIODeviceQueryGetProductID( AIODeviceQuery *devq );
PUBLIC_EXTERN AIORET_TYPE AIODeviceQueryNameSize( AIODeviceQuery *devq );
PUBLIC_EXTERN char * AIODeviceQueryGetName( AIODeviceQuery *devq );
PUBLIC_EXTERN AIORET_TYPE AIODeviceQueryGetNumDIOBytes( AIODeviceQuery *devq );
PUBLIC_EXTERN AIORET_TYPE AIODeviceQueryGetNumCounters( AIODeviceQuery *devq );

/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif

#endif
