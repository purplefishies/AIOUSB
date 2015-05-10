#ifndef _AIOUSB_ADC_H
#define _AIOUSB_ADC_H

#include "AIOTypes.h"
#include "ADCConfigBlock.h"
#include "USBDevice.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


#ifndef SWIG
PUBLIC_EXTERN AIORESULT ADC_GetScanV( unsigned long DeviceIndex, double *pBuf );
PUBLIC_EXTERN AIORESULT ADC_RangeAll( unsigned long DeviceIndex, unsigned char *pGainCodes, unsigned long bSingleEnded );

PUBLIC_EXTERN AIORESULT ADC_GetChannelV( unsigned long DeviceIndex, unsigned long ChannelIndex, double *pBuf );


#endif
    

#ifndef SWIG
PUBLIC_EXTERN AIORESULT ADC_SetAllGainCodeAndDiffMode( unsigned long DeviceIndex, unsigned gain, AIOUSB_BOOL differentialMode ) ACCES_DEPRECATED("Please use ADCConfigBlockSetAllGainCodeAndDiffMode instead");

#endif


PUBLIC_EXTERN AIORESULT ADC_GetScan( unsigned long DeviceIndex, unsigned short *pBuf );
 
PUBLIC_EXTERN AIORESULT ADC_GetConfig( unsigned long DeviceIndex, unsigned char *pConfigBuf, unsigned long *ConfigBufSize );

PUBLIC_EXTERN AIORESULT ADC_SetConfig(unsigned long DeviceIndex, unsigned char *pConfigBuf, unsigned long *ConfigBufSize );
 

PUBLIC_EXTERN AIORESULT ADC_Range1( unsigned long DeviceIndex,     unsigned long ADChannel,     unsigned char GainCode,     unsigned long bSingleEnded );

PUBLIC_EXTERN AIORESULT ADC_ADMode(     unsigned long DeviceIndex,     unsigned char TriggerMode,     unsigned char CalMode );

PUBLIC_EXTERN AIORESULT ADC_SetScanLimits(     unsigned long DeviceIndex,     unsigned long StartChannel,     unsigned long EndChannel );

PUBLIC_EXTERN AIORESULT ADC_GetMaxClockRate( unsigned long ProductID,                                              unsigned int num_channels,                                              unsigned int num_oversamples                                              );

PUBLIC_EXTERN AIORESULT ADC_ClockRateForADCProduct( unsigned long ProductID );

PUBLIC_EXTERN AIORESULT ADC_SetCal(     unsigned long DeviceIndex,     const char *CalFileName );

PUBLIC_EXTERN AIORESULT ADC_QueryCal(     unsigned long DeviceIndex );

PUBLIC_EXTERN AIOUSB_BOOL ADC_CanCalibrate( unsigned long ProductID );

PUBLIC_EXTERN AIORESULT ADC_Initialize(     unsigned long DeviceIndex,     unsigned char *pConfigBuf,     unsigned long *ConfigBufSize,     const char *CalFileName );

PUBLIC_EXTERN AIORESULT ADC_BulkAcquire(     unsigned long DeviceIndex,     unsigned long BufSize,     void *pBuf );

PUBLIC_EXTERN AIORESULT ADC_BulkPoll(     unsigned long DeviceIndex,     unsigned long *BytesLeft     );



/* FastScan Functions */
PUBLIC_EXTERN AIORESULT ADC_InitFastITScanV( unsigned long DeviceIndex );

PUBLIC_EXTERN AIORESULT ADC_CreateFastITConfig(     unsigned long DeviceIndex,     int size      );

PUBLIC_EXTERN AIORESULT ADC_ResetFastITScanV(     unsigned long DeviceIndex     );

PUBLIC_EXTERN AIORESULT ADC_SetFastITScanVChannels(     unsigned long DeviceIndex,     unsigned long NewChannels      ); 

