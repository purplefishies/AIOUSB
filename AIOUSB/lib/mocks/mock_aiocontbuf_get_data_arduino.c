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
    int number_scans = buf->num_scans;
    *bytes = 0;
    int pos;
    static int scan_count = 0;
    static int channel_count = 0;
    static int os = 0;
    static char arduino_counter = 0;
    double scale_down = 1;
    int noisy_ground = 30;
    int noisy_signal = 10;
    if ( getenv("MOCK_SCALE_DOWN") ) { 
        scale_down = strtod(getenv("MOCK_SCALE_DOWN"),NULL);
    } 
    if ( getenv("MOCK_NOISY_GROUND") )  {
        noisy_ground = atoi(getenv("MOCK_NOISY_GROUND"));
    }
    if ( getenv("MOCK_NOISY_SIGNAL") )  {
        noisy_signal = atoi(getenv("MOCK_NOISY_SIGNAL"));
    }


    int initial = (scan_count *(AIOContinuousBufNumberChannels(buf)*(AIOContinuousBufGetOversample(buf)+1))) + 
        channel_count * ( AIOContinuousBufGetOversample(buf)+1 ) + os;

    for ( ; scan_count < number_scans; scan_count ++ ) { 
        for ( ; channel_count < AIOContinuousBufNumberChannels(buf); channel_count ++ ) {
            for ( ; os < AIOContinuousBufGetOversample(buf)+1 ; os ++ ) {
                if ( *bytes >= (datasize) ) {
                    goto end_aiocontbuf_get_data;
                }
                pos = (scan_count *(AIOContinuousBufNumberChannels(buf)*(AIOContinuousBufGetOversample(buf)+1))) + 
                    channel_count * ( AIOContinuousBufGetOversample(buf)+1 ) + os - initial;

                /* counts[pos] =  (uint16_t)(65535 / (AIOContinuousBufNumberChannels(buf)-1)) * channel_count; */
                int tmpval;
                if ( channel_count < 3 ) { 
                    if ( channel_count == 0 ) { 
                        /* 10110101  >> 4 */
                        tmpval = ((arduino_counter) >> 4 ) & 1;
                    } else if ( channel_count == 1 ) { 
                        tmpval = ((arduino_counter) >> 5 ) & 1;
                    } else {
                        tmpval = ((arduino_counter) >> 6 ) & 1;
                    }
                    if ( tmpval == 1 ) { 
                        tmpval = (int)((65535 - ( rand() % noisy_signal )) / scale_down);
                    } else {
                        tmpval = (rand() % noisy_ground );
                    }
                } else {
                    tmpval = rand() % noisy_ground;
                }
                counts[pos] = tmpval;

                if ( pos > 65536 /2 ) {
                    fprintf(stderr,"ERROR: pos=%d\n", pos );
                }
                *bytes += 2;

                if ( *bytes >= (datasize) ) { 
                    os ++;
                    goto end_aiocontbuf_get_data;
                }
            }
            os = 0;
        }
        arduino_counter ++;
        channel_count = 0;
    }
 end_aiocontbuf_get_data:
    /* printf("Final value=%d\n",*bytes); */
    /* usbresult = number_scans*AIOContinuousBufNumberChannels(buf)*( AIOContinuousBufGetOversample(buf)+1 )*sizeof(uint16_t); */
    usbresult = *bytes;
    return usbresult;
}
