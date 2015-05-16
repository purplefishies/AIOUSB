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
    AIOBuf *ret = (AIOBuf *)calloc(1,sizeof(AIOBuf));
    if (!ret )
        return ret;
    ret->_buf = malloc( ((int)type)*size );

    if ( !ret->_buf ) {
        free(ret);
        ret = NULL;
    }
    ret->size = size;
    ret->type = type;
    ret->defined = AIOUSB_TRUE;
    return ret;
}

/*----------------------------------------------------------------------------*/

AIORET_TYPE DeleteAIOBuf( AIOBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ASSERT( buf );
    if ( buf->_buf ) {
        free( buf->_buf );

    } else {
        retval = -AIOUSB_ERROR_INVALID_MEMORY;
    }
    free( buf );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOBufGetSize( AIOBuf *buf )
{
    AIO_ASSERT( buf );

    AIORET_TYPE retval = buf->size;

    return retval;
}

AIORET_TYPE AIOBufGetTotalSize( AIOBuf *buf )
{
    AIO_ASSERT( buf );
    AIO_ERROR_VALID_DATA( -AIOUSB_ERROR_INVALID_AIOBUFTYPE, (int)buf->type > 0 );
    return buf->size * (int)buf->type;
}



/*----------------------------------------------------------------------------*/
AIOBufType AIOBufGetType( AIOBuf *buf )
{
    AIO_ASSERT_RET( AIO_ERROR_BUF, buf );

    AIOBufType retval = buf->type;
    
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOBufGetTypeSize( AIOBuf *buf )
{
    AIO_ASSERT( buf );
    
    AIOBufType retval = buf->type;

    return (AIORET_TYPE)(int)retval;
}


/*----------------------------------------------------------------------------*/

AIORET_TYPE AIOBufRead( AIOBuf *buf, void *tobuf, size_t size_tobuf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ASSERT( buf );
    AIO_ASSERT( tobuf );
    
    int ibufsize;
    
    AIO_ASSERT_RET(-AIOUSB_ERROR_INVALID_AIOBUFTYPE, (ibufsize = (int)AIOBufGetTotalSize(buf) ) >= 0  );

    int act_size = MIN( ibufsize, (int)size_tobuf );
    memcpy( tobuf, buf->_buf, act_size );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOBufWrite( AIOBuf *buf, void *frombuf, size_t size_frombuf )
{
    AIO_ASSERT( buf );
    AIO_ASSERT( frombuf );
    int obufsize;

    AIO_ASSERT_RET( -AIOUSB_ERROR_INVALID_AIOBUFTYPE, (obufsize = (int)AIOBufGetTotalSize(buf) ) >= 0 );

    int act_size = MIN( obufsize, (int)size_frombuf );
    memcpy( buf->_buf, frombuf, act_size );
    return act_size;
}

/*----------------------------------------------------------------------------*/
AIOBufIterator *AIOBufGetIterator( AIOBuf *buf )
{
    AIO_ASSERT_RET(NULL,buf);

    AIOBufIterator *tmp = NULL;
    tmp = (AIOBufIterator *)calloc(1,sizeof(AIOBufIterator));
    
    tmp->loc = buf->_buf;
    tmp->next = AIOBufIteratorNext;
    tmp->buf = buf;

    return tmp;
}

/*----------------------------------------------------------------------------*/
AIOUSB_BOOL AIOBufIteratorIsValid( AIOBufIterator *biter )
{
    AIOUSB_BOOL retval = AIOUSB_TRUE;
    void *tmp;

    tmp = (void *)( (char *)biter->buf->_buf + AIOBufGetTotalSize( biter->buf ) );
    if ( biter->loc >= tmp ) 
        retval = AIOUSB_FALSE;

    return retval;
}

/*----------------------------------------------------------------------------*/
void AIOBufIteratorNext( AIOBufIterator *biter )
{
    biter->loc = (void *)((char*)biter->loc + AIOBufGetTypeSize(biter->buf));
    /* printf("do something\n"); */
    /* biter->loc += biter->size; */
}



AIO_NUMBER AIOBufIteratorGetValue( AIOBufIterator *biter )
{
    AIO_NUMBER retval;
    switch ( AIOBufGetTypeSize( biter->buf ) ) { 
    case 2:
        { 
            uint16_t tmp;
            memcpy(&tmp, biter->loc , sizeof(uint16_t ));
            retval = (AIO_NUMBER)tmp;
        }
        break;
    case 4:
        {
            uint32_t tmp;
            memcpy(&tmp, biter->loc, sizeof(uint32_t));
            retval = (AIO_NUMBER)tmp;
        }
        break;
    case 8:
        {
            uint64_t tmp;
            memcpy(&tmp, biter->loc, sizeof(uint64_t));
            retval = (AIO_NUMBER)tmp;
        }
        break;
    case 16:
        {
            AIO_NUMBER tmp;
            memcpy(&tmp, biter->loc, sizeof(AIO_NUMBER));
            retval = tmp;
        }
        break;
    default:                    /* 1 byte */
        {
            uint8_t tmp;
            memcpy(&tmp, biter->loc, sizeof(uint64_t));
            retval = (AIO_NUMBER)tmp;
        }
        break;
    }
    return retval;
}


#ifdef __cplusplus
}
#endif 

#ifdef SELF_TEST
#include "AIOUSBDevice.h"
#include "AIOTypes.h"
#include "gtest/gtest.h"

using namespace AIOUSB;


TEST(AIOBuf, CreationAndDestruction) 
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOBuf *buf = NewAIOBuf( AIO_DEFAULT_BUF, 100 );
    ASSERT_TRUE( buf );
    
    ASSERT_EQ( 100, AIOBufGetSize( buf ));

    ASSERT_EQ( AIO_DEFAULT_BUF, AIOBufGetType( buf ));

    ASSERT_EQ( 100, AIOBufGetTotalSize(buf) );

    retval = DeleteAIOBuf( buf );
    ASSERT_EQ( AIOUSB_SUCCESS, retval );

    buf = NewAIOBuf( AIO_COUNTS_BUF, 100 );
    ASSERT_EQ( 100 * (int)AIO_COUNTS_BUF, AIOBufGetTotalSize(buf) );
    retval = DeleteAIOBuf( buf );
    ASSERT_EQ( AIOUSB_SUCCESS, retval );

}

TEST(AIOBuf, WriteIntoBuffer )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOBuf *buf = NewAIOBuf( AIO_DEFAULT_BUF, 100 );
    char tmp[110];
    char tmp2[100];
    ASSERT_TRUE(tmp);

    for( int i = 0; i < sizeof(tmp); i ++ ) tmp[i] = (char )i;

    retval = AIOBufWrite( buf, tmp, sizeof(tmp) );
    ASSERT_EQ( 100, retval );

    EXPECT_EQ( 0 ,  memcmp( tmp, buf->_buf, sizeof( retval )) );
    
    AIOBufRead( buf, tmp2, sizeof(tmp2));
    EXPECT_EQ( 0 ,  memcmp( tmp2, tmp, sizeof( tmp2 )) );



    retval = DeleteAIOBuf( buf );
    ASSERT_EQ( AIOUSB_SUCCESS, retval );

}

