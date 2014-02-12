#ifndef _AIOCHANNEL_MASK_H
#define _AIOCHANNEL_MASK_H

#include <AIOChannelMask.h>
#include <AIOTypes.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif


/* typedef unsigned long AIOChannelMask; */
typedef unsigned long aio_channel_obj;
typedef struct {
  unsigned active_signals;
  aio_channel_obj *signals;
  unsigned number_signals;
} AIOChannelMask;


AIOChannelMask *NewAIOChannelMask( unsigned int size );
AIOChannelMask *NewAIOChannelMaskFromStr( const char *bitfields );
const char * AIOChannelMask_GetMask( AIOChannelMask *obj, unsigned index );
unsigned AIOChannelMask_NumberChannels( AIOChannelMask *obj );
unsigned AIOChannelMask_GetMaskWithIndex( AIOChannelMask *obj, unsigned index );



AIORET_TYPE AIOChannelMask_SetMaskFromInt( AIOChannelMask *obj, unsigned field , unsigned index );
AIORET_TYPE AIOChannelMask_SetMask( AIOChannelMask *obj, const char *bitfields );

#ifdef __aiousb_cplusplus
}
#endif

#endif
