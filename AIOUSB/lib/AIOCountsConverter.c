/**
 * @file   AIOCountsConverter.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  General header files for the AIOUSB library
 *
 */

#include "AIOTypes.h"
#include "AIOUSB_Core.h"
#include "AIOCountsConverter.h"
#include <pthread.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif 

/*----------------------------------------------------------------------------*/
AIOCountsConverter *NewAIOCountsConverterWithBuffer( void *buf, 
                                                     unsigned num_channels, 
                                                     AIOGainRange *ranges, 
                                                     unsigned num_oversamples, 
                                                     unsigned unit_size  
                                                     ) 
{
    AIOCountsConverter *tmp = NewAIOCountsConverter( num_channels, ranges, num_oversamples , unit_size );
    if (!tmp ) 
        return NULL;

    tmp->buf              = buf;

    return tmp;
}

/*----------------------------------------------------------------------------*/
AIOCountsConverter *NewAIOCountsConverter( unsigned num_channels, AIOGainRange *ranges, unsigned num_oversamples, unsigned unit_size  ) {
    AIOCountsConverter *tmp = (AIOCountsConverter *)malloc( sizeof(struct aio_counts_converter) );
    if (!tmp ) 
        return NULL;
    tmp->num_oversamples  = num_oversamples;
    tmp->num_channels     = num_channels;
    tmp->gain_ranges      = ranges; /* Gain ranges should be same length as num_channels */
    tmp->unit_size        = unit_size;
    tmp->Convert          = AIOCountsConverterConvert;
    tmp->ConvertFifo      = AIOCountsConverterConvertFifo;
    return tmp;
}

/*----------------------------------------------------------------------------*/
void DeleteAIOCountsConverter( AIOCountsConverter *ccv )
{
    free(ccv);
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOCountsConverterConvertNScans( AIOCountsConverter *ccv, int num_scans )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOCountsConverterConvertAllAvailableScans( AIOCountsConverter *ccv )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;
}

/*----------------------------------------------------------------------------*/
double Convert( AIOGainRange range, unsigned short sum )
{
    return ((double)(range.max - range.min)*sum )/ ((( unsigned short )-1)+1) + range.min;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOCountsConverterConvertFifo( AIOCountsConverter *cc, void *tobufptr, void *frombufptr , unsigned num_bytes )
{
    AIOFifoVolts *tofifo     = (AIOFifoVolts*)tobufptr;
    AIOFifoCounts *fromfifo  = (AIOFifoCounts*)frombufptr;
    int allowed_scans = num_bytes / ((cc->num_oversamples+1) * cc->num_channels * cc->unit_size );
    AIORET_TYPE count = 0;
    double tmpvolt;
    int pos;
    unsigned short *tmpbuf = (unsigned short *)malloc( allowed_scans * cc->num_channels * cc->unit_size * (1+cc->num_oversamples));

    fromfifo->PopN( fromfifo, tmpbuf, allowed_scans*cc->num_channels*(1+cc->num_oversamples));

    for ( int scan_count = 0, tobuf_pos = 0; scan_count < allowed_scans ; scan_count ++ ) {
        for ( unsigned ch = 0; ch < cc->num_channels; ch ++ , tobuf_pos ++ ) { 
            unsigned sum = 0;
            for ( unsigned os = 0; os < cc->num_oversamples + 1; os ++ ) {
                pos = (scan_count *(cc->num_channels)*(cc->num_oversamples + 1)) + 
                    ch * ( cc->num_oversamples + 1) + os;
                sum += tmpbuf[pos];
                count += sizeof(unsigned short);
            }
            sum /= (cc->num_oversamples + 1);

            tmpvolt = (double)Convert( cc->gain_ranges[ch], sum );
            tofifo->Push( tofifo, tmpvolt );
        }
    }

    free(tmpbuf);
    return count;

}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOCountsConverterConvert( AIOCountsConverter *cc, void *to_buf, void *from_buf, unsigned num_bytes )
{
    int allowed_scans = num_bytes / (cc->num_oversamples * cc->num_channels * cc->unit_size );
    AIORET_TYPE count = 0;

    for ( int scan_count = 0, tobuf_pos = 0; scan_count < allowed_scans ; scan_count ++ ) {
        for ( unsigned ch = 0; ch < cc->num_channels; ch ++ , tobuf_pos ++ ) { 
            unsigned sum = 0;
            for ( unsigned os = 0; os < cc->num_oversamples + 1; os ++ ) {
                sum += ((unsigned short *)from_buf)[ (scan_count*cc->num_channels) + ch + os ];
                count += sizeof(unsigned short);
            }
            sum /= (cc->num_oversamples + 1);
            ((double *)to_buf)[tobuf_pos] = Convert( cc->gain_ranges[ch], sum );
        }
    }

    return count;
}

/*----------------------------------------------------------------------------*/

PUBLIC_EXTERN AIOGainRange* NewAIOGainRangeFromADCConfigBlock( ADCConfigBlock *adc )
{
    AIOGainRange *tmp = (AIOGainRange *)calloc(16,sizeof(AIOGainRange));
    if (!tmp )
        return tmp;

    for ( int i = 0; i < AD_NUM_GAIN_CODE_REGISTERS ; i ++ ) {
        tmp[i].min = adRanges[ adc->registers[i] ].minVolts;
        tmp[i].max = adRanges[ adc->registers[i] ].minVolts + adRanges[ adc->registers[i] ].range;
    }

    return tmp;
}

/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN void DeleteAIOGainRange( AIOGainRange *agr )
{
    free(agr);
}


#ifdef __cplusplus
}
#endif


