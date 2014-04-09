/**
 * @file   AIOTypes.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  Provides a simple function for listing ACCES I/O USB devices on USB bus
 *
 */

#include "AIOUSB_Core.h"
#include <stdio.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif


PUBLIC_EXTERN void AIOUSB_ListDevices() {
    AIOUSB_BOOL found = AIOUSB_FALSE;

    if(AIOUSB_Lock()) {
          if(AIOUSB_IsInit()) {
                int index;
                for(index = 0; index < MAX_USB_DEVICES; index++) {
                      if(deviceTable[ index ].device != NULL) {
                            const int MAX_NAME_SIZE = 100;
                            char name[ MAX_NAME_SIZE + 1 ];
                            unsigned long productID;
                            unsigned long nameSize = MAX_NAME_SIZE;
                            unsigned long numDIOBytes;
                            unsigned long numCounters;
                            AIOUSB_UnLock();                              // unlock while communicating with device
                            const unsigned long result = QueryDeviceInfo(index, &productID, &nameSize, name, &numDIOBytes, &numCounters);
                            if(result == AIOUSB_SUCCESS) {
                                  name[ nameSize ] = '\0';
                                  if(!found) {
                                    // print a heading before the first device found
                                        printf("ACCES devices found:\n");
                                        found = AIOUSB_TRUE;
                                    }
                                  printf(
                                      "  Device at index %d:\n"
                                      "    Product ID: %#lx\n"
                                      "    Product name: %s\n"
                                      "    Number of digital I/O bytes: %lu\n"
                                      "    Number of counters: %lu\n",
                                      index,
                                      productID,
                                      name,
                                      numDIOBytes,
                                      numCounters
                                      );
                              }

                            AIOUSB_Lock();
                        }
                  }
            }
          AIOUSB_UnLock();
      }
    if(!found)
        printf("No ACCES devices found\n");
}


#ifdef __cplusplus
} /* namespace AIOUSB */
#endif


