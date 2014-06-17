/**
 * @file   AIOUSB_Properties.c
 * @author $Author$
 * @date   $Date$
 * @copy
 * @brief ACCES I/O USB Property utilities for Linux. These functions assist with identifying
 *       cards and verifying that the API is in fact operating on the correct type of card.
 *
 * @todo Implement a friendly FindDevices() function as well as FindDeviceByCriteria() 
 *       function to replace all of the standard looping while ( deviceMask != 0 )...
 */

#include "AIOUSB_Core.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
#define RESULT_TEXT_SIZE 40

static const struct ResultCodeName {
    unsigned int result;
    char text[ RESULT_TEXT_SIZE + 2 ];
} resultCodeTable[] = {
                                /* AIOUSB result codes */
    { AIOUSB_SUCCESS                                             , "AIOUSB_SUCCESS"                     },
    { AIOUSB_ERROR_DEVICE_NOT_CONNECTED                          , "AIOUSB_ERROR_DEVICE_NOT_CONNECTED"  },
    { AIOUSB_ERROR_DUP_NAME                                      , "AIOUSB_ERROR_DUP_NAME"              },
    { AIOUSB_ERROR_FILE_NOT_FOUND                                , "AIOUSB_ERROR_FILE_NOT_FOUND"        },
    { AIOUSB_ERROR_INVALID_DATA                                  , "AIOUSB_ERROR_INVALID_DATA"          },
    { AIOUSB_ERROR_INVALID_INDEX                                 , "AIOUSB_ERROR_INVALID_INDEX"         },
    { AIOUSB_ERROR_INVALID_MUTEX                                 , "AIOUSB_ERROR_INVALID_MUTEX"         },
    { AIOUSB_ERROR_INVALID_PARAMETER                             , "AIOUSB_ERROR_INVALID_PARAMETER"     },
    { AIOUSB_ERROR_INVALID_THREAD                                , "AIOUSB_ERROR_INVALID_THREAD"        },
    { AIOUSB_ERROR_NOT_ENOUGH_MEMORY                             , "AIOUSB_ERROR_NOT_ENOUGH_MEMORY"     },
    { AIOUSB_ERROR_NOT_SUPPORTED                                 , "AIOUSB_ERROR_NOT_SUPPORTED"         },
    { AIOUSB_ERROR_OPEN_FAILED                                   , "AIOUSB_ERROR_OPEN_FAILED"           },

                                /* SPECIAL CASE that should not occur, but supposedly can */
    { AIOUSB_ERROR_LIBUSB                                        , "AIOUSB_ERROR_LIBUSB"                },
                                /* LIBUSB result codes */
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_IO)            , "LIBUSB_ERROR_IO"                    },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_INVALID_PARAM) , "LIBUSB_ERROR_INVALID_PARAM"         },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_ACCESS)        , "LIBUSB_ERROR_ACCESS"                },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_NO_DEVICE)     , "LIBUSB_ERROR_NO_DEVICE"             },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_NOT_FOUND)     , "LIBUSB_ERROR_NOT_FOUND"             },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_BUSY)          , "LIBUSB_ERROR_BUSY"                  },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_TIMEOUT)       , "LIBUSB_ERROR_TIMEOUT"               },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_OVERFLOW)      , "LIBUSB_ERROR_OVERFLOW"              },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_PIPE)          , "LIBUSB_ERROR_PIPE"                  },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_INTERRUPTED)   , "LIBUSB_ERROR_INTERRUPTED"           },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_NO_MEM)        , "LIBUSB_ERROR_NO_MEM"                },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_NOT_SUPPORTED) , "LIBUSB_ERROR_NOT_SUPPORTED"         },
    { LIBUSB_RESULT_TO_AIOUSB_RESULT(LIBUSB_ERROR_OTHER)         , "LIBUSB_ERROR_OTHER"                 }
};

#ifdef __cplusplus
const int NUM_RESULT_CODES = sizeof(resultCodeTable) / sizeof(resultCodeTable[ 0 ]);
#else
#define NUM_RESULT_CODES (sizeof(resultCodeTable) / sizeof(resultCodeTable[ 0 ]))
#endif

/*----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------*/
/**
 * @param minProductID 
 * @param maxProductID 
 * @param maxDevices 
 * @param deviceList  [ 1 + maxDevices * 2 ] 
 * @return 
 */