PUBLIC_EXTERN AIORESULT ADC_GetFastITScanV(     unsigned long DeviceIndex,     double *pData     );

PUBLIC_EXTERN AIORESULT ADC_GetITScanV(     unsigned long DeviceIndex,     double *pBuf     );


PUBLIC_EXTERN unsigned ADC_GetOversample( unsigned long DeviceIndex );
PUBLIC_EXTERN AIORESULT ADC_SetOversample( unsigned long DeviceIndex, unsigned char Oversample );

PUBLIC_EXTERN AIORESULT WriteConfigBlock(unsigned long DeviceIndex);
PUBLIC_EXTERN AIORESULT ReadConfigBlock(unsigned long DeviceIndex,AIOUSB_BOOL forceRead  );

PUBLIC_EXTERN void AIOUSB_SetAllGainCodeAndDiffMode( ADConfigBlock *config, unsigned gainCode, AIOUSB_BOOL differentialMode );
PUBLIC_EXTERN unsigned AIOUSB_GetGainCode( const ADConfigBlock *config, unsigned channel );
PUBLIC_EXTERN void AIOUSB_SetGainCode( ADConfigBlock *config, unsigned channel, unsigned gainCode );
PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_IsDifferentialMode( const ADConfigBlock *config, unsigned channel );
PUBLIC_EXTERN void AIOUSB_SetDifferentialMode( ADConfigBlock *config, unsigned channel, AIOUSB_BOOL differentialMode );
PUBLIC_EXTERN unsigned AIOUSB_GetCalMode( const ADConfigBlock *config );
PUBLIC_EXTERN void AIOUSB_SetCalMode( ADConfigBlock *config, unsigned calMode );

 PUBLIC_EXTERN AIORESULT AIOUSB_ADC_ExternalCal( unsigned long DeviceIndex, const double points[], int numPoints, unsigned short returnCalTable[], const char *saveFileName );

PUBLIC_EXTERN unsigned AIOUSB_GetTriggerMode( const ADConfigBlock *config );
PUBLIC_EXTERN void AIOUSB_SetTriggerMode( ADConfigBlock *config, unsigned triggerMode );
PUBLIC_EXTERN unsigned AIOUSB_GetStartChannel( const ADConfigBlock *config );
PUBLIC_EXTERN unsigned AIOUSB_GetEndChannel( const ADConfigBlock *config );
PUBLIC_EXTERN AIORET_TYPE AIOUSB_SetScanRange( ADConfigBlock *config, unsigned startChannel, unsigned endChannel );



PUBLIC_EXTERN unsigned long AIOUSB_SetStreamingBlockSize( unsigned long DeviceIndex, unsigned long BlockSize ); 

PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_IsDiscardFirstSample( unsigned long DeviceIndex );    
PUBLIC_EXTERN unsigned long AIOUSB_SetDiscardFirstSample(unsigned long DeviceIndex,AIOUSB_BOOL discard );


PUBLIC_EXTERN AIOBuf *NewBuffer( unsigned int bufsize );
PUBLIC_EXTERN void DeleteBuffer( AIOBuf *buf );
PUBLIC_EXTERN AIOBuf *CreateSmartBuffer( unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_ADC_InternalCal( unsigned long DeviceIndex, AIOUSB_BOOL autoCal, unsigned short returnCalTable[], const char *saveFileName ); 

PUBLIC_EXTERN AIORET_TYPE  BulkPoll(unsigned long DeviceIndex, AIOBuf * );

PUBLIC_EXTERN unsigned char *ADC_GetADConfigBlock_Registers( ADConfigBlock *config );

PUBLIC_EXTERN void AIOUSB_SetRegister( ADConfigBlock *cb, unsigned int Register, unsigned char value );

PUBLIC_EXTERN unsigned char AIOUSB_GetRegister( ADConfigBlock *cb, unsigned int Register );




#ifdef __aiousb_cplusplus
}
#endif

#endif
