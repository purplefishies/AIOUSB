#ifndef _DIO_BUF_H
#define _DIO_BUF_H

#include "AIOTypes.h"
#include "DIOBuf.h"
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

#ifndef PUBLIC_EXTERN
#define PUBLIC_EXTERN extern
#endif

typedef unsigned char DIOBufferType ;

typedef struct { 
  unsigned _size;
  unsigned char *_buffer;        /**> Actual elements  */
  char *_strbuf;                 /**> Display string   */
} DIOBuf;

PUBLIC_EXTERN DIOBuf *NewDIOBuf ( unsigned size );
PUBLIC_EXTERN void DeleteDIOBuf ( DIOBuf  *buf );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromChar( const char *ary , int size_array );
PUBLIC_EXTERN DIOBuf *NewDIOBufFromBinStr( const char *ary );
PUBLIC_EXTERN DIOBuf *DIOBufReplaceString( DIOBuf *buf, char *ary, int size_array );
PUBLIC_EXTERN char *DIOBufToHex( DIOBuf *buf );
PUBLIC_EXTERN char *DIOBufToBinary( DIOBuf *buf );
PUBLIC_EXTERN DIOBuf  *DIOBufResize( DIOBuf  *buf , unsigned size );
PUBLIC_EXTERN unsigned DIOBufSize( DIOBuf  *buf );
PUBLIC_EXTERN char *DIOBufToString( DIOBuf  *buf );
PUBLIC_EXTERN int DIOBufSetIndex( DIOBuf *buf, unsigned index, unsigned value );
PUBLIC_EXTERN int DIOBufGetIndex( DIOBuf *buf, unsigned index );


#ifdef __aiousb_cplusplus
}
#endif

#endif


