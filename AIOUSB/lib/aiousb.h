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
#include "AIODeviceInfo.h"
#include "AIODeviceTable.h"
#include "AIOUSBDevice.h"
#include "ADCConfigBlock.h"
#include "AIOUSB_Properties.h"
#include "AIOUSB_DIO.h"
#include "AIOUSB_ADC.h"
#include "AIOUSB_CTR.h"
#include "AIOUSB_DAC.h"
#include "AIOUSB_CustomEEPROM.h"
#include "USBDevice.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

#ifndef PUBLIC_EXTERN
#define PUBLIC_EXTERN extern
#endif



/* PUBLIC_EXTERN AIOBuf *NewBuffer( unsigned int bufsize ); */
/* PUBLIC_EXTERN void DeleteBuffer( AIOBuf *buf ); */
/* PUBLIC_EXTERN AIOBuf *CreateSmartBuffer( unsigned long DeviceIndex ); */
/* PUBLIC_EXTERN unsigned long AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry ); */



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



#ifdef __aiousb_cplusplus
}
#endif


#endif


/* end of file */
