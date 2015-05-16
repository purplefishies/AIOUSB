#ifndef _AIO_BUF_H
#define _AIO_BUF_H

#include "AIOTypes.h"
#include "AIOEither.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef enum {
    AIO_ERROR_BUF = -1,
    AIO_DEFAULT_BUF = 1,
    AIO_COUNTS_BUF = 2,
    AIO_VOLTS_BUF  = 8
} AIOBufType;




typedef struct aiobuf {
    size_t size;
    void *_buf;
    AIOBufType type;
    AIOUSB_BOOL defined;
} AIOBuf;

typedef struct aiobuf_iterator {
    AIOBuf *buf;
    void *loc;
    void (*next)(struct aiobuf_iterator *);
} AIOBufIterator;

typedef struct aio_cmd {
    int stop_scan;
    int stop_scan_arg;
    int channel;
} AIOCmd;


PUBLIC_EXTERN AIOBuf * NewAIOBuf( AIOBufType type , size_t size );
PUBLIC_EXTERN AIORET_TYPE DeleteAIOBuf( AIOBuf *type );
PUBLIC_EXTERN AIORET_TYPE AIOBufGetSize( AIOBuf *buf );
PUBLIC_EXTERN AIOBufType AIOBufGetType( AIOBuf *buf );
PUBLIC_EXTERN AIORET_TYPE AIOBufRead( AIOBuf *buf, void *tobuf, size_t size_tobuf );
PUBLIC_EXTERN AIORET_TYPE AIOBufWrite( AIOBuf *buf, void *frombuf, size_t size_frombuf );
PUBLIC_EXTERN AIOBufIterator *AIOBufGetIterator( AIOBuf *buf );
PUBLIC_EXTERN AIOUSB_BOOL AIOBufIteratorIsValid( AIOBufIterator *biter );
PUBLIC_EXTERN void AIOBufIteratorNext( AIOBufIterator *biter );


#ifdef __aiousb_cplusplus
}
#endif



#endif