unsigned long AIOUSB_GetDeviceByProductID(int minProductID,
                                          int maxProductID,
                                          int maxDevices,
                                          int *deviceList
                                          ) {
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
        if( deviceTable[ index ].device != NULL &&
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
}
/*----------------------------------------------------------------------------*/
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
                unsigned long deviceSerialNumber;
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
}
/*----------------------------------------------------------------------------*/
/**
 * @brief AIOUSB_GetDeviceProperties() returns a richer amount of information 
 * than QueryDeviceInfo()
 */
unsigned long AIOUSB_GetDeviceProperties(unsigned long DeviceIndex,DeviceProperties *properties ) {
    if(properties == 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;
    
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    properties->Name = deviceDesc->cachedName; /* if NULL, name will be requested from device */
    properties->SerialNumber = deviceDesc->cachedSerialNumber; /* if 0, serial number will be requested from device */
    properties->ProductID = deviceDesc->ProductID;
    properties->DIOPorts = deviceDesc->DIOBytes;
    properties->Counters = deviceDesc->Counters;
    properties->Tristates = deviceDesc->Tristates;
    properties->RootClock = deviceDesc->RootClock;
    properties->DACChannels = deviceDesc->ImmDACs;
    properties->ADCChannels = deviceDesc->ADCChannels;
    properties->ADCMUXChannels = deviceDesc->ADCMUXChannels;
    properties->ADCChannelsPerGroup = deviceDesc->ADCChannelsPerGroup;
    AIOUSB_UnLock();                                                            // unlock while communicating with device
    if(properties->Name == NULL)
        properties->Name = GetSafeDeviceName(DeviceIndex);
    if(properties->SerialNumber == 0)
        result = GetDeviceSerialNumber(DeviceIndex, &properties->SerialNumber);

    return result;
}
/*----------------------------------------------------------------------------*/
static int CompareResultCodes(const void *p1, const void *p2)
{
    assert(p1 != 0 &&
           (*( struct ResultCodeName** )p1) != 0 &&
           p2 != 0 &&
           (*( struct ResultCodeName** )p2) != 0);
    const unsigned int result1 = (*( struct ResultCodeName** )p1)->result,
                       result2 = (*( struct ResultCodeName** )p2)->result;
    if(result1 < result2)
        return -1;
    else if(result1 > result2)
        return 1;
    else
        return 0;
}
/*----------------------------------------------------------------------------*/
const char *AIOUSB_GetResultCodeAsString(unsigned long result_value)
{
    const char *resultText = "UNKNOWN";

    if(AIOUSB_Lock()) {
        /*
         * resultCodeIndex[] represents an index into resultCodeTable[], sorted by result code;
         * specifically, it contains pointers into resultCodeTable[]; to get the actual result
         * code, the pointer in resultCodeIndex[] must be dereferenced
         */
          static struct ResultCodeName const *resultCodeIndex[ NUM_RESULT_CODES ];  /* index of result codes in resultCodeTable[] */
          const unsigned long INIT_PATTERN = 0x100c48b9ul;                          /* random pattern */
          static unsigned long resultCodeIndexCreated = 0;                          /* == INIT_PATTERN if index has been created */
          if(resultCodeIndexCreated != INIT_PATTERN) {
                                /*
                                 * build index of result codes
                                 */
                int index;
                for(index = 0; index < NUM_RESULT_CODES; index++)
                    resultCodeIndex[ index ] = &resultCodeTable[ index ];
                qsort(resultCodeIndex, NUM_RESULT_CODES, sizeof(struct ResultCodeName *), CompareResultCodes);
                resultCodeIndexCreated = INIT_PATTERN;
            }

          struct ResultCodeName key;                                  // key.name not used
          key.result = result_value;
          const struct ResultCodeName *const pKey = &key;
          const struct ResultCodeName **resultCode = ( const struct ResultCodeName** )bsearch(&pKey, 
                                                                                              resultCodeIndex, 
                                                                                              NUM_RESULT_CODES, 
                                                                                              sizeof(struct ResultCodeName *), 
                                                                                              CompareResultCodes
                                                                                              );
          if(resultCode != 0)
              resultText = (*resultCode)->text;
          AIOUSB_UnLock();
      }
    return resultText;
}
/*----------------------------------------------------------------------------*/
void AIOUSB_ListDevices() {
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
                            AIOUSB_UnLock();                               /* unlock while communicating with device */
                            const unsigned long result = QueryDeviceInfo(index, &productID, &nameSize, name, &numDIOBytes, &numCounters);
                            if(result == AIOUSB_SUCCESS) {
                                  name[ nameSize ] = '\0';
                                  if(!found) {
                                     /* print a heading before the first device found */
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
}
#endif
