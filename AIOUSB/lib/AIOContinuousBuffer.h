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
  unsigned hz;
  unsigned divisora;
  unsigned divisorb;
  unsigned _read_pos, _write_pos;
  unsigned size;
  unsigned counter_control;
  unsigned timeout;
  AIOChannelMask *mask;
  volatile enum THREAD_STATUS status;  /* Are we running, paused ..etc; */
} AIOContinuousBuf;

AIOContinuousBuf *NewAIOContinuousBuf( int bufsize, unsigned number_channels );
void DeleteAIOContinuousBuf( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf );
unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask );
void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex );
AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf );
unsigned int AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf );
void AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz );
AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode );

unsigned buffer_max( AIOContinuousBuf *buf );
unsigned AIOContinuousBufGetDivisorA( AIOContinuousBuf *buf );
unsigned AIOContinuousBufGetDivisorB( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, AIOBufferType *writebuf, unsigned size, AIOContinuousBufMode flag );
AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned size);
AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf );

#ifdef __aiousb_cplusplus
}
#endif

#endif
