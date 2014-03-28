/**
 * @file   AIOTypes.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 *
 */

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

CREATE_ENUM_W_START( THREAD_STATUS, 0 , 
                     NOT_STARTED, 
                     RUNNING, 
                     TERMINATED, 
                     JOINED 
                     );

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

/**< Simple macro for iterating over objects */
#define foreach_array( i , ary, size )  i = ary[0]; \
                                        for ( int j = 0; j < size ; j ++, i = ary[i] )

typedef double AIOBufferType;


#ifdef __aiousb_cplusplus
}
#endif

#endif
