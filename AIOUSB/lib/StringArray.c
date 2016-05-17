#include "StringArray.h"
#include "AIOTypes.h"
#include <stdarg.h>
#include <string.h>


#ifdef __cplusplus
namespace AIOUSB {
#endif

StringArray *NewStringArrayWithStrings(size_t numstrings, ... )
{
    StringArray *tmpsa = NewStringArray( numstrings );
    if ( !tmpsa )return tmpsa;

    va_list arguments;
    va_start( arguments, numstrings );
    for ( int i = 0; i < (int)numstrings; i ++ ) { 
        char *tmp = va_arg(arguments, char * );
        tmpsa->_strings[i] = strdup(tmp);
    }
    va_end(arguments);
    return tmpsa;
}

StringArray *NewStringArray(size_t numstrings)
{
    if ( !numstrings ) return NULL;
    StringArray *tmp = (StringArray *)malloc(sizeof(StringArray));
    if (!tmp) return NULL;
    tmp->_size = numstrings;
    tmp->_strings = (char **)malloc(sizeof(char *)*numstrings);
    if (!tmp->_strings )
        goto err;

    return tmp;
        
 err:
    free(tmp );

    return NULL;
}

AIORET_TYPE DeleteStringArray( StringArray *str)
{
    AIO_ASSERT( str );
    for ( int i = 0; i < str->_size; i ++ ) {
        free( str->_strings[i] );
    }
    free(str->_strings);
    free(str);
    return AIOUSB_SUCCESS;
}

StringArray *CopyStringArray( StringArray *str )
{
    AIO_ASSERT_RET( NULL, str );
    StringArray *tmp = NewStringArray( str->_size );
    if ( !tmp ) return tmp;
    memcpy(tmp->_strings, str->_strings, sizeof(char *)*str->_size );
    return tmp;
}

char *StringArrayToString( StringArray *str )
{

    return NULL;
}

char *StringArrayToStringWithDelimeter( StringArray *str )
{


    return NULL;
}



#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#include <gtest/gtest.h>
#include <iostream>
using namespace AIOUSB;

#include <unistd.h>
#include <stdio.h>


TEST(StringArray,Basics ) 
{
    StringArray *tmp = NewStringArrayWithStrings(4,(char*)"First",(char*)"Second",(char*)"Third",(char *)"Fourth");
    ASSERT_TRUE( tmp );
    DeleteStringArray( tmp );
}

int main(int argc, char *argv[] )
{
  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();

  return RUN_ALL_TESTS();  

}
#endif
