/**
 * @file   AIOEither.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief  General structure for returning results from routines
 *
 */
#ifndef _AIOEITHER_H
#define _AIOEITHER_H

#include "AIOTypes.h"
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
    aioeither_value_uint8_t,
    aioeither_value_string,
    aioeither_value_longdouble_t,
    aioeither_value_obj,
} AIO_EITHER_TYPE;

typedef struct aio_either_val {
    AIO_NUMBER number;
    void *object;
} AIO_EITHER_VALUE_ITEM;

typedef struct aio_ret_value  {
    int left;
    char *errmsg;
    AIO_EITHER_VALUE_ITEM right;
    AIO_EITHER_TYPE type;
    int size;
} AIOEither;

#define AIO_ERROR_VALUE 0xffffffffffffffff


void AIOEitherClear( AIOEither *retval );

void AIOEitherSetRight(AIOEither *retval, AIO_EITHER_TYPE val , void *tmp, ... );
void AIOEitherGetRight(AIOEither *retval, void *tmp, ... );

void AIOEitherSetLeft(AIOEither *retval, int val );
int  AIOEitherGetLeft(AIOEither *retval );
AIOUSB_BOOL AIOEitherHasError( AIOEither *retval );

char *AIOEitherToString( AIOEither *retval, AIORET_TYPE *result );
int AIOEitherToInt( AIOEither retval);
short AIOEitherToShort( AIOEither *retval, AIORET_TYPE *result );
unsigned AIOEitherToUnsigned( AIOEither *retval, AIORET_TYPE *result );
double AIOEitherToDouble( AIOEither *retval, AIORET_TYPE *result );
AIO_NUMBER AIOEitherToAIONumber( AIOEither *retval, AIORET_TYPE *result );
AIORET_TYPE AIOEitherToAIORetType( AIOEither either );




#ifdef __aiousb_cplusplus
}
#endif



#endif
