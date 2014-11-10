/**
 * @file   aiousb.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  General header files for the AIOUSB library
 *
 */

#ifndef AIOUSB_H
#define AIOUSB_H

#include <stdlib.h>
#include <assert.h>
#include "AIOTypes.h"
#include "AIOUSB_Properties.h"
#include "AIOUSB_DIO.h"
#include "AIOUSB_ADC.h"
#include "AIOUSB_CTR.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

#ifndef PUBLIC_EXTERN
#define PUBLIC_EXTERN extern
#endif

PUBLIC_EXTERN unsigned long QueryDeviceInfo(
                                            unsigned long DeviceIndex,
                                            unsigned long *pPID,
                                            unsigned long *pNameSize,
                                            char *pName,
                                            unsigned long *pDIOBytes,
                                            unsigned long *pCounters );

PUBLIC_EXTERN unsigned long ClearDevices( void );

PUBLIC_EXTERN unsigned long ResolveDeviceIndex( unsigned long DeviceIndex );



PUBLIC_EXTERN void ADC_InitConfigBlock( ADConfigBlock *, 
                                 void *deviceDesc, 
                                 unsigned int 
                                 );

PUBLIC_EXTERN void ADC_InitConfigBlockForTesting( ADConfigBlock *, 
                                           void *deviceDesc, 
                                           unsigned int , 
                                           AIOUSB_BOOL );


PUBLIC_EXTERN void ADC_SetTestingMode( ADConfigBlock *config, 
                                AIOUSB_BOOL testing );

AIOUSB_BOOL ADC_GetTestingMode(ADConfigBlock *config, 
                               AIOUSB_BOOL testing );


PUBLIC_EXTERN unsigned long ADC_GetChannelV(
    unsigned long DeviceIndex,
    unsigned long ChannelIndex,
    double *pBuf );

PUBLIC_EXTERN unsigned long ADC_GetScanV(
    unsigned long DeviceIndex,
    double *pBuf );

PUBLIC_EXTERN unsigned ADC_SetAllGainCodeAndDiffMode( 
    unsigned long DeviceIndex, 
    unsigned gain, 
    AIOUSB_BOOL differentialMode );

PUBLIC_EXTERN unsigned long ADC_GetScan(
    unsigned long DeviceIndex,
    unsigned short *pBuf );

PUBLIC_EXTERN unsigned long ADC_GetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize );

PUBLIC_EXTERN unsigned long ADC_SetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize );

PUBLIC_EXTERN unsigned long ADC_RangeAll(
    unsigned long DeviceIndex,
    unsigned char *pGainCodes,
    unsigned long bSingleEnded );

PUBLIC_EXTERN unsigned long ADC_Range1(
    unsigned long DeviceIndex,
    unsigned long ADChannel,
    unsigned char GainCode,
    unsigned long bSingleEnded );

PUBLIC_EXTERN unsigned long ADC_ADMode(
    unsigned long DeviceIndex,
    unsigned char TriggerMode,
    unsigned char CalMode );

PUBLIC_EXTERN unsigned long ADC_SetScanLimits(
    unsigned long DeviceIndex,
    unsigned long StartChannel,
    unsigned long EndChannel );

PUBLIC_EXTERN unsigned long ADC_SetCal(
    unsigned long DeviceIndex,
    const char *CalFileName );

