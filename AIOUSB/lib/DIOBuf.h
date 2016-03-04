#ifndef _DIO_BUF_H
#define _DIO_BUF_H

#include "AIOTypes.h"
#include "AIOChannelMask.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif

typedef struct {
    unsigned _size;
    unsigned char *_buffer;
    char *_strbuf;
    int _strbuf_size;
} DIOBuf;


typedef unsigned char DIOBufferType ;

/* BEGIN AIOUSB_API */

PUBLIC_EXTERN DIOBuf *NewDIOBuf ( unsigned size );
PUBLIC_EXTERN void DeleteDIOBuf ( DIOBuf  *buf );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromChar( const char *ary , int size_array );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromBinStr( const char *ary );
PUBLIC_EXTERN DIOBuf *DIOBufReplaceString( DIOBuf *buf, char *ary, int size_array );
PUBLIC_EXTERN DIOBuf *DIOBufReplaceBinString( DIOBuf *buf, char *bitstr );
PUBLIC_EXTERN char *DIOBufToHex( DIOBuf *buf );
PUBLIC_EXTERN char *DIOBufToBinary( DIOBuf *buf );
PUBLIC_EXTERN char *DIOBufToInvertedBinary( DIOBuf *buf );
PUBLIC_EXTERN DIOBuf  *DIOBufResize( DIOBuf  *buf , unsigned size );
PUBLIC_EXTERN unsigned DIOBufSize( DIOBuf  *buf );
PUBLIC_EXTERN unsigned DIOBufByteSize( DIOBuf *buf );
PUBLIC_EXTERN char *DIOBufToString( DIOBuf  *buf );
PUBLIC_EXTERN AIORET_TYPE DIOBufSetIndex( DIOBuf *buf, int index, unsigned value );
PUBLIC_EXTERN AIORET_TYPE DIOBufGetIndex( DIOBuf *buf, int index );
PUBLIC_EXTERN AIORET_TYPE DIOBufGetByteAtIndex( DIOBuf *buf, unsigned index, char *value);
PUBLIC_EXTERN AIORET_TYPE DIOBufSetByteAtIndex( DIOBuf *buf, unsigned index, char  value );

/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif

#endif


