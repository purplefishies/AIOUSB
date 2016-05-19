#ifndef _LIST_H
#define _LIST_H

#include "StringArray.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


#define TAIL_Q_LIST( TYPE , PRETTYNAME )                                          \
    typedef struct TailQListEntry ## PRETTYNAME {                                 \
        TYPE _value;                                                              \
        TAILQ_ENTRY(TailQListEntry ## PRETTYNAME) entries;                        \
    } TailQListEntry ##PRETTYNAME;                                                \
                                                                                  \
    typedef struct TailQList ## PRETTYNAME {                                      \
        struct TailQListEntry ## PRETTYNAME _list;                                \
        /* New stuff */                                                           \
        TAILQ_HEAD( tailhead ##PRETTYNAME, TailQListEntry ## PRETTYNAME ) head;   \
        struct tailhead ##PRETTYNAME  *headp;                                     \
    } TailQList ##PRETTYNAME;                                                     \

#ifdef DEBUG_NL
#define __NL__ __NL__
#else
#define __NL__
#endif

#define TAIL_Q_LIST_IMPLEMENTATION( TYPE, PRETTYNAME )                                                \
__NL__        TailQList ## PRETTYNAME  *NewTailQList ## PRETTYNAME() {                                \
__NL__            TailQList ## PRETTYNAME  *tmp=                                                      \
__NL__                (TailQList ## PRETTYNAME *)calloc(1,sizeof(TailQList ## PRETTYNAME ));          \
__NL__            TAILQ_INIT( &(tmp->head) );                                                         \
__NL__            return tmp;                                                                         \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        TailQListEntry ## PRETTYNAME *NewTailQListEntry ## PRETTYNAME( TYPE value )             \
__NL__        {                                                                                       \
__NL__            TailQListEntry ## PRETTYNAME *tmp =                                                 \
__NL__                (TailQListEntry ## PRETTYNAME *)calloc(1,sizeof(TailQListEntry ## PRETTYNAME)); \
__NL__            if ( !tmp ) return tmp;                                                             \
__NL__            tmp->_value = value;                                                                \
__NL__            return tmp;                                                                         \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        AIORET_TYPE DeleteTailQListEntry ## PRETTYNAME( TailQListEntry ## PRETTYNAME *entry )   \
__NL__        {                                                                                       \
__NL__            AIO_ASSERT( entry );                                                                \
__NL__            Delete ## PRETTYNAME (entry->_value);                                               \
__NL__            free(entry);                                                                        \
__NL__            return AIOUSB_SUCCESS;                                                              \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        char *TailQListEntry ## PRETTYNAME ## ToString( TailQListEntry ## PRETTYNAME *entry )   \
__NL__        {                                                                                       \
__NL__            char *tmpstr = 0;                                                                   \
__NL__            char *tmp;                                                                          \
__NL__            asprintf(&tmpstr, "%s", (tmp= PRETTYNAME ## ToString(entry->_value)) );             \
__NL__            free(tmp);                                                                          \
__NL__            return tmpstr;                                                                      \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        char *TailQList ## PRETTYNAME ## ToString( TailQList ## PRETTYNAME  *list ) {           \
__NL__            TailQListEntry ## PRETTYNAME *np;                                                   \
__NL__            char start[] = "";                                                                  \
__NL__            char *tmp = start;                                                                  \
__NL__            int started = 1;                                                                    \
__NL__            char *keep = (char *)0;                                                             \
__NL__            for (np = list->head.tqh_first; np != NULL; np = np->entries.tqe_next) {            \
__NL__                char *_t1;                                                                      \
__NL__                asprintf(&keep, "%s%s", tmp, (_t1 = TailQListEntry ## PRETTYNAME ## ToString( np )) );         \
__NL__                free(_t1);                                                                      \
__NL__                if ( started != 1 )                                                             \
__NL__                    free(tmp);                                                                  \
__NL__                tmp = strdup(keep);                                                             \
__NL__                free(keep);                                                                     \
__NL__                if ( np->entries.tqe_next ) {                                                   \
__NL__                    asprintf(&keep, "%s,", tmp );                                               \
__NL__                    free(tmp);                                                                  \
__NL__                    tmp = strdup(keep);                                                         \
__NL__                    free(keep);                                                                 \
__NL__                }                                                                               \
__NL__                started = 0;                                                                    \
__NL__            }                                                                                   \
__NL__            return tmp;                                                                         \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        AIORET_TYPE DeleteTailQList ## PRETTYNAME ( TailQList ## PRETTYNAME  *list ) {          \
__NL__            AIO_ASSERT(list);                                                                   \
__NL__            TailQListEntry ## PRETTYNAME *tmp;                                                  \
__NL__            while ( list->head.tqh_first != NULL ) {                                            \
__NL__                tmp = list->head.tqh_first;                                                     \
__NL__                TAILQ_REMOVE( &list->head, list->head.tqh_first, entries );                     \
__NL__                DeleteTailQListEntry ## PRETTYNAME ( tmp );                                     \
__NL__            }                                                                                   \
__NL__            free(list);                                                                         \
__NL__            return AIOUSB_SUCCESS;                                                              \
__NL__        }                                                                                       \
__NL__                                                                                                \
__NL__        AIORET_TYPE TailQList ## PRETTYNAME ## Insert( TailQList ## PRETTYNAME  *list,          \
__NL__                                                    TailQListEntry ## PRETTYNAME *nnode ) {     \
__NL__            AIO_ASSERT( list ); AIO_ASSERT( nnode );                                            \
__NL__            TAILQ_INSERT_TAIL( &list->head, nnode, entries );                                   \
__NL__            return AIOUSB_SUCCESS;                                                              \
__NL__        }

TAIL_Q_LIST(int, int );
TAIL_Q_LIST(StringArray *, StringArray_p );


#ifdef __aiousb_cplusplus
}
#endif

#endif


