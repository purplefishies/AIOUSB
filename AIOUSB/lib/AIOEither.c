/**
 * @file   AIOEither.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief  General structure for AIOUSB Fifo
 *
 */


#include "AIOTypes.h"
#include "AIOEither.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
namespace AIOUSB 
{
#endif

#define LOOKUP(T) aioret_value_ ## T 

void AIOEitherClear( AIOEither *retval )
{
    assert(retval);
    switch(retval->type) { 
    case aioeither_value_string:
        free(retval->right.object);
        break;
    case aioeither_value_obj:
        free(retval->right.object);
        break;
    default:
        ;
    }
    if ( retval->errmsg ) {
        free(retval->errmsg );
        retval->errmsg = 0;
    }

}
    
void AIOEitherSetRight(AIOEither *retval, AIO_EITHER_TYPE val , void *tmp, ... )
{
    assert(retval);
    va_list ap;
    switch(val) { 
    case aioeither_value_int:
        {
            int t = *(int *)tmp;
            *(int *)(&retval->right.number) = t;
            retval->type = aioeither_value_int;
        }
        break;
    case aioeither_value_unsigned:
        {
            unsigned t = *(unsigned*)tmp;
            *(unsigned *)(&retval->right.number) = t;
            retval->type = aioeither_value_unsigned;
        }
        break;
    case aioeither_value_uint16_t:
        {
            uint16_t t = *(uint16_t*)tmp;
            *(uint16_t *)&retval->right.number = t;
            retval->type = aioeither_value_uint16_t;
        }
        break;
    case aioeither_value_double:
        {
            double t = *(double *)tmp;
            *(double *)&retval->right.number = t;
            retval->type = aioeither_value_double;
        }
        break;
    case aioeither_value_longdouble_t:
         {
            long double t = *(long double *)tmp;
            *(long double *)&retval->right.number = t;
            retval->type = aioeither_value_longdouble_t;
        }
         break;
    case aioeither_value_string:
        { 
            char *t = *(char **)tmp;
            retval->right.object = strdup(t);
            retval->size     = strlen(t)+1;
            retval->type     = aioeither_value_string;
        }
        break;
    case aioeither_value_obj:
        {    
            va_start(ap, tmp);
            int d = va_arg(ap, int);
            va_end(ap);
            retval->right.object = malloc(d);
            retval->type = aioeither_value_obj;
            retval->size = d;
            memcpy(retval->right.object, tmp, d );
        }
        break;
    default:
        break;
    }
}

void AIOEitherGetRight(AIOEither *retval, void *tmp, ... )
{
    assert(retval);
    va_list ap;
    switch(retval->type) { 
    case aioeither_value_int:
        {
            int *t = (int *)tmp;
            *t = *(int*)(&retval->right.number);
        }
        break;
    case aioeither_value_unsigned:
        {
            unsigned *t = (unsigned *)tmp;
            *t = *(unsigned*)(&retval->right.number);
        }
        break;
    case aioeither_value_uint16_t:
        {
            uint16_t *t = (uint16_t*)tmp;
            *t = *(uint16_t*)(&retval->right.number);
        }
        break;
    case aioeither_value_double_t:
        {
            double *t = (double *)tmp;
            *t = *(double*)(&retval->right.number);
        }
        break;
    case aioeither_value_longdouble_t:
        {
            long double *t = (long double *)tmp;
            *t = *(long double*)(&retval->right.number);
        }
        break;
    case aioeither_value_string:
        { 
            memcpy(tmp, retval->right.object, strlen((char *)retval->right.object)+1);
        }
        break;
    case aioeither_value_obj:
        {
            va_start(ap, tmp);
            int d = va_arg(ap, int);
            va_end(ap);
            memcpy(tmp, retval->right.object, d );
        }
        break;
    default:
        break;
    }

}

void AIOEitherSetLeft(AIOEither *retval, int val)
{
    assert(retval);
    retval->left = val;
}

int AIOEitherGetLeft(AIOEither *retval)
{
    return retval->left;
}

AIOUSB_BOOL AIOEitherHasError( AIOEither *retval )
{
    return (retval->left == 0 ? AIOUSB_FALSE : AIOUSB_TRUE );
}

char *AIOEitherToString( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET( NULL, result );


    return (char *)&(retval->right.object);

}

int AIOEitherToInt( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET(0xffffffff,result );
    return *(int *)&(retval->right.number);
}

short AIOEitherToShort( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET(0xffff,result );
    return *(short *)&(retval->right.number);
}

unsigned AIOEitherToUnsigned( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET(0xffffffff, result );
    return *(unsigned *)&(retval->right.number);
}

double AIOEitherToDouble( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET(0xffffffff, result );
    return *(double *)&(retval->right.number);
}

AIO_NUMBER AIOEitherToAIONumber( AIOEither *retval, AIORET_TYPE *result )
{
    AIO_ASSERT_RET(0xffffffffffffffff, result );
    return *(AIO_NUMBER *)&(retval->right.number);
}



#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#include "AIOUSBDevice.h"
#include "AIOEither.h"
#include "gtest/gtest.h"
#include "tap.h"
#include <iostream>
using namespace AIOUSB;

struct testobj {
    int a;
    int b;
    int c;
};

TEST(AIOEitherTest,BasicAssignments)
{
    AIOEither a = {0};
    int tv_int = 22;

    uint32_t tv_uint = 23;
    double tv_double = 3.14159;
    long double tv_ld = 2323244234234.3434;
    char *tv_str = (char *)"A String";
    char readvals[100];
    struct testobj tv_obj = {1,2,3};

    AIOEitherSetRight( &a, aioeither_value_int32_t  , &tv_int );
    AIOEitherGetRight( &a, readvals );
    EXPECT_EQ( tv_int,  *(int*)&readvals[0] );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_uint32_t , &tv_uint );
    AIOEitherGetRight( &a, readvals );
    EXPECT_EQ( tv_uint, *(uint32_t*)&readvals[0] );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_double_t , &tv_double );
    AIOEitherGetRight( &a, readvals );
    EXPECT_EQ( tv_double, *(double*)&readvals[0] );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_longdouble_t , &tv_ld );
    AIOEitherGetRight( &a, readvals );
    EXPECT_EQ( tv_ld, *(long double*)&readvals[0] );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_string, &tv_str );
    AIOEitherGetRight( &a, readvals );
    EXPECT_STREQ( tv_str, readvals );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_string, &tv_str );
    AIOEitherGetRight( &a, readvals );
    EXPECT_STREQ( tv_str, readvals );
    AIOEitherClear( &a );


    AIOEitherSetRight( &a, aioeither_value_obj, &tv_obj , sizeof(struct testobj));
    AIOEitherGetRight( &a, readvals, sizeof(struct testobj));
    EXPECT_EQ( ((struct testobj*)&readvals)->a, tv_obj.a );
    EXPECT_EQ( ((struct testobj*)&readvals)->b, tv_obj.b );
    EXPECT_EQ( ((struct testobj*)&readvals)->c, tv_obj.c );
    AIOEitherClear( &a );

    AIOEitherSetRight( &a, aioeither_value_obj, &tv_obj , sizeof(struct testobj));
    AIOEitherGetRight( &a, readvals, sizeof(struct testobj));
    EXPECT_EQ( ((struct testobj*)&readvals)->a, tv_obj.a );
    EXPECT_EQ( ((struct testobj*)&readvals)->b, tv_obj.b );
    EXPECT_EQ( ((struct testobj*)&readvals)->c, tv_obj.c );
    AIOEitherClear( &a );

}

TEST(CanCreate,Shorts)
{
    AIOEither a = {0};
    uint16_t tv_ui = 33;
    char readvals[100];

    AIOEitherSetRight( &a, aioeither_value_uint16_t, &tv_ui );
    AIOEitherGetRight( &a, readvals, sizeof(struct testobj));

    EXPECT_EQ( a.type, aioeither_value_uint16_t );
    EXPECT_EQ( tv_ui, *(uint16_t*)&readvals[0] );

}

typedef struct simple {
    char *tmp;
    int a;
    double b;
} Foo;


AIOEither doIt( Foo *) 
{
    AIOEither retval = {0};
    asprintf(&retval.errmsg, "Error got issue with %d\n", 3 );
    retval.left = 3;
    return retval;
}

TEST(CanCreate,Simple)
{
    Foo tmp = {NULL, 3,34.33 };
    AIOEither retval = doIt( &tmp );
    
    EXPECT_EQ( 3,  AIOEitherGetLeft( &retval ) );
    AIOEitherClear( &retval );

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


