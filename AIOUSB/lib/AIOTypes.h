#ifndef _AIOTYPES_H
#define _AIOTYPES_H
#define HAS_PTHREAD 1
#include <aiousb.h>


#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif



typedef struct object {
  struct object *next;
  struct object *tmp;
} Object;

enum THREAD_STATUS { 
  NOT_STARTED = 0, 
  RUNNING, 
  TERMINATED, 
  JOINED 
};

CREATE_ENUM_W_START( AIOContinuousBufMode, 0 ,
                     AIOCONTINUOUS_BUF_ALLORNONE,
                     AIOCONTINUOUS_BUF_NORMAL,
                     AIOCONTINUOUS_BUF_OVERRIDE
                     );

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif


#define AUR_CBUF_SETUP  0x01000007
#define AUR_CBUF_EXIT   0x00020002

#define NUMBER_CHANNELS 16


typedef short AIOBufferType;
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
  volatile enum THREAD_STATUS status;  /* Are we running, paused ..etc; */
} AIOContinuousBuf;


AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf );
unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf );
void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex );
unsigned buffer_max( AIOContinuousBuf *buf );
unsigned AIOContinuousBufGetDivisorA( AIOContinuousBuf *buf );
unsigned AIOContinuousBufGetDivisorB( AIOContinuousBuf *buf );
AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, AIOBufferType *writebuf, unsigned size, AIOContinuousBufMode flag );
AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf );
 AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned size);


#ifdef __aiousb_cplusplus
}
#endif

#endif
