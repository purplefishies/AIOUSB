#ifndef _STRINGARRAY_H
#define _STRINGARRAY_H

#include "AIOTypes.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef struct StringArray {
    int _size;
    char ** _strings;
#ifdef __cplusplus
    StringArray(int size, ... );
    ~StringArray();
#endif
} StringArray;

#ifdef __cplusplus
#define STRING_ARRAY(N, ... ) 
#else
#define STRING_ARRAY(N, ... ) 
#endif

/* BEGIN AIOUSB_API */
PUBLIC_EXTERN StringArray *NewStringArray(size_t numstrings );
PUBLIC_EXTERN StringArray *NewStringArrayWithStrings(size_t numstrings, ... );
PUBLIC_EXTERN AIORET_TYPE DeleteStringArray(StringArray *str);
PUBLIC_EXTERN StringArray *CopyStringArray( StringArray *str );
PUBLIC_EXTERN char *StringArrayToString( StringArray *str );
PUBLIC_EXTERN char *StringArrayToStringWithDelimeter( StringArray *str, const char *delim);
/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif


#endif