#ifdef SELF_TEST

#include "AIOUSBDevice.h"
#include "AIOUSB_Core.h"
#include "AIOFifo.h"
#include "gtest/gtest.h"
#include "tap.h"
#include <iostream>
using namespace AIOUSB;


/**
 * @todo Want the API to set the isInit flag in case the device is ever added
 */
TEST(Initialization,BasicInit )
{
    unsigned short *counts = (unsigned short *)malloc(2000);
    AIOCountsConverter *cc = NewAIOCountsConverterWithBuffer( counts, 16, NULL, 20 , sizeof(unsigned short)  );

    DeleteAIOCountsConverter(cc);
}


TEST(Initialization,Callback )
{

    AIOGainRange *ranges = (AIOGainRange *)malloc(16*sizeof(AIOGainRange));
    int num_channels     = 16;
    int num_oversamples  = 20;
    int num_scans        = 1000;

    int total_size       = num_channels * num_oversamples * num_scans;

    unsigned short *from_buf = (unsigned short *)malloc(total_size*sizeof(unsigned short));
    double *to_buf   = ( double *)malloc(total_size*sizeof(double));

    for ( int i = 0; i < num_channels; i ++ ) {
        ranges[i].max = 10.0;
        ranges[i].min = -10.0;
    }

    AIOCountsConverter *cc = NewAIOCountsConverterWithBuffer( from_buf, num_channels, ranges, num_oversamples , sizeof(unsigned short)  );
    
    for ( int i = 0; i < total_size; i++ )
        from_buf[i] = (((unsigned short)-1)+1) / 2;

    /**
     * @brief When we convert this buffer, we expect the oversmaples to go away ( so total size
     * will be num_channels * num_scans;
     *
     * We also expect that the voltages should be halfway between the min and max since we are
     * using the halfway value of an unsigned short
     */
    int count = cc->Convert( cc, to_buf, from_buf , total_size*sizeof(unsigned short) );
    EXPECT_EQ( num_scans*num_channels*(1+num_oversamples)*sizeof(unsigned short), count ) << "We should remove all of the oversamples and " << 
        "leave just the number_of_scans * number_of_channels";
    
    EXPECT_EQ( to_buf[0], (ranges[0].max + ranges[0].min) / 2 );
    DeleteAIOCountsConverter(cc);
}


