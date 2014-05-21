/**
 * @file   AIOTypes.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 *
 */

#ifndef _AIO_CONTINUOUS_BUFFER_H
#define _AIO_CONTINUOUS_BUFFER_H

#include "AIOTypes.h"
#include "AIOChannelMask.h"
#include "AIOTypes.h"
#include <pthread.h>


#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef void *(*AIOUSB_WorkFn)( void *obj );


typedef struct {
  void *(*callback)(void *object);
#ifdef HAS_PTHREAD
  pthread_t worker;
  pthread_mutex_t lock;
  pthread_attr_t tattr;
#endif
  AIOUSB_WorkFn work;
  unsigned long DeviceIndex;
  AIOBufferType *buffer;
  unsigned char *countsbuf;
  unsigned bufunitsize;
  unsigned hz;
  unsigned usbbuf_size;
  unsigned divisora;
  unsigned divisorb;
  unsigned _read_pos, _write_pos;
  /* unsigned totalsize; */
  unsigned basesize;
  unsigned size;
  unsigned counter_control;
  unsigned timeout;
  AIORET_TYPE exitcode;
  AIOUSB_BOOL testing;
  unsigned extra;                     /**< Keeps track of under writes */
  AIOChannelMask *mask;               /**< Used for keeping track of channels */
  AIOBufferType *tmpbuf;
  unsigned tmpbufsize;
  volatile THREAD_STATUS status; /* Are we running, paused ..etc; */
} AIOContinuousBuf;

PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , unsigned scancounts, unsigned number_channels );
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts );
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels );
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufTesting( unsigned long DeviceIndex , unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts  );

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
PUBLIC_EXTERN unsigned AIOContinuousBuf_NumberSignals( AIOContinuousBuf *buf );
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


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufReadIntegerScanCounts( AIOContinuousBuf *buf, unsigned short *tmp , unsigned tmpsize, unsigned size );
PUBLIC_EXTERN unsigned AIOContinuousBuf_NumberWriteScansInCounts(AIOContinuousBuf *buf );

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufCountScansAvailable(AIOContinuousBuf *buf);



PUBLIC_EXTERN void AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_ResetDevice( AIOContinuousBuf *buf);


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned readbufsize, unsigned size);
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, AIOBufferType *writebuf, unsigned wrbufsize, unsigned size, AIOContinuousBufMode flag );
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufWriteCounts( AIOContinuousBuf *buf, unsigned short *data, unsigned datasize, unsigned size , AIOContinuousBufMode flag );

PUBLIC_EXTERN AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf );

#ifdef __aiousb_cplusplus
}
#endif

#endif
