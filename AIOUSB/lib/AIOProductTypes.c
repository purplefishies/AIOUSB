#include "AIOProductTypes.h"
#include <stdarg.h>
#include <string.h>


#ifdef __cplusplus
namespace AIOUSB {
#endif


AIOProductRange *NewAIOProductRange( unsigned long start, unsigned long end)
{
    AIOProductRange *tmp = (AIOProductRange*)malloc(sizeof(AIOProductRange));
    if (!tmp) return NULL;
    tmp->_start = start;
    tmp->_end   = end;
    return tmp;
}

AIORET_TYPE DeleteAIOProductRange( AIOProductRange *pr )
{
    AIO_ASSERT( pr );
    free(pr);
    return AIOUSB_SUCCESS;
}

AIORET_TYPE AIOProductRangeStart( AIOProductRange *pr )
{
    AIO_ASSERT(pr);
    return pr->_start;
}

AIORET_TYPE AIOProductRangeEnd( AIOProductRange *pr )
{
    AIO_ASSERT(pr);
    return pr->_end;
}

AIOProductGroup *NewAIOProductGroup(size_t numbergroups, ... )
{
    va_list arguments;
    int i;
    AIOProductGroup *ng = (AIOProductGroup*)malloc(sizeof(AIOProductGroup));
    if ( !ng ) return NULL;
    ng->_groups = (AIOProductRange**)malloc(sizeof(AIOProductRange*)*numbergroups );
    if (!ng->_groups ) goto cleanup;
    ng->_num_groups = numbergroups;

    va_start( arguments, numbergroups );
    for ( i = 0; i < (int)numbergroups ; i ++ ) {
        AIOProductRange *tmp = va_arg( arguments, AIOProductRange*);
        if ( !tmp ) goto err;
        ng->_groups[i] = tmp;
    }

    va_end(arguments);
    return ng;

 err:
    for (i = i-1 ;i >= 0; i -- ) {
        free(ng->_groups[i]);
    }
    
 cleanup:
    free( ng );

    return NULL;
}

AIORET_TYPE DeleteAIOProductGroup( AIOProductGroup *pg )
{
    AIO_ASSERT( pg );
    int i;
    for ( i = 0; i < (int)pg->_num_groups; i ++ ) {
        free( pg->_groups[i] );
    }
    free(pg->_groups);
    free(pg);
    return AIOUSB_SUCCESS;
}


#ifdef __cplusplus
}
#endif


#ifdef SELF_TEST

#include "AIOUSBDevice.h"
#include "gtest/gtest.h"

#include <iostream>
using namespace AIOUSB;

#include <unistd.h>
#include <stdio.h>

TEST(AIOProductRange,NewProductRange ) 
{
    AIOProductRange *tmp = NewAIOProductRange(100,1100);
    ASSERT_TRUE( tmp );

    ASSERT_EQ( 1100, AIOProductRangeEnd(tmp));
    ASSERT_EQ( 100 , AIOProductRangeStart(tmp));
    
    DeleteAIOProductRange( tmp );
}

TEST(AIOProductGroup,NewGroup )
{
    AIOProductRange *first = NewAIOProductRange(100,1100);
    AIOProductRange *second = NewAIOProductRange(1105,1200);
    AIOProductGroup *pg = NewAIOProductGroup( 2, first, second );
    
    ASSERT_TRUE( pg );


    DeleteAIOProductGroup( pg );
}

TEST(AIOProductGroup,NulLGroups )
{
    AIOProductRange *first = NewAIOProductRange(100,1100);
    AIOProductRange *second = NewAIOProductRange(1105,1200);
    AIOProductRange *nullval = NULL;

    AIOProductGroup *pg = NewAIOProductGroup( 3, first, second, nullval);
    
    ASSERT_FALSE( pg );

    ASSERT_DEATH( { DeleteAIOProductGroup(pg); }, "Assertion `pg' failed.");
    DeleteAIOProductRange( first );
    DeleteAIOProductRange( second );
}


int main(int argc, char *argv[] )
{
  
  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();

  return RUN_ALL_TESTS();  

}

#endif
