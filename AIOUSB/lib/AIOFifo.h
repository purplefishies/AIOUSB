#ifndef _AIO_FIFO_H
#define _AIO_FIFO_H

#include "AIOTypes.h"
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
typedef struct aio_fifo { 
    void *data;
    unsigned int refsize;
    unsigned int size;
    volatile unsigned int read_pos;
    volatile unsigned int write_pos;
    AIORET_TYPE (*Read)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize );
    AIORET_TYPE (*Write)( struct aio_fifo *fifo, void *tobuf, unsigned maxsize );
    size_t (*delta)( struct aio_fifo *fifo  );

#if 1
    pthread_mutex_t lock;
#endif

} AIOFifo;

AIOFifo *NewAIOFifo( unsigned int size , unsigned int refsize );
void DeleteAIOFifo( AIOFifo *fifo );
AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize );
AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize );

#endif


#ifdef __aiousb_cplusplus
}
#endif


#endif



