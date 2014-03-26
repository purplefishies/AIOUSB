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

AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , unsigned scancounts, unsigned number_channels );
AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts );
AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels );
AIOContinuousBuf *NewAIOContinuousBufTesting( unsigned long DeviceIndex , unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts  );

void DeleteAIOContinuousBuf( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBuf_InitConfiguration(  AIOContinuousBuf *buf );

void AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) );
void AIOContinuousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing );
AIORET_TYPE AIOContinuousBuf_SendPreConfig( AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBuf_SetStartAndEndChannel( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel );
AIORET_TYPE AIOContinuousBuf_SetChannelRangeGain( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel , unsigned gainCode );
unsigned AIOContinuousBuf_GetOverSample( AIOContinuousBuf *buf );
void AIOContinuousBuf_SetOverSample( AIOContinuousBuf *buf, unsigned os );
void AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff );
void AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard );
unsigned AIOContinuousBuf_NumberChannels( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBuf_SaveConfig( AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf );
unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask );
void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex );
AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufGetSize( AIOContinuousBuf *buf );
THREAD_STATUS AIOContinuousBufGetStatus( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufGetExitCode( AIOContinuousBuf *buf );


AIORET_TYPE AIOContinuousBufReadIntegerScanCounts( AIOContinuousBuf *buf, unsigned short *tmp , unsigned tmpsize, unsigned size );


unsigned AIOContinuousBuf_NumberWriteScansInCounts(AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBufCountScansAvailable(AIOContinuousBuf *buf);



void AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz );
AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode );
AIORET_TYPE AIOContinuousBuf_ResetDevice( AIOContinuousBuf *buf);

unsigned buffer_max( AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned readbufsize, unsigned size);
AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, AIOBufferType *writebuf, unsigned wrbufsize, unsigned size, AIOContinuousBufMode flag );
AIORET_TYPE AIOContinuousBufWriteCounts( AIOContinuousBuf *buf, unsigned short *data, unsigned datasize, unsigned size , AIOContinuousBufMode flag );

AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf );

AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf );

#ifdef __aiousb_cplusplus
}
#endif

#endif