TEST(Composite,FifoWriting )
{

    AIOGainRange *ranges = (AIOGainRange *)malloc(16*sizeof(AIOGainRange));
    int num_channels     = 16;
    int num_oversamples  = 20;
    int num_scans        = 1000;
    int retval = 0;
    int total_size       = num_channels * (num_oversamples+1) * num_scans;

    unsigned short *from_buf = (unsigned short *)malloc(total_size*sizeof(unsigned short));
    double *to_buf   = ( double *)malloc(total_size*sizeof(double));

    for ( int i = 0; i < num_channels; i ++ ) {
        ranges[i].max = 10.0;
        ranges[i].min = 0.0;
    }
    for ( int i = 0; i < total_size; i++ )
        from_buf[i] = (((unsigned short)-1)+1) / 2;

    AIOCountsConverter *cc = NewAIOCountsConverterWithBuffer( from_buf, num_channels, ranges, num_oversamples , sizeof(unsigned short)  );

    AIOFifoCounts *infifo = NewAIOFifoCounts( (unsigned)num_channels*(num_oversamples+1)*num_scans );
    AIOFifoVolts *outfifo = NewAIOFifoVolts( num_channels*(num_oversamples+1)*num_scans );

    /**
     * @brief Load the fifo with values
     */

    retval = infifo->PushN( infifo, from_buf, total_size );
    EXPECT_GE( retval, total_size*sizeof(unsigned short) );

    retval = cc->ConvertFifo( cc, outfifo, infifo , total_size*sizeof(unsigned short) );

    EXPECT_GE( retval, 0 );

    outfifo->PopN( outfifo, to_buf, num_channels );

    for ( int i = 0 ; i < num_channels ; i ++ ) {
        EXPECT_EQ( to_buf[i], (ranges[0].max + ranges[0].min) / 2 ) << "For i=" << i << std::endl;
    }
}

class AllGainCode : public ::testing::TestWithParam<ADGainCode> {};
TEST_P( AllGainCode, FromADCConfigBlock )
{
    ADConfigBlock cb = {0};
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    ADCConfigBlockInitializeDefault( &cb );
    ADGainCode gcode = GetParam();

    /* Try several gain codes */
    for ( int i = 0; i < 16 ; i ++ ) {
        retval = ADCConfigBlockSetGainCode( &cb, i, gcode );
        ASSERT_EQ(  0 , retval ) << "Able to set the channel's gain code correctly for channel '" << i << "'" << std::endl;

        int tmp = ADCConfigBlockGetGainCode(&cb, i );
        ASSERT_EQ( gcode, ADCConfigBlockGetGainCode(&cb, i )) << "Able to get the channel's gain code correctly for channel '" << i << "'" << std::endl;
    }

    /* Verify that we can copy these settings to the gaincode */
    AIOGainRange *tmpvals = NewAIOGainRangeFromADCConfigBlock( &cb );
    ASSERT_TRUE( tmpvals );

    for ( int i = 0; i < 16 ; i ++ ) {
        ASSERT_EQ( (adRanges[ gcode ].minVolts), tmpvals[i].min ) << "Able to get the Min value";
        ASSERT_EQ( (adRanges[ gcode ].minVolts + adRanges[ gcode ].range), tmpvals[i].max ) << "Able to get the Max value";
    }

    DeleteAIOGainRange( tmpvals );
}

INSTANTIATE_TEST_CASE_P( TestRangeConversion, AllGainCode, ::testing::Values( AD_GAIN_CODE_0_10V,
                                                                              AD_GAIN_CODE_10V,  
                                                                              AD_GAIN_CODE_0_5V, 
                                                                              AD_GAIN_CODE_5V,   
                                                                              AD_GAIN_CODE_0_2V, 
                                                                              AD_GAIN_CODE_2V,   
                                                                              AD_GAIN_CODE_0_1V, 
                                                                              AD_GAIN_CODE_1V    
                                                                              ));


int main(int argc, char *argv[] )
{

  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
  delete listeners.Release(listeners.default_result_printer());
#endif

  listeners.Append( new tap::TapListener() );
  return RUN_ALL_TESTS();  
}



#endif
