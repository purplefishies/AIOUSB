#ifndef _AIO_FIFO_H
#define _AIO_FIFO_H

#include "AIOTypes.h"
#include "AIOEither.h"
#include <stdint.h>
#include <stdlib.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


#ifdef __KERNEL__

AIOFifo *NewAIOFifo( unsigned int size );
void DeleteAIOFifo( AIOFifo *fifo );
AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize );
AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize );

#else

#ifdef HAS_THREAD
#define LOCKING_MECHANISM  pthread_mutex_t lock;
#define GRAB_RESOURCE(obj)    pthread_mutex_lock( &obj->lock );
#define RELEASE_RESOURCE(obj) pthread_mutex_unlock( &obj->lock );
#else
#define LOCKING_MECHANISM  ;
#define GRAB_RESOURCE(obj) ;
#define RELEASE_RESOURCE(obj);
#endif

#define AIO_FIFO_INTERFACE                                              \
    void *data;                                                         \
    unsigned int refsize;                                               \
    unsigned int size;                                                  \
    volatile unsigned int read_pos;                                     \
    volatile unsigned int write_pos;                                    \
    AIORET_TYPE (*Read)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize ); \
    AIORET_TYPE (*Write)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize ); \
    size_t (*delta)( struct aio_fifo *fifo  );                          \
    size_t (*_calculate_size_write)( struct aio_fifo *fifo, unsigned maxsize ); \
    size_t (*_calculate_size_read)( struct aio_fifo *fifo, unsigned maxsize );

/* typedef enum {  */
/*     aioret_value_int = 1, */
/*     aioret_value_int32_t = 1, */
/*     aioret_value_uint32_t = 2, */
/*     aioret_value_unsigned = 2, */
/*     aioret_value_double_t = 3, */
/*     aioret_value_double = 3, */
/*     aioret_value_string = 4, */
/*     aioret_value_obj, */
/* } AIORET_VALUE_TYPE; */
/* typedef union {  */
/*     int i; */
/*     unsigned int u; */
/*     double d;  */
/*     char *s; */
/*     void *v; */
/* } AIORET_VALUE_ITEM; */
/* typedef struct aio_ret_value  { */
/*     int left; */
/*     AIORET_VALUE_ITEM right; */
/*     AIORET_VALUE_TYPE type; */
/* } AIORET_VALUE; */

typedef struct aio_fifo { 
    AIO_FIFO_INTERFACE;
    LOCKING_MECHANISM;
} AIOFifo;

/* typedef unsigned int TYPE; */
typedef uint32_t TYPE;

typedef struct new_aio_fifo {
    AIO_FIFO_INTERFACE;
    LOCKING_MECHANISM;
    AIORET_TYPE (*Push)( struct new_aio_fifo *fifo, TYPE a );
    AIORET_TYPE (*PushN)( struct new_aio_fifo *fifo, TYPE *a, unsigned N );
    AIOEither (*Pop)( struct new_aio_fifo *fifo );
    AIORET_TYPE (*PopN)( struct new_aio_fifo *fifo , TYPE *a, unsigned N );
} AIOFifoTYPE;


AIOFifo *NewAIOFifo( unsigned int size , unsigned int refsize );
void DeleteAIOFifo( AIOFifo *fifo );
AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize );
AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize );
AIORET_TYPE AIOFifoWriteAllOrNone( AIOFifo *fifo, void *frombuf , unsigned maxsize );
AIORET_TYPE AIOFifoReadAllOrNone( AIOFifo *fifo, void *tobuf , unsigned maxsize );

AIOFifoTYPE *NewAIOFifoTYPE( unsigned int size );
AIORET_TYPE Push( AIOFifoTYPE *fifo, TYPE a );
AIORET_TYPE PushN( AIOFifoTYPE *fifo, TYPE *a, unsigned N );


#endif


#ifdef __aiousb_cplusplus
}
#endif


#endif



