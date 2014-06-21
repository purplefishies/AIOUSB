#ifndef _AIOUSB_ADC_H
#define _AIOUSB_ADC_H

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


extern unsigned ADC_GetOversample( unsigned long DeviceIndex );
extern unsigned long ADC_SetOversample( unsigned long DeviceIndex, unsigned char Oversample );


#ifdef __aiousb_cplusplus
}
#endif

#endif
