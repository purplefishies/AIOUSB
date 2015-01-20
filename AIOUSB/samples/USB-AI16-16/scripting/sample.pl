#!/usr/bin/perl

use AIOUSB;

{
    package Device;

    sub new(@) {
        my ($self,%args) = @_;
        bless \%args,Device;
    }

}


my $deviceIndex = 0;
my $MAX_NAME_SIZE = 20;

my $deviceIndex = 0;
my $deviceFound = $AIOUSB::AIOUSB_FALSE;

my $CAL_CHANNEL = 5;
my $MAX_CHANNELS = 128;
my $NUM_CHANNELS = 16;
my $NUM_OVERSAMPLES = 10;
my $NUM_SCANS = 100000;
my $BULK_BYTES = $NUM_SCANS * $NUM_CHANNELS * 2 * ($NUM_OVERSAMPLES+1);
my $CLOCK_SPEED = 500000 / ( $NUM_CHANNELS * ($NUM_OVERSAMPLES+1) );


AIOUSB::AIOUSB_Init();
AIOUSB::AIOUSB_Reset( $deviceIndex );
AIOUSB::AIOUSB_SetCommTimeout( $deviceIndex, 1000 );
AIOUSB::AIOUSB_SetDiscardFirstSample( $deviceIndex, $AIOUSB::AIOUSB_TRUE );

$deviceIndex = 0;
$result = AIOUSB::AIOUSB_Init();
if ( $result != AIOUSB::AIOUSB_SUCCESS ) {
    print STDERR "Error running AIOUSB_Init()...";
    exit(1);
}

$deviceMask = AIOUSB::GetDevices();
if( $deviceMask == 0 ) {
    print STDERR "No ACCES devices found on USB bus\n";
    exit(1);
}

$devices = [];
$number_devices = 1;
$index = 0;

while( $deviceMask > 0 && $#{$devices} < $number_devices ) { 
    if( $deviceMask & 1 ) {
        my $obj = AIOUSB::AIODeviceInfoGet( $index );
        if( $obj->swig_PID_get() == $AIOUSB::USB_AIO16_16A || $obj->swig_PID_get == $AIOUSB::USB_DIO_16A ) { 
            push( @{$devices}, new Device( "index" => $index, "productID"=>$obj->swig_PID_get(), "numDIOBytes"=>$obj->swig_DIOBytes_get,"numCounters" => $obj->swig_Counters_get ));
            print "";
        }
    }
    $index += 1;
    $deviceMask >>= 1;
}

print "Size of devices is $#{$devices}\n";


$deviceIndex = $devices->[0]->{index};


AIOUSB::AIOUSB_Reset( $deviceIndex );
print "Setting timeout\n";
AIOUSB::AIOUSB_SetCommTimeout( $deviceIndex, 1000 );
AIOUSB::AIOUSB_SetDiscardFirstSample( $deviceIndex, $AIOUSB::AIOUSB_TRUE );

$serialNumber = AIOUSB::new_ulp();
$result = AIOUSB::GetDeviceSerialNumber( $deviceIndex, $serialNumber );
print sprintf "Serial number of device at index %d: %x\n" , $deviceIndex, AIOUSB::ulp_value( $serialNumber );

$result = 0;
($ndevice,$result) = AIOUSB::AIODeviceTableGetDeviceAtIndex( $deviceIndex , $result );
$cb = AIOUSB::AIOUSBDeviceGetADCConfigBlock( $ndevice );
print "";


AIOUSB::AIOUSB_SetAllGainCodeAndDiffMode( $cb, $AIOUSB::AD_GAIN_CODE_10V, $AIOUSB::AIOUSB_FALSE );
AIOUSB::AIOUSB_SetCalMode( $cb, $AIOUSB::AD_CAL_MODE_NORMAL );
AIOUSB::AIOUSB_SetTriggerMode( $cb, 0 );
AIOUSB::AIOUSB_SetScanRange( $cb, 2, 13 );
AIOUSB::AIOUSB_SetOversample( $cb, 0 );

AIOUSB::ADC_WriteADConfigBlock( $deviceIndex, $cb );

print "A/D settings successfully configured\n";



$retval = AIOUSB::ADC_SetCal($deviceIndex, ":AUTO:");
if( $result != $AIOUSB::AIOUSB_SUCCESS ) {
    print "Error '%s' performing automatic A/D calibration" % ( AIOUSB_GetResultCodeAsString( result ) );
    sys.exit(0);
}

AIOUSB::ADC_SetOversample( $deviceIndex, 0 );
AIOUSB::ADC_SetScanLimits( $deviceIndex, $CAL_CHANNEL, $CAL_CHANNEL );
AIOUSB::ADC_ADMode( $deviceIndex, 0 , $AIOUSB::AD_CAL_MODE_GROUND );

$counts = AIOUSB::new_ushortarray( 16 );
$result = AIOUSB::ADC_GetScan( $deviceIndex, $counts );

if( $result != $AIOUSB::AIOUSB_SUCCESS ) {
    print sprintf "Error '%s' attempting to read ground counts\n" , AIOUSB::AIOUSB_GetResultCodeAsString( $result );
} else {
    print sprintf "Ground counts = %u (should be approx. 0)\n" , AIOUSB::ushort_getitem( $counts, $CAL_CHANNEL) ;
}


