#ifndef LIBAIOUSB_H
#define LIBAIOUSB_H

#include <mex.h>  /* only needed because of mexFunction below and mexPrintf */
#include "AIOUSB_Core.h"
#include "AIOChannelMask.h"
#include "AIOContinuousBuffer.h"
#include "AIODataTypes.h"
#include "AIOTypes.h"
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

PUBLIC_EXTERNAL AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , unsigned scancounts, unsigned number_channels );
PUBLIC_EXTERNAL AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts );
PUBLIC_EXTERNAL AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels );
PUBLIC_EXTERNAL AIOContinuousBuf *NewAIOContinuousBufTesting( unsigned long DeviceIndex , unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts  );
PUBLIC_EXTERNAL void DeleteAIOContinuousBuf( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_InitConfiguration(  AIOContinuousBuf *buf );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_SendPreConfig( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_SetStartAndEndChannel( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_SetChannelRangeGain( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel , unsigned gainCode );
PUBLIC_EXTERNAL unsigned AIOContinuousBuf_GetOverSample( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetOverSample( AIOContinuousBuf *buf, unsigned os );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard );
PUBLIC_EXTERNAL unsigned AIOContinuousBuf_NumberChannels( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_SaveConfig( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask );
PUBLIC_EXTERNAL void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL unsigned int AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL unsigned int AIOContinuousBufGetSize( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL THREAD_STATUS AIOContinuousBufGetStatus( AIOContinuousBuf *buf );
PUBLIC_EXTERNAL AIORET_TYPE AIOContinuousBufGetExitCode( AIOContinuousBuf *buf );

/* DIO */
PUBLIC_EXTERN unsigned long DIO_Configure(
                                          unsigned long DeviceIndex,
                                          unsigned char bTristate,
                                          void *pOutMask,
                                          void *pData
                                          ) ;
PUBLIC_EXTERN unsigned long DIO_ConfigureEx( 
                                            unsigned long DeviceIndex, 
                                            void *pOutMask, 
                                            void *pData, 
                                            void *pTristateMask 
                                             ) ; 
PUBLIC_EXTERN unsigned long DIO_ConfigurationQuery(
                                     unsigned long DeviceIndex,
                                     void *pOutMask,
                                     void *pTristateMask
                                     ) ;
PUBLIC_EXTERN unsigned long DIO_WriteAll(
                                         unsigned long DeviceIndex,
                                         void *pData
                                         ) ;
PUBLIC_EXTERN unsigned long DIO_Write8(
                         unsigned long DeviceIndex,
                         unsigned long ByteIndex,
                         unsigned char Data
                         ) ;
PUBLIC_EXTERN unsigned long DIO_Write1(
                         unsigned long DeviceIndex,
                         unsigned long BitIndex,
                         unsigned char bData
                         ) ;
PUBLIC_EXTERN unsigned long DIO_ReadAll(
                          unsigned long DeviceIndex,
                          void *Buffer
                          ) ;
PUBLIC_EXTERN unsigned long DIO_Read8(
                        unsigned long DeviceIndex,
                        unsigned long ByteIndex,
                        unsigned char *pBuffer
                        ) ;
PUBLIC_EXTERN unsigned long DIO_Read1(
                        unsigned long DeviceIndex,
                        unsigned long BitIndex,
                        unsigned char *pBuffer
                        ) ;
PUBLIC_EXTERN unsigned long DIO_StreamOpen(
                             unsigned long DeviceIndex,
                             unsigned long bIsRead
                             ) ;
PUBLIC_EXTERN unsigned long DIO_StreamClose(
                              unsigned long DeviceIndex
                              ) ;
PUBLIC_EXTERN unsigned long DIO_StreamSetClocks(
                                  unsigned long DeviceIndex,
                                  double *ReadClockHz,
                                  double *WriteClockHz
                                  ) ;
PUBLIC_EXTERN unsigned long DIO_StreamFrame(
                              unsigned long DeviceIndex,
                              unsigned long FramePoints,
                              unsigned short *pFrameData,
                              unsigned long *BytesTransferred
                              ) ;



#endif
