#ifndef _CHANNEL_RANGE_H
#define _CHANNEL_RANGE_H

#include "AIOTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

struct ad_gain_pairs {
    ADGainCode gain;
    const char *name;
};

struct ad_gain_pairs AD_GAIN_CODE_STRINGS[] = {
    {AD_GAIN_CODE_0_10V, "0-10"},
    {AD_GAIN_CODE_10V, "+-10"},
    {AD_GAIN_CODE_0_5V,"0-5"},
    {AD_GAIN_CODE_5V,"+-5"},
    {AD_GAIN_CODE_0_2V,"0-2"},
    {AD_GAIN_CODE_2V,"+-2"},
    {AD_GAIN_CODE_0_1V,"0-1"},
    {AD_GAIN_CODE_1V,"+-1"},
    {ADGainCode_end,0}
};

#define LENGTH_AD_GAIN_CODE_STRINGS ((int)( sizeof(AD_GAIN_CODE_STRINGS)/sizeof(struct ad_gain_pairs) - 1 ))


extern int aio_channel_range_error;

typedef struct aio_channel_range {
    int start;
    int end;
    ADGainCode gain;
} AIOChannelRange;

AIOChannelRange *NewAIOChannelRangeFromStr( const char *str );
void DeleteAIOChannelRange( AIOChannelRange *range );
char *AIOChannelRangeToStr( AIOChannelRange *range );
AIORET_TYPE AIOChannelRangeGetStart( AIOChannelRange *range );
AIORET_TYPE AIOChannelRangeGetEnd( AIOChannelRange *range );
AIORET_TYPE AIOChannelRangeGetGain( AIOChannelRange *range );

#ifdef __aiousb_cplusplus
}
#endif

#endif
