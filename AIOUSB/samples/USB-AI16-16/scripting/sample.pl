#!/usr/bin/perl

use AIOUSB;

$deviceIndex = 0;
AIOUSB::AIOUSB_Init();
AIOUSB::AIOUSB_Reset( $deviceIndex );
AIOUSB::AIOUSB_SetCommTimeout( $deviceIndex, 1000 );
AIOUSB::AIOUSB_SetDiscardFirstSample( $deviceIndex, $AIOUSB::AIOUSB_TRUE );

$cb = AIOUSB::AIOUSB_GetConfigBlock( $deviceIndex  );

AIOUSB::AIOUSB_InitConfigBlock($cb, $deviceIndex, $AIOUSB::AIOUSB_FALSE );
AIOUSB::AIOUSB_SetAllGainCodeAndDiffMode($cb, $AIOUSB::AD_GAIN_CODE_10V, $AIOUSB::AIOUSB_FALSE );
AIOUSB::AIOUSB_SetCalMode($cb, $AIOUSB::AD_CAL_MODE_NORMAL );
AIOUSB::AIOUSB_SetTriggerMode($cb, 0 );
AIOUSB::AIOUSB_SetScanRange($cb, 2, 13 );
AIOUSB::AIOUSB_SetOversample($cb, 0 );
AIOUSB::ADC_CopyConfig( $deviceIndex,$cb );

# const int CAL_CHANNEL = 5;
# const int MAX_CHANNELS = 128;
# const int NUM_CHANNELS = 16;
# unsigned short counts[ MAX_CHANNELS ];
# double volts[ MAX_CHANNELS ];
# unsigned char gainCodes[ NUM_CHANNELS ];

$result = AIOUSB::ADC_SetCal( $deviceIndex, ":AUTO:" );

$CAL_CHANNEL = 5;


AIOUSB::ADC_SetOversample( $deviceIndex, 0 );
AIOUSB::ADC_SetScanLimits( $deviceIndex, $CAL_CHANNEL, $CAL_CHANNEL );
AIOUSB::ADC_ADMode( $deviceIndex, 0 , $AIOUSB::AD_CAL_MODE_GROUND );
# if ( $result == $AIOUSB::AIOUSB_SUCCESS ) {
#     printf( "Ground counts = %u (should be approx. 0)\n", counts[ CAL_CHANNEL ] );
# } else { 
#     printf( "Error '%s' attempting to read ground counts\n", AIOUSB_GetResultCodeAsString( result ) );
# }



AIOUSB::ADC_ADMode( $deviceIndex, 0 , $AIOUSB::AD_CAL_MODE_REFERENCE );

AIOUSB::AIOUSB_Reset( $deviceIndex );
AIOUSB::ADC_SetOversample( $deviceIndex, 10 );
AIOUSB::ADC_SetScanLimits( $deviceIndex, 0, 15 );
AIOUSB::AIOUSB_SetStreamingBlockSize( $deviceIndex, 100352 );

$bb = AIOUSB::CreateSmartBuffer( $deviceIndex );

$clockHz = AIOUSB::new_dp();
AIOUSB::dp_assign( $clockHz , 1 );
AIOUSB::CTR_StartOutputFreq( $deviceIndex, 0, $clockHz );

AIOUSB::ADC_ADMode( $deviceIndex, $AIOUSB::AD_TRIGGER_SCAN | $AIOUSB::AD_TRIGGER_TIMER, $AIOUSB::AD_CAL_MODE_NORMAL );
AIOUSB::AIOUSB_SetMiscClock( $deviceIndex, 100000 );

$result = AIOUSB::BulkAcquire( $deviceIndex,  $bb , $bb->swig_bufsize_get()  );

if( $result != AIOUSB_SUCCESS  ) {
    printf( "Error '%s' attempting to start bulk acquire of %d bytes\n", AIOUSB::AIOUSB_GetResultCodeAsString( $result ), BULK_BYTES );
    exit(1);
}
print "Remaning is " . $bb->swig_bytes_remaining_get() . "\n";
sleep(1);
for ( $seconds = 0; $seconds < 100; $seconds++ ) {
    sleep( 1 );

    $result = AIOUSB::BulkPoll( $deviceIndex, $bb );

    if( $result == AIOUSB_SUCCESS ) {
        printf( "  %lu bytes remaining\n", $bb->swig_bytes_remaining_get() );
        if ( $bb->swig_bytes_remaining_get() == 0 ) {
            last;
        }
    } else {
        printf( "Error '%s' polling bulk acquire progress\n", AIOUSB::AIOUSB_GetResultCodeAsString( $result ));
        last;
    }
}
