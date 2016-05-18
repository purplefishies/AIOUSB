#include "AIOTuple.h"
#include <stdarg.h>
#include <string.h>


#ifdef __cplusplus
namespace AIOUSB {
#endif



#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#include "gtest/gtest.h"
#include <iostream>

using namespace AIOUSB;


TEST(Tuple,Basic2)
{
    AIOTUPLE2( AIOTuple2_AIORET_TYPE__StringArray, AIORET_TYPE, StringArray ) bar( AIOUSB_ERROR_INVALID_DATA, StringArray(3 , (char *)"Hello",(char *)"There",(char*)"" ));
    ASSERT_STREQ( "Hello", bar._2._strings[0] );
    ASSERT_STREQ( "There", bar._2._strings[1] );

    ASSERT_EQ( AIOUSB_ERROR_INVALID_DATA, bar._1 );
}

TEST(Tuple,StringArray)
{
    
    AIOTuple2_AIORET_TYPE__StringArray *tmp;
    StringArray t1(3 , (char *)"Hello",(char *)"There",(char*)"Blah" );
    AIOTuple2_AIORET_TYPE__StringArray bar( AIOUSB_ERROR_INVALID_DATA, t1 );

    ASSERT_EQ(1,1);
}



int main(int argc, char *argv[] )
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();

    return RUN_ALL_TESTS();  

}
#endif
