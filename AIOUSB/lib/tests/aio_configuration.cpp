#include "AIOCommandLine.h"
#include "gtest/gtest.h"
#include "tap.h"

using namespace AIOUSB;

TEST(FOO,BAR) 
{
    EXPECT_EQ(1,1);
    EXPECT_NE(2,1);
}

/**
 * command --device index=0,adcconfig=$(cat foo.json),timeout=1000,debug=1
 *         --device index=1,adcconfig=$(cat foo.json),timeout=1000,channel=2=+-2V,sample=getscan,file=foo.csv
 *         --device index=2,aiocontinuousbuf=$(cat cb.json),timeout=2000,setcal=auto,count=3000,sample=getscanv,file=bar.csv
 *         --device index=2,aiocontinuousbuf=$(cat cb.json),timeout=2000,setcal=normal,count=3000,sample=getscanv,file=bar.csv
 *         --device index=3,timeout=1000,setcal=normal,numscans=1000,sample=getscanv,file=output.csv
 **/

TEST(AIOCommandLine,ParseSimpleADCJSON )
{
    optind = 0;
    char *argv[] = {
        (char *)"this_prog",
        (char *)"--aiodevice",
        (char *)"0",
        (char *)"--aiotimeout",
        (char *)"110",
        (char *)"--aiodebug",
        (char *)"1"
    };
    int argc = 7;
    AIOArgument *args = aiousb_getoptions( argc, argv );

    for ( int i = 0; i < *args->size ; i ++ ) {
        EXPECT_EQ( args[i].config.timeout, 110 );
        EXPECT_EQ( args[i].config.debug, AIOUSB_TRUE );
        EXPECT_EQ( args[i].config.timeout, 110 );
    }
    free(args);
}

TEST(AIOCommandLine,ParseAdvancedADCJSON )
{
    optind = 0;
    char *argv[] = {
        (char *)"this_prog",
        (char *)"--aiodevice=0",
        (char *)"--adcconfig={\"adcconfig\":{\"channels\":[{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"}],\"calibration\":\"Normal\",\"trigger\":{\"reference\":\"sw\",\"edge\":\"rising-edge\",\"refchannel\":\"single-channel\"},\"start_channel\":\"0\",\"end_channel\":\"15\",\"oversample\":\"102\",\"timeout\":\"1000\"}}",
        (char *)"--aiotimeout=1200"
    };
    int argc = 4;
    AIOArgument *args = aiousb_getoptions( argc, argv );

    for ( int i = 0; i < *args->size ; i ++ ) {
        EXPECT_EQ( args[i].config.timeout, 1200 );
        EXPECT_EQ( args[i].config.debug, AIOUSB_FALSE );
    }
    free(args);
}

TEST(AIOCommandLine,MultipleCommands )
{
    optind = 0;
    char *argv[] = {
        (char *)"this_prog",
        (char *)"--aiodevice=0",
        (char *)"--aiotimeout=1021",
        (char *)"--aiodevice=1",
        (char *)"--aiotimeout=1022",
        (char *)"--aiodebug=1",
    };
    int argc = 6;
    AIOArgument *args = aiousb_getoptions( argc, argv );

    for ( int i = 0; i < *args->size ; i ++ ) {
        if ( i == 0 ) {
            EXPECT_EQ( args[i].config.timeout, 1021 );
            EXPECT_EQ( args[i].config.debug, AIOUSB_FALSE );
        } else {
            EXPECT_EQ( args[i].config.timeout, 1022 );
            EXPECT_EQ( args[i].config.debug, AIOUSB_TRUE );
        }

    }
    free(args);
}

TEST(AIOCommandLine,MultipleJSONObjects )
{
 optind = 0;
    char *argv[] = {
        (char *)"this_prog",
        (char *)"--aiodevice=0",
        (char *)"--adcconfig={\"adcconfig\":{\"mux_settings\":{\"adc_channels_per_group\":\"1\",\"adc_mux_channels\":\"16\"},\"channels\":[{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"}],\"calibration\":\"Normal\",\"trigger\":{\"reference\":\"sw\",\"edge\":\"rising-edge\",\"refchannel\":\"single-channel\"},\"start_channel\":\"0\",\"end_channel\":\"15\",\"oversample\":\"102\",\"timeout\":\"1000\"}}",
        (char *)"--aiodevice=1",
        (char *)"--adcconfig={\"adcconfig\":{\"mux_settings\":{\"adc_channels_per_group\":\"1\",\"adc_mux_channels\":\"16\"},\"channels\":[{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"}],\"calibration\":\"Normal\",\"trigger\":{\"reference\":\"sw\",\"edge\":\"rising-edge\",\"refchannel\":\"single-channel\"},\"start_channel\":\"1\",\"end_channel\":\"13\",\"oversample\":\"105\",\"timeout\":\"1000\"}}",
    };

    int argc = 5;
    AIOArgument *args = aiousb_getoptions( argc, argv );
    EXPECT_EQ( *args->size, 2 );

}

TEST(AIOCommandLine,DeviceObject )
{
 optind = 0;
    char *argv[] = {
        (char *)"this_prog",
        (char *)"--aiousbdevice={\"device_index\":\"0\",\"adc_channels_per_group\":\"1\",\"adc_mux_channels\":\"16\",\"adcconfig\":{\"channels\":[{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"0-2V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"+-5V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"},{\"gain\":\"0-10V\"}],\"calibration\":\"Normal\",\"trigger\":{\"reference\":\"sw\",\"edge\":\"rising-edge\",\"refchannel\":\"single-channel\"},\"start_channel\":\"0\",\"end_channel\":\"15\",\"oversample\":\"102\",\"timeout\":\"1000\"}}",
    };
}



int main(int argc, char *argv[] )
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
    delete listeners.Release(listeners.default_result_printer());
#endif

    listeners.Append( new tap::TapListener() );
    return RUN_ALL_TESTS();  
}
