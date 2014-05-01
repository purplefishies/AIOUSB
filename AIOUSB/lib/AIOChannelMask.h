#ifndef _AIOCHANNEL_MASK_H
#define _AIOCHANNEL_MASK_H

#include "AIOTypes.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>


#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef unsigned long aio_channel_obj;
typedef struct {
  int *signal_indices;
  int signal_index;
  unsigned active_signals;
  aio_channel_obj *signals;
  unsigned number_signals;
  unsigned pos;
} AIOChannelMask;


AIOChannelMask *NewAIOChannelMask( unsigned int size );
AIOChannelMask *NewAIOChannelMaskFromStr( const char *bitfields );
void  DeleteAIOChannelMask( AIOChannelMask *mask );

const char * AIOChannelMask_GetMask( AIOChannelMask *mask, unsigned index );
AIORET_TYPE AIOChannelMask_NumberChannels( AIOChannelMask *mask );
AIORET_TYPE AIOChannelMask_GetMaskWithIndex( AIOChannelMask *mask, unsigned index );
int AIOChannelMask_Indices( AIOChannelMask *mask );
int AIOChannelMask_NextIndex( AIOChannelMask *mask );

AIORET_TYPE AIOChannelMask_SetMaskFromInt( AIOChannelMask *mask, unsigned field , unsigned index );
AIORET_TYPE AIOChannelMask_SetMaskFromStr( AIOChannelMask *mask, const char *bitfields );
AIORET_TYPE AIOChannelMask_SetMaskAuto( AIOChannelMask *mask );

#define BIT_LENGTH(x) ( sizeof(x) * 8 )

#ifdef __aiousb_cplusplus
}
#endif

#endif
