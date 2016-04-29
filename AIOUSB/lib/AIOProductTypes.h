
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
} AIOProductRange;


/**
 * @brief A smart product group that marks a range of ACCES I/O Products
 *
 */
typedef struct AIOProductGroup {
    AIOProductRange **_groups;
    size_t _num_groups;
} AIOProductGroup;

/* BEGIN AIOUSB_API */
PUBLIC_EXTERN AIOProductRange *NewAIOProductRange( unsigned long start, unsigned long end);
PUBLIC_EXTERN AIORET_TYPE DeleteAIOProductRange( AIOProductRange *pr );
PUBLIC_EXTERN AIORET_TYPE AIOProductRangeStart( AIOProductRange *pr );
PUBLIC_EXTERN AIORET_TYPE AIOProductRangeEnd( AIOProductRange *pr );

PUBLIC_EXTERN AIOProductGroup *NewAIOProductGroup(size_t numgroups, ...  );
PUBLIC_EXTERN AIORET_TYPE DeleteAIOProductGroup(AIOProductGroup *);


/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif


#endif