AIOUSB::ADC_ADMode( $deviceIndex, 0 , $AIOUSB::AD_CAL_MODE_REFERENCE ); # TriggerMode
$result = AIOUSB::ADC_GetScan( $deviceIndex, $counts );
if( result != AIOUSB_SUCCESS ) { 
    print sprintf "Error '%s' attempting to read reference counts\n" , AIOUSB_GetResultCodeAsString( result );
} else {
    print sprintf "Reference counts = %u (should be approx. 65130)\n", AIOUSB::ushort_getitem( $counts, $CAL_CHANNEL );
}

$gainCodes = [ map { 0 } 1..16 ];

#
# demonstrate scanning channels and measuring voltages
#

for($i = 0; $i < 16; $i ++ ) { 
    $gainCodes->[$i] = $AIOUSB::AD_GAIN_CODE_0_10V;
}

AIOUSB::ADC_RangeAll( $deviceIndex , $gainCodes, $AIOUSB::AIOUSB_TRUE );

AIOUSB::ADC_SetOversample( $deviceIndex, $NUM_OVERSAMPLES );
AIOUSB::ADC_SetScanLimits( $deviceIndex, 0, $NUM_CHANNELS - 1 );
AIOUSB::ADC_ADMode( $deviceIndex, 0 , $AD_CAL_MODE_NORMAL );


print "Volts read:";

$volts = [map { 0 } 1..16 ];
for( $i = 0; $i < 1 ; $i ++ ) {
    $result = AIOUSB::ADC_GetScanV( $deviceIndex, $volts );
    for( $j = 0; $j < 16 ; $j ++ ) {
        print sprintf "  Channel %2d = %6.6f\n" , $j, $result->[$j];
    }
}

# demonstrate reading a single channel in volts
($result, $exitcode) = AIOUSB::ADC_GetChannelV( $deviceIndex, $CAL_CHANNEL, $volts->[ 0 ] );

print sprintf("Result from A/D channel %d was %f\n" , $CAL_CHANNEL, $result->[0] );

($result, $exitcode) = AIOUSB::ADC_GetChannelV( $deviceIndex, 1 , $volts->[ 0 ] );

print sprintf("Result from A/D channel %d was %f\n", 1, $result->[0] );





AIOUSB::AIOUSB_Reset( $deviceIndex );
AIOUSB::ADC_SetOversample( $deviceIndex, $NUM_OVERSAMPLES );
AIOUSB::ADC_SetScanLimits( $deviceIndex, 0, $NUM_CHANNELS - 1 );
AIOUSB::AIOUSB_SetStreamingBlockSize($deviceIndex, 64*1024 );


print sprintf("Allocating %d Bytes\n", ( $BULK_BYTES ));
$databuf = AIOUSB::new_ushortarray( $BULK_BYTES );


$clockHz = 0;
AIOUSB::CTR_StartOutputFreq( $deviceIndex, 0, $clockHz );


# 
#  configure A/D for timer-triggered acquisition
#
AIOUSB::ADC_ADMode( $deviceIndex, $AIOUSB::AD_TRIGGER_SCAN | $AIOUSB::AD_TRIGGER_TIMER, $AIOUSB::AD_CAL_MODE_NORMAL );


 
# start bulk acquire; ADC_BulkAcquire() will take care of starting
# and stopping the counter; but we do have to tell it what clock
# speed to use, which is why we call AIOUSB_SetMiscClock()
# 
print sprintf("Using Clock speed %d to acquire data\n" , $CLOCK_SPEED );
$result = AIOUSB::AIOUSB_SetMiscClock( $deviceIndex, $CLOCK_SPEED );

#print "Index is $deviceIndex\n";

$result = AIOUSB::ADC_BulkAcquire( $deviceIndex, $BULK_BYTES, $databuf );

if( $result != AIOUSB::AIOUSB_SUCCESS ) { 
    print sprintf( "Error '%s' attempting to start bulk acquire of %d bytes\n" , AIOUSB::AIOUSB_GetResultCodeAsString( $result ), $BULK_BYTES );
    exit(1);
} else {
    print sprintf( "Started bulk acquire of %d bytes\n", BULK_BYTES );
}

$bytesRemaining = AIOUSB::new_ulp();
AIOUSB::ulp_assign( $bytesRemaining, $BULK_BYTES );

for( $i = 0 ; $i < 60 ; $i ++ ) {
    sleep(1);
    $result = AIOUSB::ADC_BulkPoll( $deviceIndex, $bytesRemaining );
    if( $result != $AIOUSB::AIOUSB_SUCCESS ) {
        print sprintf("Error '%s' polling bulk acquire progress\n" , AIOUSB::AIOUSB_GetResultCodeAsString( $result ));
    } else {
        print sprintf( "  %lu bytes remaining\n" , AIOUSB::ulp_value( $bytesRemaining ) );
        last if( AIOUSB::ulp_value($bytesRemaining) == 0 );
    }
}

__END__


__END__