TEST(AIOBufIterator,GoThroughAllValues)
{
    AIORET_TYPE retval =  AIOUSB_SUCCESS;
    AIOBuf *buf = NewAIOBuf( AIO_COUNTS_BUF, 100 );
    uint16_t testvalues[100];
    AIOBuf *tmpbuf = NewAIOBuf( AIO_COUNTS_BUF, 0 );
    AIOBufIterator *iter;
    unsigned short *counts;
    int i = 0;
    for ( counts = (uint16_t *)buf->_buf, i = 0; counts < (uint16_t*)buf->_buf + 100; counts += 1 , i ++ )
        *counts = (uint16_t)i;
    
    ASSERT_DEATH({ iter = AIOBufGetIterator( NULL ); }, "Assertion `buf' failed" );

    iter = AIOBufGetIterator( buf );
    ASSERT_TRUE( iter );
    ASSERT_TRUE( iter->buf );

    ASSERT_TRUE( iter->loc );
    ASSERT_TRUE( iter->next );
    ASSERT_TRUE( AIOBufIteratorIsValid( iter ) );
    
    iter = AIOBufGetIterator( tmpbuf );
    ASSERT_FALSE( AIOBufIteratorIsValid( iter ));

    iter = AIOBufGetIterator( buf );
    iter->loc = &((uint16_t *)buf->_buf)[100];
    ASSERT_FALSE( AIOBufIteratorIsValid( iter )) << "Very end should indicate we've gone too far\n";

    iter->loc = &((uint16_t *)buf->_buf)[99];
    ASSERT_TRUE( AIOBufIteratorIsValid( iter )) << "Still should have an element left\n";

    iter->next(iter);
    ASSERT_EQ( iter->loc, &((uint16_t *)buf->_buf)[100] );

    iter = AIOBufGetIterator( buf );
    iter->next(iter);
    ASSERT_EQ( iter->loc, &((uint16_t *)buf->_buf)[1] );
    

    int j = 0;
    for ( iter = AIOBufGetIterator( buf ), j = 0; AIOBufIteratorIsValid(iter); iter->next(iter) , j ++ ) {
        testvalues[j] = AIOBufIteratorGetValue(iter);
    }

    ASSERT_EQ( 0, memcmp( testvalues, buf->_buf , sizeof(testvalues)) );

    retval = DeleteAIOBuf( buf );
    ASSERT_EQ( AIOUSB_SUCCESS, retval );

}

int main(int argc, char *argv[] )
{

  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
  delete listeners.Release(listeners.default_result_printer());
#endif

  return RUN_ALL_TESTS();  
}


#endif
