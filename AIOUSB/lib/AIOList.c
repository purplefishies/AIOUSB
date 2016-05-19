#include "AIOTypes.h"
#include <sys/queue.h>
#include <stdio.h>
#include <string.h>
#include "AIOList.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

char *intToString( int val )
{
    char *tmp = 0;
    asprintf(&tmp,"%d",val );
    return tmp;
}
/**
 * Dummy function to make tail q list work
 */
AIORET_TYPE Deleteint( int val ) {
    return AIOUSB_SUCCESS;
}
 
TAIL_Q_LIST_IMPLEMENTATION( StringArray *, StringArray_p );
TAIL_Q_LIST_IMPLEMENTATION( int, int );


#ifdef __cplusplus
}
#endif




#ifdef SELF_TEST

#include "gtest/gtest.h"
#include "AIOList.h"
#include <stdio.h>
#include <iostream>
using namespace AIOUSB;


TEST(QList,test)
{
    TailQListint *tmp = NewTailQListint();
    char *hold;

    TailQListintInsert( tmp, NewTailQListEntryint( 42 )  );
    TailQListintInsert( tmp, NewTailQListEntryint( 43 )  );
    TailQListintInsert( tmp, NewTailQListEntryint( 44 )  );
    ASSERT_STREQ((hold=TailQListintToString( tmp )), "42,43,44" ); 
    free(hold);

    DeleteTailQListint( tmp );
}

TEST(Qlist,StringArray)
{
    TailQListStringArray_p *tmp = NewTailQListStringArray_p();
    char *hold;

    TailQListStringArray_pInsert(tmp,NewTailQListEntryStringArray_p(NewStringArrayWithStrings(3,(char *)"This",(char *)"is",(char *)"a string")));
    TailQListStringArray_pInsert(tmp,NewTailQListEntryStringArray_p(NewStringArrayWithStrings(3,(char *)"and", (char *)"another",(char *)"string")));
    ASSERT_STREQ((hold=TailQListStringArray_pToString( tmp )), "This is a string,and another string" ); 
    free(hold);

    DeleteTailQListStringArray_p( tmp );
}

int 
main(int argc, char *argv[] ) 
{
  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
  return RUN_ALL_TESTS();
}



#endif
