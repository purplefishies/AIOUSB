#ifndef _AIO_FIFO_H
#define _AIO_FIFO_H

#include "AIOTypes.h"
#include "AIOEither.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

#define AIO_FIFO_INTERFACE                                                           \
    void *data;                                                                      \
    unsigned int refsize;                                                            \
    unsigned int size;                                                               \
    volatile unsigned int read_pos;                                                  \
    volatile unsigned int write_pos;                                                 \
    AIO_EITHER_TYPE kind;                                                            \
    AIORET_TYPE (*Read)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize );     \
    AIORET_TYPE (*Write)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize );    \
    void (*Reset)( struct aio_fifo *fifo );                                          \
    size_t (*delta)( struct aio_fifo *fifo  );                                       \
    size_t (*rdelta)( struct aio_fifo *fifo  );                                      \
    size_t (*_calculate_size_write)( struct aio_fifo *fifo, unsigned maxsize );      \
    size_t (*_calculate_size_read)( struct aio_fifo *fifo, unsigned maxsize );

typedef struct aio_fifo { 
    AIO_FIFO_INTERFACE;
    LOCKING_MECHANISM;
} AIOFifo;


typedef uint32_t TYPE;
typedef void INPUT_TYPE;

typedef struct new_aio_fifo {
    AIO_FIFO_INTERFACE;
    LOCKING_MECHANISM;
    AIORET_TYPE (*Push)( struct new_aio_fifo *fifo, TYPE a );
    AIORET_TYPE (*PushN)( struct new_aio_fifo *fifo, INPUT_TYPE *a, unsigned N );
    AIOEither (*Pop)( struct new_aio_fifo *fifo );
    AIORET_TYPE (*PopN)( struct new_aio_fifo *fifo , INPUT_TYPE *a, unsigned N );
} AIOFifoTYPE;


