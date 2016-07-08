/*
 * sample program to write out the calibration table and then 
 * reload it again, verify that the data is in fact reversed
 */

import AIOUSB.*;
import java.util.ArrayList;
import java.util.Arrays;

public class read_channels_test_java {
    public static final String PROGNAME = new read_channels_test_java().getClass().getName();

    public static void main( String args[] ) {

        AIOCommandLineOptions options = AIOUSB.NewAIOCommandLineOptionsFromDefaultOptions( AIOUSB.AIO_CMDLINE_SCRIPTING_OPTIONS() );
        long retval;
        System.out.println("Number of args are " + args.length  + "\n");
        System.out.println("Bar: " + Thread.currentThread().getStackTrace()[1].getClassName() + "\n") ;
        // ArrayList nargs = new java.util.ArrayList( args.toList() );
        ArrayList<String> tmpargs = new ArrayList<String>(Arrays.asList(args));
        ArrayList<Integer> indices = new ArrayList<Integer>();

        
        retval = AIOUSB.AIOProcessCommandLine( options, tmpargs );

        retval = AIOUSB.AIOUSB_FindDeviceIndicesByGroup( indices, AIOUSB.AIO_ANALOG_INPUT() );

        retval = AIOUSB.AIOCommandLineOptionsListDevices( options, indices );
        
        ADCConfigBlock config = AIOUSB.NewADCConfigBlockFromJSON( AIOUSB.AIOCommandLineOptionsGetDefaultADCJSONConfig(options) )  ;
        
        System.out.println(config);

        retval = AIOUSB.AIOCommandLineOptionsOverrideADCConfigBlock( config, options );

        retval = AIOUSB.ADC_SetCal(AIOUSB.AIOCommandLineOptionsGetDeviceIndex(options), ":AUTO:");
        
        // SWIGTYPE_p_AIOUSBDevice dev = AIOUSB.AIODeviceTableGetAIOUSBDeviceAtIndex( AIOUSB.AIOCommandLineOptionsGetDeviceIndex(options));
        // SWIGTYPE_p_USBDevice usb = AIOUSB.AIOUSBDeviceGetUSBHandle( dev );
        // retval = AIOUSB.ADCConfigBlockCopy( AIOUSB.AIOUSBDeviceGetADCConfigBlock( dev ), config );
        // retval = AIOUSB.USBDevicePutADCConfigBlock( usb, config );
        // 
        // Or in just one line

        retval = AIOUSB.USBDevicePutADCConfigBlock( AIOUSB.AIOUSBDeviceGetUSBHandle( AIOUSB.AIODeviceTableGetAIOUSBDeviceAtIndex( AIOUSB.AIOCommandLineOptionsGetDeviceIndex(options))), config );

        doublearray volts = new doublearray(16);

        for ( int i = 1; i <= (int)(AIOUSB.AIOCommandLineOptionsGetScans(options)); i ++ ) { 
            retval = AIOUSB.ADC_GetScanV( AIOUSB.AIOCommandLineOptionsGetDeviceIndex(options), volts.cast() );
            System.out.println( volts );
        }


    }
};

