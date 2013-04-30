/**
 * @file   AIOUSB_ADC.c
 * @author $Author$
 * @date   $Date$
 * @copy
 * @brief
 *  ACCES I/O USB API for Linux
 *
 */


#include "AIOUSB_Core.h"
#include <stdio.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif


static
int non_usb_supported_device(int minProductID, int maxProductID, int maxDevices, int *deviceList)
{
    return minProductID < 0 ||
           minProductID > 0xffff ||
           maxProductID < minProductID ||
           maxProductID > 0xffff ||
           maxDevices < 1 ||
           maxDevices > 127 ||     // sanity check; USB can support only 127 devices on a single bus
           deviceList == NULL;
}

unsigned long
AIOUSB_GetDeviceByProductID(int minProductID,
                            int maxProductID,
                            int maxDevices,
                            int *deviceList  /* [ 1 + maxDevices * 2 ] */
                            )
{
    if(non_usb_supported_device(minProductID, maxProductID, maxDevices, deviceList))
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    if(!AIOUSB_IsInit()) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
      }

    int index, numDevices = 0;

    for(index = 0; index < MAX_USB_DEVICES && numDevices < maxDevices; index++) {
          if(
              deviceTable[ index ].device != NULL &&
              deviceTable[ index ].ProductID >= ( unsigned )minProductID &&
              deviceTable[ index ].ProductID <= ( unsigned )maxProductID
              ) {
/*
 * deviceList[] contains device index-product ID pairs, one pair per device found
 */
                deviceList[ 1 + numDevices * 2 ] = index;
                deviceList[ 1 + numDevices * 2 + 1 ] = ( int )deviceTable[ index ].ProductID;
                numDevices++;
            }
      }
    deviceList[ 0 ] = numDevices;

    AIOUSB_UnLock();
    return AIOUSB_SUCCESS;
} // AIOUSB_GetDeviceByProductID()


#ifdef __cplusplus
} // namespace AIOUSB
#endif