PUBLIC_EXTERN unsigned long ADC_QueryCal(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long ADC_Initialize(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize,
    const char *CalFileName );

PUBLIC_EXTERN unsigned long ADC_BulkAcquire(
    unsigned long DeviceIndex,
    unsigned long BufSize,
    void *pBuf );

PUBLIC_EXTERN unsigned long ADC_BulkPoll(
    unsigned long DeviceIndex,
    unsigned long *BytesLeft
    );


PUBLIC_EXTERN AIOBuf *NewBuffer( unsigned int bufsize );

PUBLIC_EXTERN void DeleteBuffer( AIOBuf *buf );

PUBLIC_EXTERN AIOBuf *CreateSmartBuffer( unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry );



PUBLIC_EXTERN AIORET_TYPE  BulkAcquire(
    unsigned long DeviceIndex,
    AIOBuf *buf,
    int size
                                );

PUBLIC_EXTERN AIORET_TYPE  BulkPoll(
    unsigned long DeviceIndex,
    AIOBuf *
                                );



PUBLIC_EXTERN unsigned char *ADC_GetADConfigBlock_Registers(
    ADConfigBlock *config
    );



/* FastScan Functions */
PUBLIC_EXTERN unsigned long ADC_InitFastITScanV( unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long ADC_CreateFastITConfig(
    unsigned long DeviceIndex,
    int size
    );

PUBLIC_EXTERN unsigned long ADC_ResetFastITScanV(
    unsigned long DeviceIndex
    );

PUBLIC_EXTERN unsigned long ADC_SetFastITScanVChannels(
    unsigned long DeviceIndex,
    unsigned long NewChannels
    );

PUBLIC_EXTERN unsigned long ADC_GetFastITScanV(
    unsigned long DeviceIndex,
    double *pData
    );

PUBLIC_EXTERN unsigned long ADC_GetITScanV(
    unsigned long DeviceIndex,
    double *pBuf
    );

PUBLIC_EXTERN unsigned long DACDirect(
    unsigned long DeviceIndex,
    unsigned short Channel,
    unsigned short Value );

PUBLIC_EXTERN unsigned long DACMultiDirect(
    unsigned long DeviceIndex,
    unsigned short *pDACData,
    unsigned long DACDataCount );

PUBLIC_EXTERN unsigned long DACSetBoardRange(
    unsigned long DeviceIndex,
    unsigned long RangeCode );

PUBLIC_EXTERN unsigned long DACOutputOpen(
    unsigned long DeviceIndex,
    double *pClockHz );

PUBLIC_EXTERN unsigned long DACOutputClose(
    unsigned long DeviceIndex,
    unsigned long bWait );

PUBLIC_EXTERN unsigned long DACOutputCloseNoEnd(
    unsigned long DeviceIndex,
    unsigned long bWait );

PUBLIC_EXTERN unsigned long DACOutputSetCount(
    unsigned long DeviceIndex,
    unsigned long NewCount );

PUBLIC_EXTERN unsigned long DACOutputFrame(
    unsigned long DeviceIndex,
    unsigned long FramePoints,
    unsigned short *FrameData );

PUBLIC_EXTERN unsigned long DACOutputFrameRaw(
    unsigned long DeviceIndex,
    unsigned long FramePoints,
    unsigned short *FrameData );

PUBLIC_EXTERN unsigned long DACOutputStart(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long DACOutputSetInterlock(
    unsigned long DeviceIndex,
    unsigned long bInterlock );

PUBLIC_EXTERN unsigned long GetDeviceSerialNumber(
                                                  unsigned long DeviceIndex,
                                                  unsigned long *pSerialNumber 
                                                  );

PUBLIC_EXTERN unsigned long GetDeviceBySerialNumber( unsigned long *pSerialNumber );

PUBLIC_EXTERN unsigned long CustomEEPROMWrite(
    unsigned long DeviceIndex,
    unsigned long StartAddress,
    unsigned long DataSize,
    void *Data );

PUBLIC_EXTERN unsigned long CustomEEPROMRead(
    unsigned long DeviceIndex,
    unsigned long StartAddress,
    unsigned long *DataSize,
    void *Data );

PUBLIC_EXTERN long AIOUSB_GetStreamingBlockSize(
    unsigned long DeviceIndex
    );


PUBLIC_EXTERN unsigned long AIOUSB_SetStreamingBlockSize(
    unsigned long DeviceIndex,
    unsigned long BlockSize );

PUBLIC_EXTERN unsigned long AIOUSB_ClearFIFO(
    unsigned long DeviceIndex,
    FIFO_Method Method
    );



PUBLIC_EXTERN const char *AIOUSB_GetVersion(void);                /* returns AIOUSB module version number as a string */
PUBLIC_EXTERN const char *AIOUSB_GetVersionDate(void);            /* returns AIOUSB module version date as a string */
PUBLIC_EXTERN const char *AIOUSB_GetResultCodeAsString( unsigned long value );       /* gets string representation of AIOUSB_xxx result code */

PUBLIC_EXTERN unsigned long AIOUSB_Init(void);                          /* must be called before use of other functions in AIOUSB */
PUBLIC_EXTERN void AIOUSB_Exit(void);                                   /* must be called after last use of other functions in AIOUSB */
PUBLIC_EXTERN unsigned long AIOUSB_Reset(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_GetDeviceProperties(
    unsigned long DeviceIndex,
    DeviceProperties *properties );

PUBLIC_EXTERN unsigned long AIOUSB_GetDeviceByProductID(
    int minProductID,
    int maxProductID,
    int maxDevices,
    int *deviceList );

PUBLIC_EXTERN double AIOUSB_GetMiscClock(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_SetMiscClock(
    unsigned long DeviceIndex,
    double clockHz );

PUBLIC_EXTERN unsigned AIOUSB_GetCommTimeout(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_SetCommTimeout(
    unsigned long DeviceIndex,
    unsigned timeout );

PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_IsDiscardFirstSample(
    unsigned long DeviceIndex );

PUBLIC_EXTERN unsigned long AIOUSB_SetDiscardFirstSample(
    unsigned long DeviceIndex,
    AIOUSB_BOOL discard );

PUBLIC_EXTERN double AIOUSB_CountsToVolts(
    unsigned long DeviceIndex,
    unsigned channel,
    unsigned short counts );

PUBLIC_EXTERN unsigned long AIOUSB_MultipleCountsToVolts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const unsigned short counts[],
    double volts[] );

PUBLIC_EXTERN unsigned short AIOUSB_VoltsToCounts(
    unsigned long DeviceIndex,
    unsigned channel,
    double volts );

PUBLIC_EXTERN unsigned long AIOUSB_MultipleVoltsToCounts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const double volts[],
    unsigned short counts[] );

PUBLIC_EXTERN unsigned long AIOUSB_ADC_LoadCalTable(
    unsigned long DeviceIndex,
    const char *fileName );

PUBLIC_EXTERN unsigned long AIOUSB_ADC_SetCalTable(
    unsigned long DeviceIndex,
    const unsigned short calTable[] );

PUBLIC_EXTERN unsigned long AIOUSB_ADC_InternalCal(
    unsigned long DeviceIndex,
    AIOUSB_BOOL autoCal,
    unsigned short returnCalTable[],
    const char *saveFileName );

PUBLIC_EXTERN void AIOUSB_SetRegister(
    ADConfigBlock *cb,
    unsigned int Register,
    unsigned char value );

PUBLIC_EXTERN unsigned char
AIOUSB_GetRegister( ADConfigBlock *cb,
                    unsigned int Register );


PUBLIC_EXTERN unsigned long AIOUSB_ADC_ExternalCal(
    unsigned long DeviceIndex,
    const double points[],
    int numPoints,
    unsigned short returnCalTable[],
    const char *saveFileName );


/*
 * ADConfigBlock was created to act like a C++ "class" in order to facilitate configuring
 * the A/D subsystem and do so in a reliable manner; the functions below should be thought
 * of as class "methods" that operate on an "instance" of ADConfigBlock; the method named
 * AIOUSB_InitConfigBlock() is akin to a class "constructor"; below is an example of how
 * to use these functions/methods
 *
 * ADConfigBlock configBlock;
 * AIOUSB_InitConfigBlock( &configBlock, DeviceIndex, AIOUSB_TRUE );		 //call "constructor"
 *   ... set up other properties ...
 * ADC_SetConfig( DeviceIndex, configBlock.registers, &configBlock.size );	 //send configuration block to device
 */
PUBLIC_EXTERN void AIOUSB_InitConfigBlock( ADConfigBlock *config, unsigned long DeviceIndex, AIOUSB_BOOL defaults );
PUBLIC_EXTERN void AIOUSB_SetAllGainCodeAndDiffMode( ADConfigBlock *config, unsigned gainCode, AIOUSB_BOOL differentialMode );
PUBLIC_EXTERN unsigned AIOUSB_GetGainCode( const ADConfigBlock *config, unsigned channel );
PUBLIC_EXTERN void AIOUSB_SetGainCode( ADConfigBlock *config, unsigned channel, unsigned gainCode );
PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_IsDifferentialMode( const ADConfigBlock *config, unsigned channel );
PUBLIC_EXTERN void AIOUSB_SetDifferentialMode( ADConfigBlock *config, unsigned channel, AIOUSB_BOOL differentialMode );
PUBLIC_EXTERN unsigned AIOUSB_GetCalMode( const ADConfigBlock *config );
PUBLIC_EXTERN void AIOUSB_SetCalMode( ADConfigBlock *config, unsigned calMode );

PUBLIC_EXTERN unsigned long AIOUSB_ADC_ExternalCal(
                                                   unsigned long DeviceIndex,
                                                   const double points[],
                                                   int numPoints,
                                                   unsigned short returnCalTable[],
                                                   const char *saveFileName
                                                   );

PUBLIC_EXTERN unsigned AIOUSB_GetTriggerMode( const ADConfigBlock *config );
PUBLIC_EXTERN void AIOUSB_SetTriggerMode( ADConfigBlock *config, unsigned triggerMode );
PUBLIC_EXTERN unsigned AIOUSB_GetStartChannel( const ADConfigBlock *config );
PUBLIC_EXTERN unsigned AIOUSB_GetEndChannel( const ADConfigBlock *config );
PUBLIC_EXTERN AIORET_TYPE AIOUSB_SetScanRange( ADConfigBlock *config, unsigned startChannel, unsigned endChannel );


#ifdef __aiousb_cplusplus
}
#endif


#endif


/* end of file */
