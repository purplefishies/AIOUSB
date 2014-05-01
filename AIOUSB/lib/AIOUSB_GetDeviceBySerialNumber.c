/**
 * @file   AIOUSB_GetDeviceBySerialNumber.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 */
#include "AIOUSB_Core.h"
#include <stdio.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

unsigned long GetDeviceBySerialNumber(unsigned long *pSerialNumber) {
    unsigned long deviceIndex = diNone;

    if(pSerialNumber == NULL)
        return deviceIndex;

    if(!AIOUSB_Lock())
        return deviceIndex;

    if(!AIOUSB_IsInit()) {
          AIOUSB_UnLock();
          return deviceIndex;
      }

    int index;
    for(index = 0; index < MAX_USB_DEVICES; index++) {
          if(deviceTable[ index ].device != NULL) {
                AIOUSB_UnLock();                                            // unlock while communicating with device
                __uint64_t deviceSerialNumber;
                const unsigned long result = GetDeviceSerialNumber(index, &deviceSerialNumber);
                AIOUSB_Lock();
                if(
                    result == AIOUSB_SUCCESS &&
                    deviceSerialNumber == *pSerialNumber
                    ) {
                      deviceIndex = index;
                      break;                                                      // from for()
                  }
/*
 * else, even if we get an error requesting the serial number from
 * this device, keep searching
 */
            }
      }

    AIOUSB_UnLock();
    return deviceIndex;
}       // GetDeviceBySerialNumber()


#ifdef __cplusplus
}       // namespace AIOUSB
#endif



/* end of file */
