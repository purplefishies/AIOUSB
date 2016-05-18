#include "StringArray.h"
#include "AIOTypes.h"
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif


/*----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------*/
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


/*----------------------------------------------------------------------------*/

#ifdef __cplusplus


StringArray::StringArray(int size, ... ) : _size(size) 
{
     this->_strings = new char *[size];
     va_list arguments;
     va_start( arguments, size );
     for ( int i = 0; i < (int)size; i ++ ) { 
         char *tmp = va_arg(arguments, char * );
         this->_strings[i] = strdup(tmp);
     }
     va_end(arguments);
}

StringArray::~StringArray() { 
    for ( int i = 0; i < this->_size; i ++ ) {
        free(this->_strings[i] );
    }
    delete [] this->_strings;
}

StringArray::StringArray(const StringArray &ref)
{
    _size = ref._size;
    _strings = new char *[_size] ;
    for ( int i = 0; i < _size ;i ++ ) { 
        _strings[i] = strdup( ref._strings[i] );
    }
}

#endif

/*----------------------------------------------------------------------------*/
StringArray *CopyStringArray( StringArray *str )
{
    AIO_ASSERT_RET( NULL, str );
    StringArray *tmp = NewStringArray( str->_size );
    if ( !tmp ) return tmp;
    memcpy(tmp->_strings, str->_strings, sizeof(char *)*str->_size );
    return tmp;
}

/*----------------------------------------------------------------------------*/
char *StringArrayToString( StringArray *str )
{
    return StringArrayToStringWithDelimeter( str, NULL);
}

/*----------------------------------------------------------------------------*/
char *StringArrayToStringWithDelimeter( StringArray *str, const char *delim)
{

    AIO_ASSERT_RET( NULL, str );
    char *tmpdelim  = ((char *)delim == NULL ? (char *)" " : (char *)delim );
    int i;
    char *retval = NULL;
    char *tmp;
    for ( i = 0; i < str->_size -1 ; i ++ ) { 
        if ( i == 0 && retval == NULL ) { 
            asprintf(&retval, "%s%s", str->_strings[i], tmpdelim );
        } else if ( i != 0 && retval == NULL ) {
            asprintf(&retval, "%s%s", str->_strings[i], tmpdelim );
        } else {
            tmp = strdup(retval );
            free(retval);
            asprintf(&retval, "%s%s%s", tmp, str->_strings[i], tmpdelim );
            free(tmp);
        }
    }
    tmp = strdup(retval);
    free(retval);
    asprintf(&retval, "%s%s", tmp, str->_strings[i]);
    free(tmp);
    return retval;
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


TEST(StringArray,Stringification)
{
    StringArray tmp = StringArray(4,(char*)"1",(char*)"2",(char*)"3",(char *)"4" );
    char *tmpstr = StringArrayToString( &tmp );
    ASSERT_STREQ( "1 2 3 4", tmpstr );
    free(tmpstr);
    tmpstr = StringArrayToStringWithDelimeter( &tmp, (const char *)"," );
    ASSERT_STREQ( "1,2,3,4", tmpstr );
    free(tmpstr);
}

TEST(StringArray,CopyXtor)
{
    StringArray tmp1(4,(char*)"1",(char*)"2",(char*)"3",(char *)"4" );
    StringArray tmp2(tmp1);

    for ( int i = 0; i < tmp1._size ; i ++ ) 
        ASSERT_STREQ( tmp1._strings[i], tmp2._strings[i] );
    
    StringArray *p1 = new StringArray(tmp2);

    for ( int i = 0; i < p1->_size ; i ++ ) 
        ASSERT_STREQ( p1->_strings[i], tmp2._strings[i] );
    
    delete p1;
}

int main(int argc, char *argv[] )
{
  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();

  return RUN_ALL_TESTS();  

}
#endif
