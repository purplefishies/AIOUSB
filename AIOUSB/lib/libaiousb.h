#ifndef LIBAIOUSB_H
#define LIBAIOUSB_H

#include <mex.h>  /* only needed because of mexFunction below and mexPrintf */
#include "AIOUSB_Core.h"
#include "AIOChannelMask.h"
#include "AIOContinuousBuffer.h"
#include "AIODataTypes.h"
#include "AIOTypes.h"

#include "DIOBuf.h"

PUBLIC_EXTERN DIOBuf *NewDIOBuf ( unsigned size );
PUBLIC_EXTERN void DeleteDIOBuf ( DIOBuf  *buf );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromChar( const char *ary , int size_array );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromBinStr( const char *ary );
PUBLIC_EXTERN DIOBuf *DIOBufReplaceString( DIOBuf *buf, char *ary, int size_array );
PUBLIC_EXTERN char *DIOBufToHex( DIOBuf *buf );
PUBLIC_EXTERN char *DIOBufToBinary( DIOBuf *buf );
PUBLIC_EXTERN DIOBuf  *DIOBufResize( DIOBuf  *buf , unsigned size );
PUBLIC_EXTERN unsigned DIOBufSize( DIOBuf  *buf );
PUBLIC_EXTERN char *DIOBufToString( DIOBuf  *buf );
PUBLIC_EXTERN int DIOBufSetIndex( DIOBuf *buf, unsigned index, unsigned value );
PUBLIC_EXTERN int DIOBufGetIndex( DIOBuf *buf, unsigned index );


#include "AIOUSB_DIO.h"
PUBLIC_EXTERN unsigned long DIO_Configure(
                                          unsigned long DeviceIndex,
                                          unsigned char bTristate,
                                          void *pOutMask,
                                          void *pData
                                          );
PUBLIC_EXTERN unsigned long DIO_ConfigureEx( 
                                            unsigned long DeviceIndex, 
                                            void *pOutMask, 
                                            void *pData, 
                                            void *pTristateMask 
                                             );
PUBLIC_EXTERN unsigned long DIO_ConfigurationQuery(
                                                   unsigned long DeviceIndex,
                                                   void *pOutMask,
                                                   void *pTristateMask
                                                   );
PUBLIC_EXTERN unsigned long DIO_WriteAll(
                                         unsigned long DeviceIndex,
                                         void *pData
                                         /* DIOBuf *data */
                                         );
PUBLIC_EXTERN unsigned long DIO_Write8(
                                       unsigned long DeviceIndex,
                                       unsigned long ByteIndex,
                                       unsigned char Data
                                       );
PUBLIC_EXTERN unsigned long DIO_Write1(
                                       unsigned long DeviceIndex,
                                       unsigned long BitIndex,
                                       unsigned char bData
                                       );
PUBLIC_EXTERN unsigned long DIO_ReadAll(
                                        unsigned long DeviceIndex,
                                        DIOBuf *buf
                                        );
PUBLIC_EXTERN unsigned long DIO_ReadAllToCharStr(
                                                 unsigned long DeviceIndex,
                                                 char *buf,
                                                 unsigned size
                                                 );

PUBLIC_EXTERN unsigned long DIO_Read8(
                                      unsigned long DeviceIndex,
                                      unsigned long ByteIndex,
                                      int *pdat
                                      );

PUBLIC_EXTERN unsigned long DIO_Read1(
                                      unsigned long DeviceIndex,
                                      unsigned long BitIndex,
                                      int *bit
                                      );

PUBLIC_EXTERN unsigned long DIO_StreamOpen(
                                           unsigned long DeviceIndex,
                                           unsigned long bIsRead
                                           );
PUBLIC_EXTERN unsigned long DIO_StreamClose(
                                            unsigned long DeviceIndex
                                            );
PUBLIC_EXTERN unsigned long DIO_StreamSetClocks(
                                                unsigned long DeviceIndex,
                                                double *ReadClockHz,
                                                double *WriteClockHz
                                                );
PUBLIC_EXTERN unsigned long DIO_StreamFrame(
                                            unsigned long DeviceIndex,
                                            unsigned long FramePoints,
                                            unsigned short *pFrameData,
                                            unsigned long *BytesTransferred
                                            );



#include "AIOUSB_USB.h"
#include "AIOUSB_WDG.h"
#include "aiousb.h"


