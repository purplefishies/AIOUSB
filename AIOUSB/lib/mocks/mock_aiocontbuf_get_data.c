#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <unistd.h>
#include <sys/stat.h>
#include "AIOTypes.h"
#include "USBDevice.h"
#include "AIOUSB_Core.h"

#include <dlfcn.h>



AIORET_TYPE aiocontbuf_get_data( AIOContinuousBuf *buf, 
                                 USBDevice *usb, 
                                 unsigned char endpoint, 
                                 unsigned char *data,
                                 int datasize,
                                 int *bytes,
                                 unsigned timeout 
                                 )
{
    AIORET_TYPE usbresult;
    uint16_t *counts = (uint16_t*)data;
    int number_scans = datasize / ( AIOContinuousBufNumberChannels(buf)*(AIOContinuousBufGetOverSample(buf)+1)*sizeof(uint16_t));
    *bytes = 0;
    int pos;
    for ( int scan_num = 0; scan_num < number_scans; scan_num ++ ) { 

        for ( int channel_count = 0; channel_count < AIOContinuousBufNumberChannels(buf); channel_count ++ ) {

            for ( int os = 0; os < AIOContinuousBufGetOverSample(buf)+1 ; os ++ ) { 
                pos = (scan_num *(AIOContinuousBufNumberChannels(buf)*(AIOContinuousBufGetOverSample(buf)+1))) + 
                    channel_count * ( AIOContinuousBufGetOverSample(buf)+1 ) + os;
                counts[pos] =  (uint16_t)(65535 / (AIOContinuousBufNumberChannels(buf)-1)) * channel_count;
                *bytes += 2;
            }
        }
    }
    usbresult = number_scans*AIOContinuousBufNumberChannels(buf)*( AIOContinuousBufGetOverSample(buf)+1 )*sizeof(uint16_t);
    return usbresult;
}

