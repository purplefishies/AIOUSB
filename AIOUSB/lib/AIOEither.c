/**
 * @file   AIOEither.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief  General structure for AIOUSB Fifo
 *
 */

#include "AIOEither.h"
#include "AIOTypes.h"
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
        free(retval->right.st);
        break;
    case aioeither_value_obj:
        free(retval->right.v);
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
            retval->right.i = t;
            retval->type = aioeither_value_int;
        }
        break;
    case aioeither_value_unsigned:
        {
            unsigned t = *(unsigned*)tmp;
            retval->right.u = t;
            retval->type = aioeither_value_unsigned;
        }
        break;
    case aioeither_value_uint16_t:
        {
            uint16_t t = *(uint16_t*)tmp;
            retval->right.us = t;
            retval->type = aioeither_value_uint16_t;
        }
        break;
    case aioeither_value_double:
        {
            double t = *(double *)tmp;
            retval->right.d = t;
            retval->type = aioeither_value_double;
        }
        break;
    case aioeither_value_string:
        { 
            char *t = *(char **)tmp;
            retval->right.st = strdup(t);
            retval->size     = strlen(t)+1;
            retval->type     = aioeither_value_string;
        }
        break;
    case aioeither_value_obj:
        {    
            va_start(ap, tmp);
            int d = va_arg(ap, int);
            va_end(ap);
            retval->right.v = malloc(d);
            retval->type = aioeither_value_obj;
            retval->size = d;
            memcpy(retval->right.v, tmp, d );
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
            *t = retval->right.i;
        }
        break;
    case aioeither_value_unsigned:
        {
            unsigned *t = (unsigned *)tmp;
            *t = retval->right.u;
        }
        break;
    case aioeither_value_uint16_t:
        {
            uint16_t *t = (uint16_t*)tmp;
            *t = retval->right.us;
        }
        break;
    case aioeither_value_double:
        {
            double *t = (double *)tmp;
            *t = retval->right.d;
        }
        break;
    case aioeither_value_string:
        { 
            memcpy(tmp, retval->right.st, strlen(retval->right.st)+1);
        }
        break;
    case aioeither_value_obj:
        {
            va_start(ap, tmp);
            int d = va_arg(ap, int);
            va_end(ap);
            memcpy(tmp, retval->right.v, d );
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


