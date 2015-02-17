/**
 * @file   AIOEither.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  General structure for returning results from routines
 *
 */
#ifndef _AIOEITHER_H
#define _AIOEITHER_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif


typedef enum { 
    aioeither_value_int = 1,
    aioeither_value_int32_t = 1,
    aioeither_value_uint32_t = 2,
    aioeither_value_unsigned = 2,
    aioeither_value_uint16_t = 3,
    aioeither_vlaue_int16_t = 4,
    aioeither_value_double_t = 5,
    aioeither_value_double = 5,
    aioeither_value_string = 6,
    aioeither_value_obj,
} AIO_EITHER_TYPE;

typedef union { 
    int i;
    unsigned int u;
    uint16_t us;
    int16_t s;
    double d; 
    char *st;
    void *v;
} AIO_EITHER_VALUE_ITEM;

typedef struct aio_ret_value  {
    int left;
    char *errmsg;
    AIO_EITHER_VALUE_ITEM right;
    AIO_EITHER_TYPE type;
    int size;
} AIOEither;


void AIOEitherClear( AIOEither *retval );

void AIOEitherSetRight(AIOEither *retval, AIO_EITHER_TYPE val , void *tmp, ... );
void AIOEitherGetRight(AIOEither *retval, void *tmp, ... );

void AIOEitherSetLeft(AIOEither *retval, int val );
int  AIOEitherGetLeft(AIOEither *retval );



#ifdef __aiousb_cplusplus
}
#endif



#endif