#define TEMPLATE_AIOFIFO_INTERFACE(NAME,TYPE)                                                       \
    typedef struct new_aio_fifo_##NAME {                                                            \
        AIO_FIFO_INTERFACE;                                                                         \
        LOCKING_MECHANISM;                                                                          \
        AIORET_TYPE (*Push)( struct new_aio_fifo_##NAME *fifo, TYPE a );                            \
        AIORET_TYPE (*PushN)( struct new_aio_fifo_##NAME *fifo, TYPE *a, unsigned N );              \
        AIOEither (*Pop)( struct new_aio_fifo_##NAME *fifo );                                       \
        AIORET_TYPE (*PopN)( struct new_aio_fifo_##NAME *fifo , TYPE *a, unsigned N );              \
    } AIOFifo##NAME;                                                                                \
    AIOFifo##NAME *NewAIOFifo##NAME( unsigned int size );                                           \
    void DeleteAIOFifo##NAME( AIOFifo##NAME *fifo );                                                \
    AIORET_TYPE AIOFifo##NAME ##Initialize( AIOFifo##NAME *nfifo );


#define TEMPLATE_AIOFIFO_API(NAME,TYPE)                                                             \
AIORET_TYPE NAME##Push( AIOFifo##NAME *fifo, TYPE a )                                               \
{                                                                                                   \
    TYPE tmp = (TYPE)a;                                                                             \
    int val = fifo->Write( (AIOFifo*)fifo, &tmp, sizeof(TYPE) );                                    \
    return val;                                                                                     \
}                                                                                                   \
AIORET_TYPE NAME##PushN( AIOFifo##NAME *fifo, TYPE *a, unsigned N )                                 \
{                                                                                                   \
    return fifo->Write( (AIOFifo*)fifo, a, N*sizeof(TYPE));                                         \
}                                                                                                   \
size_t NAME##delta( AIOFifo *tfifo  )                                                               \
{                                                                                                   \
    AIOFifo##NAME *fifo = (AIOFifo##NAME *)tfifo;                                                   \
    size_t tmp =( fifo->write_pos < fifo->read_pos ?                                                \
                  (fifo->read_pos - fifo->write_pos - 1 ) :                                         \
                  ( (fifo->size - fifo->write_pos) + fifo->read_pos - 1 ));                         \
    tmp = ( tmp / fifo->refsize) * fifo->refsize;                                                   \
    return tmp;                                                                                     \
}                                                                                                   \
AIOEither NAME##Pop( AIOFifo##NAME *fifo )                                                          \
{                                                                                                   \
    TYPE tmp;                                                                                       \
    AIOEither retval = {0};                                                                         \
    int tmpval = fifo->Read( (AIOFifo*)fifo, &tmp, sizeof(TYPE) );                                  \
                                                                                                    \
    if( tmpval <= 0 ) {                                                                             \
        retval.left = tmpval;                                                                       \
    } else {                                                                                        \
        AIOEitherSetRight( &retval, fifo->kind, &tmp );                                             \
    }                                                                                               \
                                                                                                    \
    return retval;                                                                                  \
}                                                                                                   \
AIORET_TYPE NAME##PopN( AIOFifo##NAME *fifo, TYPE *in, unsigned N)                                  \
{                                                                                                   \
    AIORET_TYPE retval = {0};                                                                       \
    int tmpval = fifo->Read( (AIOFifo*)fifo, in, sizeof(TYPE)*N );                                  \
    retval = ( tmpval < 0 ? -AIOUSB_ERROR_NOT_ENOUGH_MEMORY : tmpval );                             \
                                                                                                    \
    return retval;                                                                                  \
}                                                                                                   \
AIORET_TYPE AIOFifo##NAME ##Initialize( AIOFifo##NAME *nfifo )                                       \
{                                                                                                   \
    AIORET_TYPE retval = {0};                                                                       \
    nfifo->Push = NAME##Push;                                                                       \
    nfifo->PushN = NAME##PushN;                                                                     \
    nfifo->Pop = NAME##Pop;                                                                         \
    nfifo->PopN = NAME##PopN;                                                                       \
    nfifo->_calculate_size_write = _calculate_size_aon_write;                                       \
    nfifo->_calculate_size_read  = _calculate_size_aon_read;                                        \
    nfifo->Write = AIOFifoWriteAllOrNone;                                                           \
    nfifo->Read  = AIOFifoReadAllOrNone;                                                            \
    nfifo->delta = NAME##delta;                                                                     \
    nfifo->refsize = sizeof(TYPE);                                                                  \
    nfifo->kind = aioeither_value_##TYPE;                                                           \
    return retval;                                                                                  \
}                                                                                                   \
AIOFifo##NAME *NewAIOFifo##NAME( unsigned int size )                                                \
{                                                                                                   \
    AIOFifo##NAME *nfifo = (AIOFifo##NAME*)calloc(1,sizeof(AIOFifo##NAME));                         \
    AIOFifoInitialize( (AIOFifo*)nfifo , (size+1)*sizeof(TYPE), sizeof(TYPE));                      \
    AIOFifo##NAME ##Initialize( nfifo );                                                            \
    return nfifo;                                                                                   \
}                                                                                                   \
void DeleteAIOFifo##NAME( AIOFifo##NAME *fifo )                                                     \
{                                                                                                   \
    DeleteAIOFifo( (AIOFifo*)fifo);                                                                 \
}                                                                                                   \
AIORET_TYPE AIOFifo##NAME ##Resize( AIOFifo##NAME *nfifo , size_t size )                            \
{                                                                                                   \
    AIORET_TYPE retval = {0};                                                                       \
    retval = _AIOFifoResize( (AIOFifo *)nfifo, (size+1)*sizeof(TYPE));                              \
    return retval;                                                                                  \
}                                                                                                   \

/* Counts Fifo definition */
TEMPLATE_AIOFIFO_INTERFACE(Counts,uint16_t);
/* Volts Fifo definition */
TEMPLATE_AIOFIFO_INTERFACE(Volts,double);



AIOFifo *NewAIOFifo( unsigned int size , unsigned int refsize );
void DeleteAIOFifo( AIOFifo *fifo );

void AIOFifoReset( AIOFifo *fifo );
AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize );
AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize );
AIORET_TYPE AIOFifoWriteAllOrNone( AIOFifo *fifo, void *frombuf , unsigned maxsize );
AIORET_TYPE AIOFifoReadAllOrNone( AIOFifo *fifo, void *tobuf , unsigned maxsize );

AIOFifoTYPE *NewAIOFifoTYPE( unsigned int size );
AIORET_TYPE Push( AIOFifoTYPE *fifo, TYPE a );
AIORET_TYPE PushN( AIOFifoTYPE *fifo, INPUT_TYPE *a, unsigned N );
AIORET_TYPE PopN( AIOFifoTYPE *fifo, INPUT_TYPE *a, unsigned N );
AIORET_TYPE AIOFifoSizeRemaining( void *fifo );
AIORET_TYPE AIOFifoReadSize( void *tmpfifo );
AIORET_TYPE AIOFifoGetSize( void *fifo );
AIORET_TYPE AIOFifoResize( AIOFifo *fifo, size_t newsize );
#endif


#ifdef __aiousb_cplusplus
}
#endif


#endif



