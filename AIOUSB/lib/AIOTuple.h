#ifndef _AIOTUPLE_H
#define _AIOTUPLE_H

#include "AIOTypes.h"
#include "StringArray.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

#ifdef __cplusplus
#define AIOTUPLE2_TYPE( NAME, T1, T2)                            \
    typedef struct NAME {                                        \
        T1 _1;                                                   \
        T2 _2;                                                   \
        NAME(T1 _t1, T2 _t2 ) : _1(_t1), _2(_t2) {};             \
        T1 get_1( NAME *obj) { return obj->_1 ; };               \
        T2 get_2( NAME *obj) { return obj->_2 ; };               \
    } NAME;                                                      

#define AIO_CHAR_ARRAY(N , ... ) new char *[N]{ __VA_ARGS__ }
#else
#define AIOTUPLE2_TYPE( NAME, T1, T2)                        \
    typedef struct NAME {                                    \
        T1 _1;                                               \
        T2 _2;                                               \
    } NAME;                                                  \
    T2 NAME ## get_2( NAME *obj ) { return obj->_2 ; };      \
    T1 NAME ## get_1( NAME *obj ) { return obj->_1 ; };
    

#define AIO_CHAR_ARRAY(N , ... ) (char **)&(char *[N]){ __VA_ARGS__ }
#endif

#define AIOTUPLE2_PTR( NAME, T1, T2 )  NAME *
#define AIOTUPLE2(NAME, T1, T2 ) NAME 


AIOTUPLE2_TYPE(AIOTuple2_AIORET_TYPE__char_p_p, AIORET_TYPE, char ** );
AIOTUPLE2_TYPE(AIOTuple2_AIORET_TYPE__StringArray, AIORET_TYPE, StringArray );



#ifdef __aiousb_cplusplus
}
#endif



#endif
