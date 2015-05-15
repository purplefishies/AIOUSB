/**
 * @file   AIOBuf.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief  
 */

#include "AIOBuf.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
namespace AIOUSB {
#endif 

/*----------------------------------------------------------------------------*/
AIOBuf * NewAIOBuf( AIOBufType type , size_t size )
{
    AIOBuf *ret = NULL;
    return ret;
}

/*----------------------------------------------------------------------------*/

AIORET_TYPE DeleteAIOBuf( AIOBuf *type )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOBufSize( AIOBuf *buf )
{
    AIORET_TYPE retval = 10;
    return retval;
}

/*----------------------------------------------------------------------------*/

AIORET_TYPE AIOBufRead( AIOBuf *buf, void *tobuf, size_t size_tobuf )
{
    AIORET_TYPE retval = 10;
    return retval;
}

/*----------------------------------------------------------------------------*/
AIOBufIterator *AIOBufGetIterator( AIOBuf *buf )
{
    AIOBufIterator *tmp = NULL;
    
    return tmp;
}

/*----------------------------------------------------------------------------*/
AIOUSB_BOOL AIOBufIteratorIsValid( AIOBufIterator *biter )
{
    AIOUSB_BOOL retval = AIOUSB_TRUE;
    return retval;
}
/*----------------------------------------------------------------------------*/

void AIOBufIteratorNext( AIOBufIterator *biter )
{
    printf("do something\n");
}


#ifdef __cplusplus
}
#endif 

#ifdef SELF_TEST
#include "AIOUSBDevice.h"
#include "gtest/gtest.h"

using namespace AIOUSB;

#endif
