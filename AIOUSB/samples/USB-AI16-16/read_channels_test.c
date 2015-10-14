/*
 * sample program to write out the calibration table and then 
 * reload it again, verify that the data is in fact reversed
 */

#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include "aiocommon.h"


AIOUSB_BOOL find_ai_board( AIOUSBDevice *dev ) { 
    if ( dev->ProductID >= USB_AI16_16A && dev->ProductID <= USB_AI12_128E ) { 
        return AIOUSB_TRUE;
    } else if ( dev->ProductID >=  USB_AIO16_16A && dev->ProductID <= USB_AIO12_128E ) {
        return AIOUSB_TRUE;
    } else {
            return AIOUSB_FALSE;
    }
}


int main( int argc, char **argv ) 
{
    struct opts options = AIO_OPTIONS;
    AIORET_TYPE result = AIOUSB_SUCCESS;
    int *indices;
    int num_devices;
    ADCConfigBlock *config;
    AIOUSBDevice *dev;
    USBDevice *usb;
    double *volts;
    struct timespec starttime , curtime ;
    process_aio_cmd_line( &options, argc, argv );

    result = AIOUSB_Init();
    if ( result != AIOUSB_SUCCESS ) {
        fprintf(stderr,"Error calling AIOUSB_Init(): %d\n", (int)result );
        exit(result );
    }

    AIOUSB_ListDevices();

    process_aio_cmd_line( &options, argc, argv );

    AIOUSB_FindDevices( &indices, &num_devices, find_ai_board );
    
    if( (result = aio_list_devices( &options, indices, num_devices ) != AIOUSB_SUCCESS )) 
        exit(result);

    if ( (config = NewADCConfigBlockFromJSON( options.adcconfig_json )) == NULL )
        exit(AIOUSB_ERROR_INVALID_ADCCONFIG);

    if ( (result = aio_override_adcconfig_settings( config, &options )) != AIOUSB_SUCCESS )
        exit(result);


    /* Save the config for the device index  in question */
    dev = AIODeviceTableGetDeviceAtIndex( options.index , (AIORESULT*)&result );
    if ( result != AIOUSB_SUCCESS ) {
        fprintf(stderr,"Error getting device at index %d\n", options.index );
        exit(result);
    }

    usb = AIOUSBDeviceGetUSBHandle( dev );

    /* Copy the modified config settings back to the 
     * device ave config to the device 
     */
    result = ADCConfigBlockCopy( AIOUSBDeviceGetADCConfigBlock( dev ), config );
    result = USBDevicePutADCConfigBlock( usb, config );
    /* or do this     
     * ADC_SetConfig( options.index, config->registers, &config->size ); */

    volts = (double*)malloc((ADCConfigBlockGetEndChannel( config )-ADCConfigBlockGetStartChannel( config )+1)*sizeof(double));
    
    if ( options.with_timing ) 
        clock_gettime( CLOCK_MONOTONIC_RAW, &starttime );

    for ( int i = 0, channel = 0; i < options.num_scans; i ++ , channel = 0) {
        if ( options.counts ) { /* --counts will write out the raw values */
            if ( options.with_timing )
                clock_gettime( CLOCK_MONOTONIC_RAW, &curtime );
            ADC_GetScan( options.index, (unsigned short*)volts );
            if ( options.with_timing ) 
                clock_gettime( CLOCK_MONOTONIC_RAW, &starttime );


            unsigned short *counts = (unsigned short *)volts;
            if( options.with_timing )
                fprintf(stdout,"%d,%d,", (int)starttime.tv_sec, (int)(( starttime.tv_sec - curtime.tv_sec )*1e9 + (starttime.tv_nsec - curtime.tv_nsec )));



            for ( int j = ADCConfigBlockGetStartChannel( config ); j < ADCConfigBlockGetEndChannel( config ) ; j ++ , channel ++) {
                printf("%u,", counts[channel] );
            }
            printf("%u\n", counts[channel] );


        } else {
            if ( options.with_timing )
                clock_gettime( CLOCK_MONOTONIC_RAW, &curtime );
            ADC_GetScanV( options.index, volts );
            if ( options.with_timing ) 
                clock_gettime( CLOCK_MONOTONIC_RAW, &starttime );

            if( options.with_timing )
                fprintf(stdout,"%d,%d,", (int)starttime.tv_sec, (int)(( starttime.tv_sec - curtime.tv_sec )*1e9 + (starttime.tv_nsec - curtime.tv_nsec )));
            for ( int j = ADCConfigBlockGetStartChannel( config ); j < ADCConfigBlockGetEndChannel( config ) ; j ++ , channel ++) {
                printf("%.3f,", volts[channel] );
            }
            printf("%f\n", volts[channel] );


        }

    }

    return 0;
}



