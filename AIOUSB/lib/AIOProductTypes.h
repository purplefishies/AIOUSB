
#ifndef _AIOPRODUCT_TYPES_H
#define _AIOPRODUCT_TYPES_H

#include "AIOTypes.h"


#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


/**
 * @brief A simplified range of Products based off of device ids
 *
 */
typedef struct AIOProductRange  {
    unsigned long _start;
    unsigned long _end;
#ifdef __cplusplus
AIOProductRange( unsigned long start, unsigned long end ) : _start(start), _end(end) {};
#endif
} AIOProductRange;


/**
 * @brief A smart product group that marks a range of ACCES I/O Products
 *
 */
typedef struct AIOProductGroup {
#ifndef SWIG
#ifdef __cplusplus
    AIOProductGroup( size_t numgroups, AIOProductRange **groups ) : _num_groups(numgroups), _groups(groups) {};
    AIOProductGroup( size_t numgroups, ... );
    ~AIOProductGroup();
#endif
#endif
    size_t _num_groups;
    AIOProductRange **_groups;
} AIOProductGroup;

/* BEGIN AIOUSB_API */
PUBLIC_EXTERN AIOProductRange *NewAIOProductRange( unsigned long start, unsigned long end);
PUBLIC_EXTERN AIORET_TYPE DeleteAIOProductRange( AIOProductRange *pr );
PUBLIC_EXTERN AIORET_TYPE AIOProductRangeStart( const AIOProductRange *pr );
PUBLIC_EXTERN AIORET_TYPE AIOProductRangeEnd( const AIOProductRange *pr );

PUBLIC_EXTERN AIOProductGroup *NewAIOProductGroup(size_t numgroups, ...  );
PUBLIC_EXTERN AIORET_TYPE DeleteAIOProductGroup(AIOProductGroup *);
PUBLIC_EXTERN AIORET_TYPE AIOProductGroupContains( const AIOProductGroup *g, unsigned long val );

/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif


#endif
