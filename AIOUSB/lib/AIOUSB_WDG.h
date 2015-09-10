#ifndef AIOUSB_WDG_H
#define AIOUSB_WDG_H

#include "aiousb.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

CREATE_ENUM_W_START( WDGVals, 0x00,
                     AIOUSB_WDG_READ_VALUE,
                     AIOUSB_WDG_READ_INDEX = 0x0041
                     )

typedef struct {
  int bufsize;
  unsigned long L;
  unsigned char *wdgbuf;
  unsigned long timeout;
} AIOWDGConfig ;

/* BEGIN AIOUSB_API */
AIOWDGConfig *NewWDGConfig(void);
void DeleteWDGConfig( AIOWDGConfig *obj);
AIORET_TYPE WDG_SetConfig( unsigned long DeviceIndex, AIOWDGConfig *obj );
AIORET_TYPE WDG_GetStatus( unsigned long DeviceIndex, AIOWDGConfig *obj );
AIORET_TYPE WDG_Pet( unsigned long DeviceIndex, AIOWDGConfig *obj );
/* END AIOUSB_API */

#ifdef __aiousb_cplusplus
}
#endif

#endif
