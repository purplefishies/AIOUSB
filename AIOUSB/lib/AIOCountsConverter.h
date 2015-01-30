/**
 * @file   AIOCountsConverter.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  
 *
 */

#ifndef _AIO_COUNTS_CONVERTER_H
#define _AIO_COUNTS_CONVERTER_H

#include "AIOTypes.h"
#include "AIOContinuousBuffer.h"
#include "AIOFifo.h"


#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef struct {
    double min;
    double max;
} AIOGainRange;

typedef struct aio_counts_converter {
    unsigned num_oversamples;
    unsigned num_channels;
    unsigned unit_size;
    AIOUSB_BOOL discardFirstSample;
    void *buf;
    AIOGainRange *gain_ranges;
    AIORET_TYPE (*Convert)( struct aio_counts_converter *cc, void *tobuf, void *frombuf, unsigned num_bytes );
    AIORET_TYPE (*ConvertFifo)( struct aio_counts_converter *cc, void *tobuf, void *frombuf , unsigned num_bytes );
} AIOCountsConverter;


PUBLIC_EXTERN AIOCountsConverter *NewAIOCountsConverter( void *buf, unsigned num_channels, AIOGainRange *ranges, unsigned num_oversamples,unsigned unit_size  );
PUBLIC_EXTERN AIOCountsConverter *NewAIOCountsConverterFromAIOContinuousBuf( void *buf);
PUBLIC_EXTERN void DeleteAIOCountsConverter( AIOCountsConverter *ccv );
PUBLIC_EXTERN AIORET_TYPE AIOCountsConverterConvertNScans( AIOCountsConverter *ccv, int num_scans );
PUBLIC_EXTERN AIORET_TYPE AIOCountsConverterConvertAllAvailableScans( AIOCountsConverter *ccv );
PUBLIC_EXTERN AIORET_TYPE AIOCountsConverterConvert( AIOCountsConverter *cc, void *tobuf, void *frombuf, unsigned num_bytes );
PUBLIC_EXTERN AIORET_TYPE AIOCountsConverterConvertFifo( AIOCountsConverter *cc, void *tobuf, void *frombuf , unsigned num_bytes );

#ifdef __aiousb_cplusplus
}
#endif


#endif