/* AIOUSB_Core */
PUBLIC_EXTERN unsigned long AIOUSB_Validate_Lock(unsigned long *DeviceIndex);
PUBLIC_EXTERN unsigned long AIOUSB_Validate(unsigned long *DeviceIndex) ;
PUBLIC_EXTERN unsigned long GetDevices(void);
PUBLIC_EXTERN unsigned long QueryDeviceInfo( unsigned long DeviceIndex, unsigned long *pPID, unsigned long *pNameSize, char *pName, unsigned long *pDIOBytes, unsigned long *pCounters ) ;
PUBLIC_EXTERN unsigned long ClearDevices(void) ;
PUBLIC_EXTERN long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex) ;
PUBLIC_EXTERN unsigned long AIOUSB_SetStreamingBlockSize( unsigned long DeviceIndex, unsigned long BlockSize ) ;
PUBLIC_EXTERN unsigned long AIOUSB_ClearFIFO( unsigned long DeviceIndex, FIFO_Method Method ) ;
PUBLIC_EXTERN const char *AIOUSB_GetVersion() ;
PUBLIC_EXTERN const char *AIOUSB_GetVersionDate() ;
PUBLIC_EXTERN unsigned long AIOUSB_Init(void) ;
PUBLIC_EXTERN void AIOUSB_Exit() ;
PUBLIC_EXTERN unsigned long AIOUSB_Reset( unsigned long DeviceIndex ) ;
PUBLIC_EXTERN double AIOUSB_GetMiscClock( unsigned long DeviceIndex ) ;
PUBLIC_EXTERN unsigned long AIOUSB_SetMiscClock( unsigned long DeviceIndex, double clockHz ) ;
PUBLIC_EXTERN unsigned AIOUSB_GetCommTimeout(unsigned long DeviceIndex ) ;
PUBLIC_EXTERN unsigned long AIOUSB_SetCommTimeout(unsigned long DeviceIndex, unsigned timeout ) ;
PUBLIC_EXTERN unsigned long AIOUSB_Validate_Device(unsigned long DeviceIndex) ;

/* AIOUSB_ADC_ExternalCal */

PUBLIC_EXTERN unsigned long AIOUSB_ADC_ExternalCal( unsigned long DeviceIndex, const double points[], int numPoints, unsigned short returnCalTable[], const char *saveFileName );



/* AIOUSB_ListDevices */

PUBLIC_EXTERN void AIOUSB_ListDevices();

/* AIOContinuousBuf */

PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , unsigned scancounts, unsigned number_channels );
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts );
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels );

PUBLIC_EXTERN void DeleteAIOContinuousBuf( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_InitConfiguration(  AIOContinuousBuf *buf );
PUBLIC_EXTERN void AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) );
PUBLIC_EXTERN void AIOContinuousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SendPreConfig( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SetStartAndEndChannel( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SetChannelRangeGain( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel , unsigned gainCode );
PUBLIC_EXTERN unsigned AIOContinuousBuf_GetOverSample( AIOContinuousBuf *buf );
PUBLIC_EXTERN void AIOContinuousBuf_SetOverSample( AIOContinuousBuf *buf, unsigned os );
PUBLIC_EXTERN void AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff );
PUBLIC_EXTERN void AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard );
PUBLIC_EXTERN unsigned AIOContinuousBuf_NumberChannels( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SaveConfig( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf );
PUBLIC_EXTERN unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask );
PUBLIC_EXTERN void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf );
PUBLIC_EXTERN unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf );
PUBLIC_EXTERN unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf );
PUBLIC_EXTERN unsigned int AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf );
PUBLIC_EXTERN unsigned int AIOContinuousBufGetSize( AIOContinuousBuf *buf );
PUBLIC_EXTERN THREAD_STATUS AIOContinuousBufGetStatus( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufGetExitCode( AIOContinuousBuf *buf );

#include "AIOUSB_CTR.h"

PUBLIC_EXTERN AIORET_TYPE CTR_8254Mode(
                                         unsigned long DeviceIndex,
                                         unsigned long BlockIndex,
                                         unsigned long CounterIndex,
                                         unsigned long Mode );

PUBLIC_EXTERN AIORET_TYPE CTR_8254Load(
                                         unsigned long DeviceIndex,
                                         unsigned long BlockIndex,
                                         unsigned long CounterIndex,
                                         unsigned short LoadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ModeLoad(
                                             unsigned long DeviceIndex,
                                             unsigned long BlockIndex,
                                             unsigned long CounterIndex,
                                             unsigned long Mode,
                                             unsigned short LoadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadModeLoad(
                                                 unsigned long DeviceIndex,
                                                 unsigned long BlockIndex,
                                                 unsigned long CounterIndex,
                                                 unsigned long Mode,
                                                 unsigned short LoadValue,
                                                 unsigned short *pReadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254Read( unsigned long DeviceIndex,
                                        unsigned long BlockIndex,
                                        unsigned long CounterIndex,
                                        unsigned short *pReadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadAll( unsigned long DeviceIndex,
                                           unsigned short *pData );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadStatus( unsigned long DeviceIndex,
                                              unsigned long BlockIndex,
                                              unsigned long CounterIndex,
                                              unsigned short *pReadValue,
                                              unsigned char *pStatus );

PUBLIC_EXTERN AIORET_TYPE CTR_StartOutputFreq( unsigned long DeviceIndex,
                                               unsigned long BlockIndex,
                                               double *pHz );

PUBLIC_EXTERN AIORET_TYPE CTR_8254SelectGate( unsigned long DeviceIndex,
                                              unsigned long GateIndex );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadLatched( unsigned long DeviceIndex,
                                               unsigned short *pData );




#endif
