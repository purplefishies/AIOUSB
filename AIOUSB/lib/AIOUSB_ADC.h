#ifndef _AIOUSB_ADC_H
#define _AIOUSB_ADC_H

#include "AIOTypes.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


extern unsigned ADC_GetOversample( unsigned long DeviceIndex );
extern unsigned long ADC_SetOversample( unsigned long DeviceIndex, unsigned char Oversample );

extern unsigned long WriteConfigBlock(unsigned long DeviceIndex);
extern unsigned long ReadConfigBlock(unsigned long DeviceIndex,AIOUSB_BOOL forceRead  );

#ifdef __aiousb_cplusplus
}
#endif

#endif
