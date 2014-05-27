#ifndef _DEVICE_INFO_H
#define _DEVICE_INFO_H

#include "AIOTypes.h"
#include "AIOUSB_Core.h"
#include <stdlib.h>
#include <string.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef struct AIODeviceInfo {
    unsigned long PID;
    unsigned long NameSize;
    char *Name;
    unsigned long DIOBytes;
    unsigned long Counters;
} AIODeviceInfo;


PUBLIC_EXTERN AIODeviceInfo *NewAIODeviceInfo();
PUBLIC_EXTERN void DeleteAIODeviceInfo( AIODeviceInfo *di );
PUBLIC_EXTERN const char *AIODeviceInfoGetName( AIODeviceInfo *di );
PUBLIC_EXTERN unsigned AIODeviceInfoGetCounters( AIODeviceInfo *di ); 
PUBLIC_EXTERN unsigned AIODeviceInfoGetDIOBytes( AIODeviceInfo *di );
PUBLIC_EXTERN AIODeviceInfo *GetDeviceInfo( unsigned long DeviceIndex );


#ifdef __aiousb_cplusplus
}
#endif


#endif
