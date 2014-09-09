/**
 * @file   AIOUSB_Core.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  General header files for the AIOUSB library
 *
 */

#include "AIOUSB_Core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb.h>

#ifdef BACKTRACE
#include <execinfo.h>

#define BACKTRACE_DEBUG(n) do { \
          int j, nptrs; \
          void *buffer[100]; \
          char **strings; \
          nptrs = backtrace(buffer, 100); \
          nptrs = (nptrs > n ? n : nptrs); \
          strings = backtrace_symbols(buffer, nptrs); \
          for(j = 0; j < nptrs; j++) \
              printf("%s\n", strings[j]); \
  } while(0);
#else
#define BACKTRACE_DEBUG(n) {}
#endif


#ifdef __cplusplus
namespace AIOUSB {
#endif

int aio_errno;

static const ProductIDName productIDNameTable[] = {
    { USB_DA12_8A_REV_A , "USB-DA12-8A-A"  },
    { USB_DA12_8A       , "USB-DA12-8A"    },
    { USB_DA12_8E       , "USB-DA12-8E"    },
    { USB_DIO_32        , "USB-DIO-32"     },
    { USB_DIO_32I       , "USB-DIO-32"     },
    { USB_DIO_48        , "USB-DIO-48"     },
    { USB_DIO_96        , "USB-DIO-96"     },
    { USB_DI16A_REV_A1  , "USB-DI16A-A1"   },
    { USB_DO16A_REV_A1  , "USB-DO16A-A1"   },
    { USB_DI16A_REV_A2  , "USB-DI16A-A2"   },
    { USB_DIO_16H       , "USB-DIO-16H"    },
    { USB_DI16A         , "USB-DI16A"      },
    { USB_DO16A         , "USB-DO16A"      },
    { USB_DIO_16A       , "USB-DIO-16A"    },
    { USB_IIRO_16       , "USB-IIRO-16"    },
    { USB_II_16         , "USB-II-16"      },
    { USB_RO_16         , "USB-RO-16"      },
    { USB_IIRO_8        , "USB-IIRO-8"     },
    { USB_II_8          , "USB-II-8"       },
    { USB_IIRO_4        , "USB-IIRO-4"     },
    { USB_IDIO_16       , "USB-IDIO-16"    },
    { USB_II_16_OLD     , "USB-II-16-OLD"  },
    { USB_IDO_16        , "USB-IDO-16"     },
    { USB_IDIO_8        , "USB-IDIO-8"     },
    { USB_II_8_OLD      , "USB-II-8-OLD"   },
    { USB_IDIO_4        , "USB-IDIO-4"     },
    { USB_CTR_15        , "USB-CTR-15"     },
    { USB_IIRO4_2SM     , "USB-IIRO4-2SM"  },
    { USB_IIRO4_COM     , "USB-IIRO4-COM"  },
    { USB_DIO16RO8      , "USB-DIO16RO8"   },
    { PICO_DIO16RO8     , "PICO-DIO16RO8"  },
    { USBP_II8IDO4A     , "USBP-II8IDO4A"  },
    { USB_AI16_16A      , "USB-AI16-16A"   },
    { USB_AI16_16E      , "USB-AI16-16E"   },
    { USB_AI12_16A      , "USB-AI12-16A"   },
    { USB_AI12_16       , "USB-AI12-16"    },
    { USB_AI12_16E      , "USB-AI12-16E"   },
    { USB_AI16_64MA     , "USB-AI16-64MA"  },
    { USB_AI16_64ME     , "USB-AI16-64ME"  },
    { USB_AI12_64MA     , "USB-AI12-64MA"  },
    { USB_AI12_64M      , "USB-AI12-64M"   },
    { USB_AI12_64ME     , "USB-AI12-64ME"  },
    { USB_AI16_32A      , "USB-AI16-32A"   },
    { USB_AI16_32E      , "USB-AI16-32E"   },
    { USB_AI12_32A      , "USB-AI12-32A"   },
    { USB_AI12_32       , "USB-AI12-32"    },
    { USB_AI12_32E      , "USB-AI12-32E"   },
    { USB_AI16_64A      , "USB-AI16-64A"   },
    { USB_AI16_64E      , "USB-AI16-64E"   },
    { USB_AI12_64A      , "USB-AI12-64A"   },
    { USB_AI12_64       , "USB-AI12-64"    },
    { USB_AI12_64E      , "USB-AI12-64E"   },
    { USB_AI16_96A      , "USB-AI16-96A"   },
    { USB_AI16_96E      , "USB-AI16-96E"   },
    { USB_AI12_96A      , "USB-AI12-96A"   },
    { USB_AI12_96       , "USB-AI12-96"    },
    { USB_AI12_96E      , "USB-AI12-96E"   },
    { USB_AI16_128A     , "USB-AI16-128A"  },
    { USB_AI16_128E     , "USB-AI16-128E"  },
    { USB_AI12_128A     , "USB-AI12-128A"  },
    { USB_AI12_128      , "USB-AI12-128"   },
    { USB_AI12_128E     , "USB-AI12-128E"  },
    { USB_AO16_16A      , "USB-AO16-16A"   },
    { USB_AO16_16       , "USB-AO16-16"    },
    { USB_AO16_12A      , "USB-AO16-12A"   },
    { USB_AO16_12       , "USB-AO16-12"    },
    { USB_AO16_8A       , "USB-AO16-8A"    },
    { USB_AO16_8        , "USB-AO16-8"     },
    { USB_AO16_4A       , "USB-AO16-4A"    },
    { USB_AO16_4        , "USB-AO16-4"     },
    { USB_AO12_16A      , "USB-AO12-16A"   },
    { USB_AO12_16       , "USB-AO12-16"    },
    { USB_AO12_12A      , "USB-AO12-12A"   },
    { USB_AO12_12       , "USB-AO12-12"    },
    { USB_AO12_8A       , "USB-AO12-8A"    },
    { USB_AO12_8        , "USB-AO12-8"     },
    { USB_AO12_4A       , "USB-AO12-4A"    },
    { USB_AO12_4        , "USB-AO12-4"     },
    { USB_AIO16_16A     , "USB-AIO16-16A"  },
    { USB_AIO16_16E     , "USB-AIO16-16E"  },
    { USB_AIO12_16A     , "USB-AIO12-16A"  },
    { USB_AIO12_16      , "USB-AIO12-16"   },
    { USB_AIO12_16E     , "USB-AIO12-16E"  },
    { USB_AIO16_64MA    , "USB-AIO16-64MA" },
    { USB_AIO16_64ME    , "USB-AIO16-64ME" },
    { USB_AIO12_64MA    , "USB-AIO12-64MA" },
    { USB_AIO12_64M     , "USB-AIO12-64M"  },
    { USB_AIO12_64ME    , "USB-AIO12-64ME" },
    { USB_AIO16_32A     , "USB-AIO16-32A"  },
    { USB_AIO16_32E     , "USB-AIO16-32E"  },
    { USB_AIO12_32A     , "USB-AIO12-32A"  },
    { USB_AIO12_32      , "USB-AIO12-32"   },
    { USB_AIO12_32E     , "USB-AIO12-32E"  },
    { USB_AIO16_64A     , "USB-AIO16-64A"  },
    { USB_AIO16_64E     , "USB-AIO16-64E"  },
    { USB_AIO12_64A     , "USB-AIO12-64A"  },
    { USB_AIO12_64      , "USB-AIO12-64"   },
    { USB_AIO12_64E     , "USB-AIO12-64E"  },
    { USB_AIO16_96A     , "USB-AIO16-96A"  },
    { USB_AIO16_96E     , "USB-AIO16-96E"  },
    { USB_AIO12_96A     , "USB-AIO12-96A"  },
    { USB_AIO12_96      , "USB-AIO12-96"   },
    { USB_AIO12_96E     , "USB-AIO12-96E"  },
    { USB_AIO16_128A    , "USB-AIO16-128A" },
    { USB_AIO16_128E    , "USB-AIO16-128E" },
    { USB_AIO12_128A    , "USB-AIO12-128A" },
    { USB_AIO12_128     , "USB-AIO12-128"  },
    { USB_AIO12_128E    , "USB-AIO12-128E" }
};
#ifdef __cplusplus
const int NUM_PROD_NAMES = sizeof(productIDNameTable) / sizeof(productIDNameTable[ 0 ]);
#else
#define NUM_PROD_NAMES (sizeof(productIDNameTable) / sizeof(productIDNameTable[ 0 ]))
#endif


#define AIOUSB_ENABLE_MUTEX

PRIVATE DeviceDescriptor deviceTable[ MAX_USB_DEVICES ];
#if defined(AIOUSB_ENABLE_MUTEX)
static pthread_mutex_t aiousbMutex;
#endif


unsigned long AIOUSB_INIT_PATTERN = 0x9b6773adul;       // random pattern
unsigned long aiousbInit = 0;                   // == AIOUSB_INIT_PATTERN if AIOUSB module is initialized

static const char VERSION_NUMBER[] = "$Format: %t$";
static const char VERSION_DATE[] = "$Format: %ad$";


/**
 * @brief
 * Notes on mutual exclusion / threading:
 * - Our mutual exclusion scheme is _not_ intended to be bulletproof. It's primarily intended
 *   to ensure mutually exclusive access to deviceTable[] and other global variables. It does
 *   NOT ensure mutually exclusive access to the USB bus. In fact, we want to permit threads
 *   to communicate with multiple devices simultaneously, to the extent possible with USB.
 *
 * - Nor does this scheme prevent multiple threads from altering the configuration of the same
 *   device or communicating with the same device. In other words, it's entirely possible for
 *   one thread to configure and communicate with a device, only to have another thread come
 *   along and to the same. It's up to the users of this library to ensure that such a scenario
 *   doesn't occur.
 *
 * - This library does seek to permit one thread to control one device, and another thread to
 *   control another device. Each thread may then safely communicate with its own device and
 *   alter the portion of deviceTable[] that pertains to its device.
 *
 * - Our mutual exclusion scheme also permits two threads to cooperate in the operation of a
 *   single device, such as in cases where a background thread does the actual work and the
 *   foreground thread monitors the progress. In such a case, the background thread might update
 *   a status variable which the foreground thread monitors. This form of resource sharing is
 *   supported by our mutual exclusion scheme.
 */

AIOUSB_BOOL AIOUSB_Lock() {
    assert(AIOUSB_IsInit());
#if defined(AIOUSB_ENABLE_MUTEX)
    return(pthread_mutex_lock(&aiousbMutex) == 0);
#else
    return AIOUSB_TRUE;
#endif
}

AIOUSB_BOOL AIOUSB_UnLock() {
    assert(AIOUSB_IsInit());
#if defined(AIOUSB_ENABLE_MUTEX)
    return(pthread_mutex_unlock(&aiousbMutex) == 0);
#else
    return AIOUSB_TRUE;
#endif
}

 
unsigned long AIOUSB_Validate_Lock(unsigned long *DeviceIndex)
{
    unsigned long result = (unsigned long)AIOUSB_SUCCESS;
    assert(DeviceIndex != 0);

    if ( !AIOUSB_Lock() )
        return AIOUSB_ERROR_INVALID_MUTEX;

    if (!AIOUSB_IsInit() )
        goto unlock_mutex;

    if(*DeviceIndex == diFirst) {
        /*
         * find first device on bus
         */
        result = AIOUSB_ERROR_FILE_NOT_FOUND;
        int index;
        for(index = 0; index < MAX_USB_DEVICES; index++) {
            if(deviceTable[ index ].device != NULL) {
                *DeviceIndex = index;
                result = AIOUSB_SUCCESS;
                break;                                              // from for()
            }
        }
    } else if (*DeviceIndex == diOnly) {
        /*
         * find first device on bus, ensuring that it's the only device
         */
        result = AIOUSB_ERROR_FILE_NOT_FOUND;
        int index;
        for(index = 0; index < MAX_USB_DEVICES; index++) {
            if(deviceTable[ index ].device != NULL) {
                /* found a device */
                if(result != AIOUSB_SUCCESS) {
                    /*
                     * this is the first device found; save this index, but
                     * keep checking to see that this is the only device
                     */
                    *DeviceIndex = index;
                    result = AIOUSB_SUCCESS;
                } else {
                    /*
                     * there are multiple devices on the bus
                     */
                    result = AIOUSB_ERROR_DUP_NAME;
                    break;                             
                }
            }
        }
    } else {
        /*
         * simply verify that the supplied index is valid
         */
        if(
           *DeviceIndex < MAX_USB_DEVICES &&
           deviceTable[ *DeviceIndex ].device != NULL
           )
            result = AIOUSB_SUCCESS;
        else
            result = AIOUSB_ERROR_INVALID_INDEX;
    }

 unlock_mutex:
    AIOUSB_UnLock();
    return result;
}


unsigned long AIOUSB_Validate(unsigned long *DeviceIndex) {
    assert(DeviceIndex != 0);
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_ERROR_INVALID_INDEX;
    if ( !AIOUSB_IsInit() ) {
          AIOUSB_UnLock();
          return result;
      }

    if(*DeviceIndex == diFirst) {
      /*
       * find first device on bus
       */
        result = AIOUSB_ERROR_FILE_NOT_FOUND;
        int index;
        for(index = 0; index < MAX_USB_DEVICES; index++) {
            if(deviceTable[ index ].device != NULL) {
                *DeviceIndex = index;
                result = AIOUSB_SUCCESS;
                break;                                                      // from for()
            }
        }
    } else if(*DeviceIndex == diOnly) {
      /*
       * find first device on bus, ensuring that it's the only device
       */
          result = AIOUSB_ERROR_FILE_NOT_FOUND;
          int index;
          for(index = 0; index < MAX_USB_DEVICES; index++) {
                if(deviceTable[ index ].device != NULL) {
                  /* found a device */
                      if(result != AIOUSB_SUCCESS) {
                        /*
                         * this is the first device found; save this index, but
                         * keep checking to see that this is the only device
                         */
                        *DeviceIndex = index;
                        result = AIOUSB_SUCCESS;
                      } else {
                        /*
                         * there are multiple devices on the bus
                         */
                            result = AIOUSB_ERROR_DUP_NAME;
                            break;
                        }
                }
          }
    } else {
      /*
       * simply verify that the supplied index is valid
       */
      if(
         *DeviceIndex < MAX_USB_DEVICES &&
         deviceTable[ *DeviceIndex ].device != NULL
         )
        result = AIOUSB_SUCCESS;
      else
        result = AIOUSB_ERROR_INVALID_INDEX;
    }
    
    AIOUSB_UnLock();
    return result;
}



PRIVATE static void InitDeviceTable(void)
{
    int index;

    for(index = 0; index < MAX_USB_DEVICES; index++) {
          DeviceDescriptor *deviceDesc = &deviceTable[ index ];

          /* libusb handles */
          deviceDesc->device = NULL;
          deviceDesc->deviceHandle = NULL;

          /* run-time settings */
          deviceDesc->discardFirstSample = AIOUSB_FALSE;
          deviceDesc->commTimeout = 5000;
          deviceDesc->miscClockHz = 1;

          /* device-specific properties */
          deviceDesc->ProductID = 0;
          deviceDesc->DIOBytes
              = deviceDesc->Counters
                    = deviceDesc->Tristates
                          = deviceDesc->ConfigBytes
                                = deviceDesc->ImmDACs
                                      = deviceDesc->DACsUsed
                                            = deviceDesc->ADCChannels
                                                  = deviceDesc->ADCMUXChannels
                                                        = deviceDesc->ADCChannelsPerGroup
                                                              = deviceDesc->WDGBytes
                                                                    = deviceDesc->ImmADCs
                                                                          = deviceDesc->FlashSectors
                                                                                = 0;
          deviceDesc->RootClock
              = deviceDesc->StreamingBlockSize
                    = 0;
          deviceDesc->bGateSelectable
              = deviceDesc->bGetName
                    = deviceDesc->bDACStream
                          = deviceDesc->bADCStream
                                = deviceDesc->bDIOStream
                                      = deviceDesc->bDIOSPI
                                            = deviceDesc->bClearFIFO
                                                  = deviceDesc->bDACBoardRange
                                                        = deviceDesc->bDACChannelCal
                                                              = AIOUSB_FALSE;

          /* device state */
          deviceDesc->bDACOpen
              = deviceDesc->bDACClosing
                    = deviceDesc->bDACAborting
                          = deviceDesc->bDACStarted
                                = deviceDesc->bDIOOpen
                                      = deviceDesc->bDIORead
                                            = AIOUSB_FALSE;
          deviceDesc->DACData = NULL;
          deviceDesc->PendingDACData = NULL;
          deviceDesc->LastDIOData = NULL;
          deviceDesc->cachedName = NULL;
          deviceDesc->cachedSerialNumber = 0;
          deviceDesc->cachedConfigBlock.size = 0;       // .size == 0 == uninitialized

          /* worker thread state */
          deviceDesc->workerBusy = AIOUSB_FALSE;
          deviceDesc->workerStatus = 0;
          deviceDesc->workerResult = AIOUSB_SUCCESS;
      }
}
 


void _setup_device_parameters( DeviceDescriptor *deviceDesc , unsigned long productID ) {
    deviceDesc->StreamingBlockSize = 31ul * 1024ul;
    deviceDesc->bGetName = AIOUSB_TRUE;             // most boards support this feature
    if(productID == USB_DIO_32) {
        deviceDesc->DIOBytes = 4;
        deviceDesc->Counters = 3;
        deviceDesc->RootClock = 3000000;
    } else if(productID == USB_DIO_48) {
        deviceDesc->DIOBytes = 6;
    } else if(productID == USB_DIO_96) {
        deviceDesc->DIOBytes = 12;
    } else if( productID >= USB_DI16A_REV_A1 && productID <= USB_DI16A_REV_A2 ) {
        deviceDesc->DIOBytes = 1;
        deviceDesc->bDIOStream = AIOUSB_TRUE;
        deviceDesc->bDIOSPI = AIOUSB_TRUE;
        deviceDesc->bClearFIFO = AIOUSB_TRUE;
    } else if(
              productID >= USB_DIO_16H &&
              productID <= USB_DIO_16A
              ) {
        deviceDesc->DIOBytes = 4;
        deviceDesc->Tristates = 2;
        deviceDesc->bDIOStream = AIOUSB_TRUE;
        deviceDesc->bDIOSPI = AIOUSB_TRUE;
        deviceDesc->bClearFIFO = AIOUSB_TRUE;
    } else if(
              productID == USB_IIRO_16 ||
              productID == USB_II_16 ||
              productID == USB_RO_16 ||
              productID == USB_IIRO_8 ||
              productID == USB_II_8 ||
              productID == USB_IIRO_4
              ) {
        deviceDesc->DIOBytes = 4;
        deviceDesc->WDGBytes = 2;
    } else if(
              productID == USB_IDIO_16 ||
              productID == USB_II_16_OLD ||
              productID == USB_IDO_16 ||
              productID == USB_IDIO_8 ||
              productID == USB_II_8_OLD ||
              productID == USB_IDIO_4
              ) {
        deviceDesc->DIOBytes = 4;
        deviceDesc->WDGBytes = 2;
    } else if(
              productID >= USB_DA12_8A_REV_A &&
              productID <= USB_DA12_8A
              ) {
        deviceDesc->bDACStream = AIOUSB_TRUE;
        deviceDesc->ImmDACs = 8;
        deviceDesc->DACsUsed = 5;
        deviceDesc->bGetName = AIOUSB_FALSE;
        deviceDesc->RootClock = 12000000;
    } else if(productID == USB_DA12_8E) {
        deviceDesc->ImmDACs = 8;
        deviceDesc->bGetName = AIOUSB_FALSE;
    } else if(productID == USB_CTR_15) {
        deviceDesc->Counters = 5;
        deviceDesc->bGateSelectable = AIOUSB_TRUE;
        deviceDesc->RootClock = 10000000;
    } else if(
              productID == USB_IIRO4_2SM ||
              productID == USB_IIRO4_COM
              ) {
        deviceDesc->DIOBytes = 2;
    } else if(productID == USB_DIO16RO8) {
        deviceDesc->DIOBytes = 3;
    } else if(productID == PICO_DIO16RO8) {
        deviceDesc->DIOBytes = 3;
    } else if(
              (productID >= USB_AI16_16A && productID <= USB_AI12_16E) ||
              (productID >= USB_AIO16_16A && productID <= USB_AIO12_16E)
              ) {
        deviceDesc->DIOBytes = 2;
        deviceDesc->Counters = 1;
        deviceDesc->RootClock = 10000000;
        deviceDesc->bADCStream = AIOUSB_TRUE;
        deviceDesc->ImmADCs = 1;
        deviceDesc->ADCChannels
            = deviceDesc->ADCMUXChannels = 16;
        deviceDesc->ADCChannelsPerGroup = 1;
        deviceDesc->ConfigBytes = AD_CONFIG_REGISTERS;
        deviceDesc->bClearFIFO = AIOUSB_TRUE;
        if(productID & 0x0100) {
            deviceDesc->ImmDACs = 2;
            deviceDesc->bDACBoardRange = AIOUSB_TRUE;
        }
    } else if(
              (productID >= USB_AI16_64MA && productID <= USB_AI12_64ME) ||
              (productID >= USB_AIO16_64MA && productID <= USB_AIO12_64ME)
              ) {
        deviceDesc->DIOBytes = 2;
        deviceDesc->Counters = 1;
        deviceDesc->RootClock = 10000000;
        deviceDesc->bADCStream = AIOUSB_TRUE;
        deviceDesc->ImmADCs = 1;
        deviceDesc->ADCChannels = 16;
        deviceDesc->ADCMUXChannels = 64;
        deviceDesc->ADCChannelsPerGroup = 4;
        deviceDesc->ConfigBytes = AD_MUX_CONFIG_REGISTERS;
        deviceDesc->bClearFIFO = AIOUSB_TRUE;
        if(productID & 0x0100) {
            deviceDesc->ImmDACs = 2;
            deviceDesc->bDACBoardRange = AIOUSB_TRUE;
        }
    } else if(
              (productID >= USB_AI16_32A && productID <= USB_AI12_128E) ||
              (productID >= USB_AIO16_32A && productID <= USB_AIO12_128E)
              ) {
        deviceDesc->DIOBytes = 2;
        deviceDesc->Counters = 1;
        deviceDesc->RootClock = 10000000;
        deviceDesc->bADCStream = AIOUSB_TRUE;
        deviceDesc->ImmADCs = 1;
        deviceDesc->ADCChannels = 16;

        /*
         * there are four groups of five
         * products each in this family;
         * each group of five products has
         * 32 more MUX channels than the
         * preceding group; so we calculate
         * the number of MUX channels by
         * doing some arithmetic on the
         * product ID
         */

        int I = (productID - USB_AI16_32A) & ~0x0100;
        I /= 5;               /* products per group */
        deviceDesc->ADCMUXChannels = 32 * (I + 1);
        deviceDesc->ADCChannelsPerGroup = 8;
        deviceDesc->ConfigBytes = AD_MUX_CONFIG_REGISTERS;
        deviceDesc->bClearFIFO = AIOUSB_TRUE;
        if(productID & 0x0100) {
            deviceDesc->ImmDACs = 2;
            deviceDesc->bDACBoardRange = AIOUSB_TRUE;
        }
    } else if(
              productID >= USB_AO16_16A &&
              productID <= USB_AO12_4
              ) {
        deviceDesc->DIOBytes = 2;
        deviceDesc->FlashSectors = 32;
        deviceDesc->bDACBoardRange = AIOUSB_TRUE;
        deviceDesc->bDACChannelCal = AIOUSB_TRUE;

        /*
         * we use a few bits within the
         * product ID to determine the
         * number of DACs and whether or not
         * the product has ADCs
         */
        switch(productID & 0x0006) {
        case 0x0000:
            deviceDesc->ImmDACs = 16;
            break;

        case 0x0002:
            deviceDesc->ImmDACs = 12;
            break;

        case 0x0004:
            deviceDesc->ImmDACs = 8;
            break;

        case 0x0006:
            deviceDesc->ImmDACs = 4;
            break;
        }
        if((productID & 0x0001) == 0)
            deviceDesc->ImmADCs = 2;
    }

    /* allocate I/O image buffers */
    if(deviceDesc->DIOBytes > 0) {
        /* calloc() zeros memory */
        deviceDesc->LastDIOData = ( unsigned char* )calloc(deviceDesc->DIOBytes, sizeof(unsigned char));
        assert(deviceDesc->LastDIOData != 0);
    }
}

/**
 * @brief A mock function that can set up the DeviceTable with any type of devices
 *
 *
 **/
void AddDeviceToDeviceTable( int *numAccesDevices, unsigned long productID ) {
    DeviceDescriptor *deviceDesc = &deviceTable[ (*numAccesDevices)++ ];
    deviceDesc->device        = NULL;
    deviceDesc->deviceHandle  = NULL;
    deviceDesc->ProductID     = productID;
    _setup_device_parameters( deviceDesc , productID );
}

void PopulateDeviceTableTest(unsigned long *products, int length ) {
    int numAccesDevices = 0;
    AIOUSB_InitTest();    
    for( int i = 0; i < length ; i ++ ) {
        AddDeviceToDeviceTable( &numAccesDevices, products[i] );
        deviceTable[numAccesDevices-1].device = (libusb_device *)0x42;
    }
}





/** 
 * @todo Rely on Global Header files for the functionality of devices / cards
 * as opposed to hard coding
 */
void PopulateDeviceTable(void) {
  /*
   * populate device table with ACCES devices found on USB bus
   */
    if(!AIOUSB_IsInit())
        return;
    int numAccesDevices = 0;
    libusb_device **deviceList;
    const int numDevices = libusb_get_device_list(NULL, &deviceList);
    if(numDevices > 0) {
          int index;
          for(index = 0; index < numDevices && numAccesDevices < MAX_USB_DEVICES; index++) {
                struct libusb_device_descriptor libusbDeviceDesc;
                libusb_device *const device = deviceList[ index ];
                assert(device != 0);
                const int libusbResult = libusb_get_device_descriptor(device, &libusbDeviceDesc);
                if(libusbResult == LIBUSB_SUCCESS) {
                      if(libusbDeviceDesc.idVendor == ACCES_VENDOR_ID) {
                        /* add this device to the device table */
                            DeviceDescriptor *deviceDesc = &deviceTable[ numAccesDevices++ ];
                            deviceDesc->device = libusb_ref_device(device);
                            deviceDesc->deviceHandle = NULL;

                            /* set up device-specific properties */
                            unsigned productID = deviceDesc->ProductID = libusbDeviceDesc.idProduct;

                            _setup_device_parameters( deviceDesc , productID );
                        }
                  }
            }
      }
    libusb_free_device_list(deviceList, AIOUSB_TRUE);
}



static void CloseAllDevices(void) {
    if(!AIOUSB_IsInit())
        return;
    int index;
    for(index = 0; index < MAX_USB_DEVICES; index++) {
          DeviceDescriptor *deviceDesc = &deviceTable[ index ];

          if(deviceDesc->deviceHandle != NULL) {
                libusb_close(deviceDesc->deviceHandle);
                deviceDesc->deviceHandle = NULL;
            }
          libusb_unref_device(deviceDesc->device);

          if(deviceDesc->LastDIOData != NULL) {
                free(deviceDesc->LastDIOData);
                deviceDesc->LastDIOData = NULL;
            }

          if(deviceDesc->cachedName != NULL) {
                free(deviceDesc->cachedName);
                deviceDesc->cachedName = NULL;
            }
      }
}




static int CompareProductIDs(const void *p1, const void *p2)
{
    assert(p1 != 0 &&
           (*( ProductIDName** )p1) != 0 &&
           p2 != 0 &&
           (*( ProductIDName** )p2) != 0);
    const unsigned int productID1 = (*( ProductIDName** )p1)->id,
                       productID2 = (*( ProductIDName** )p2)->id;
    if(productID1 < productID2)
        return -1;
    else if(productID1 > productID2)
        return 1;
    else
        return 0;
}

/**
 * @brief this function returns the name of a product ID; generally,
 * it's best to use this only as a last resort, since most
 * devices return their name when asked in QueryDeviceInfo()
 */
PRIVATE const char *ProductIDToName(unsigned int productID) {


    const char *name = "UNKNOWN";

    if(AIOUSB_Lock()) {
      /*
       * productIDIndex[] represents an index into
       * productIDNameTable[], sorted by product ID;
       * specifically, it contains pointers into
       * productIDNameTable[]; to get the actual product ID,
       * the pointer in productIDIndex[] must be
       * dereferenced; using a separate index table instead
       * of sorting productIDNameTable[] directly permits us
       * to create multiple indexes, in particular, a second
       * index sorted by product name
       */


      /* index of product IDs in productIDNameTable[] */
          static ProductIDName const *productIDIndex[ NUM_PROD_NAMES ];
          /* random pattern */
          const unsigned long INIT_PATTERN = 0xe697f8acul;

          /* == INIT_PATTERN if index has been created */
          static unsigned long productIDIndexCreated = 0;
          if(productIDIndexCreated != INIT_PATTERN) {
            /* build index of product IDs */
                int index;
                for(index = 0; index < NUM_PROD_NAMES; index++)
                    productIDIndex[ index ] = &productIDNameTable[ index ];
                qsort(productIDIndex, NUM_PROD_NAMES, sizeof(ProductIDName *), CompareProductIDs);
                productIDIndexCreated = INIT_PATTERN;
            }

          ProductIDName key;
          key.id = productID;
          const ProductIDName *const pKey = &key;
          const ProductIDName **product
              = ( const ProductIDName** )bsearch(&pKey, productIDIndex, NUM_PROD_NAMES, sizeof(ProductIDName *), CompareProductIDs);
          if(product != 0)
              name = (*product)->name;
          AIOUSB_UnLock();
      }
    return name;
}



static int CompareProductNames(const void *p1, const void *p2) {
    assert(p1 != 0 &&
           (*( ProductIDName** )p1) != 0 &&
           p2 != 0 &&
           (*( ProductIDName** )p2) != 0);
    return strcmp((*( ProductIDName** )p1)->name, (*( ProductIDName** )p2)->name);
}

/**
 * @details This function is the complement of ProductIDToName() and
 * returns the product ID for a given name; this function should be
 * used with care; it will work reliably if passed a name obtained
 * from ProductIDToName(); however, if passed a name obtained from the
 * device itself it may not work; the reason is that devices contain
 * their own name strings, which are most likely identical to the
 * names defined in this module, but not guaranteed to be so; that's
 * not as big a problem as it sounds, however, because if one has the
 * means to obtain the name from the device, then they also have
 * access to the device's product ID, so calling this function is
 * unnecessary; this function is mainly for performing simple
 * conversions between product names and IDs, primarily to support
 * user interfaces
 *
 * @param name
 *
 * @return
 */
PRIVATE unsigned int ProductNameToID(const char *name) {
    assert(name != 0);

    unsigned int productID = 0;
    if(AIOUSB_Lock()) {
      /*
       * productNameIndex[] represents an index into
       * productIDNameTable[], sorted by product name (see notes for
       * ProductIDToName())
       */

          static ProductIDName const *productNameIndex[ NUM_PROD_NAMES ];
          /** index of product names in productIDNameTable[] */

          const unsigned long INIT_PATTERN = 0x7e6b2017ul;
          /** random pattern */

          static unsigned long productNameIndexCreated = 0;
          /** == INIT_PATTERN if index has been created */

          if(productNameIndexCreated != INIT_PATTERN) {
            /* build index of product names */
                int index;
                for(index = 0; index < NUM_PROD_NAMES; index++)
                    productNameIndex[ index ] = &productIDNameTable[ index ];
                qsort(productNameIndex, NUM_PROD_NAMES, sizeof(ProductIDName *), CompareProductNames);
                productNameIndexCreated = INIT_PATTERN;
            }

          ProductIDName key;                                   // key.id not used
          strncpy(key.name, name, PROD_NAME_SIZE);
          key.name[ PROD_NAME_SIZE ] = 0;                     // in case strncpy() doesn't copy null
          const ProductIDName *const pKey = &key;
          const ProductIDName **product
              = ( const ProductIDName** )bsearch(&pKey, productNameIndex, NUM_PROD_NAMES, sizeof(ProductIDName *), CompareProductNames);
          if(product != 0)
              productID = (*product)->id;
          AIOUSB_UnLock();
      }
    return productID;
}

static unsigned long GetDeviceName(unsigned long DeviceIndex, const char **name) {
    assert(name != 0);
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_SUCCESS;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->cachedName != 0) {
      /*
       * name is already cached, return it
       */
      *name = deviceDesc->cachedName;
          AIOUSB_UnLock();
      } else {
      /*
       * name is not yet cached, so request it from the device
       */
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                                                    // unlock while communicating with device
          const int MAX_NAME_SIZE = 100;                      // characters, not bytes
          char *const deviceName = ( char* )malloc(MAX_NAME_SIZE + 2);                // characters, null-terminated
          assert(deviceName != 0);
          if(deviceName != 0) {
                libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
                if(deviceHandle != 0) {
                  /*
                   * descriptor strings returned by the device are
                   * Unicode, not ASCII, and occupy two bytes per
                   * character, so we need to handle our maximum
                   * lengths accordingly
                   */
                      const int CYPRESS_GET_DESC = 0x06;
                      const int DESC_PARAMS = 0x0302;           /**03 = descriptor type: string; 02 = index */
                      const int MAX_DESC_SIZE = 256;           // bytes, not characters
                      unsigned char *const descData = ( unsigned char* )malloc(MAX_DESC_SIZE);
                      assert(descData != 0);
                      if(descData != 0) {
                            const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                                 USB_READ_FROM_DEVICE,
                                                                                 CYPRESS_GET_DESC,
                                                                                 DESC_PARAMS,
                                                                                 0,
                                                                                 descData,
                                                                                 MAX_DESC_SIZE,
                                                                                 timeout
                                                                                 );
                            if(bytesTransferred == MAX_DESC_SIZE) {
                              /**
                               * @verbatim
                               * extract device name from descriptor and copy to cached name buffer
                               *
                               * descData[ 0 ] = 1 (descriptor length) + 1 (descriptor type) + 2 (Unicode) * string_length
                               * descData[ 1 ] = \x03 (descriptor type: string)
                               * descData[ 2 ] = low byte of first character of Unicode string
                               * descData[ 3 ] = \0 (high byte)
                               * descData[ 4 ] = low byte of second character of string
                               * descData[ 5 ] = \0 (high byte)
                               * ...
                               * descData[ string_length * 2 ] = low byte of last character of string
                               * descData[ string_length * 2 + 1 ] = \0 (high byte)
                               * @endverbatim
                               */

                                  const int srcLength = ( int )((descData[ 0 ] - 2) / 2);               // characters, not bytes
                                  int srcIndex, dstIndex;
                                  for(srcIndex = 2 /* low byte first char */, dstIndex = 0;
                                      dstIndex < srcLength &&
                                      dstIndex < MAX_NAME_SIZE;
                                      srcIndex += 2 /* bytes per char */, dstIndex++
                                      ) {
                                        deviceName[ dstIndex ] = descData[ srcIndex ];
                                    }
                                  deviceName[ dstIndex ] = 0;                   // null-terminate
                                  AIOUSB_Lock();
                                  *name = deviceDesc->cachedName = deviceName;               // do not free( deviceName )
                                  AIOUSB_UnLock();
                              } else
                                result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                            free(descData);
                        } else
                          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                  } else
                    result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                if(result != AIOUSB_SUCCESS)
                    free(deviceName);
            } else
              result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
      }
    return result;
}

/*
 * GetSafeDeviceName() returns a null-terminated device name; if GetSafeDeviceName() is unable
 * to obtain a legitimate device name it returns something like "UNKNOWN" or 0
 */
PRIVATE const char *GetSafeDeviceName(unsigned long DeviceIndex) {
    const char *deviceName = 0;

    if(!AIOUSB_Lock())
        return deviceName;

    if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS) {
          DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
          if(deviceDesc->bGetName) {
            /*
             * device supports getting its product name, so use it instead of the
             * name from the local product name table
             */
                AIOUSB_UnLock();                                            // unlock while communicating with device
                unsigned long res = GetDeviceName(DeviceIndex, &deviceName);
                if(res != AIOUSB_SUCCESS) {
                  /*
                   * failed to get name from device, so fall back to local table after all
                   */
                      AIOUSB_Lock();
                      deviceName = ProductIDToName(deviceDesc->ProductID);
                      AIOUSB_UnLock();
                  }
            } else {
            /*
             * device doesn't support getting its product name, so use local
             * product name table
             */
                deviceName = ProductIDToName(deviceDesc->ProductID);
                AIOUSB_UnLock();
            }
      } else {
          AIOUSB_UnLock();
      }
    return deviceName;
}

struct libusb_device_handle * AIOUSB_GetUSBHandle( DeviceDescriptor *deviceDesc ) {
    return deviceDesc->deviceHandle;
}

/**
 * @param DeviceIndex
 * @return struct libusb_device_handle *
 */
PRIVATE struct libusb_device_handle *AIOUSB_GetDeviceHandle(unsigned long DeviceIndex) {
    libusb_device_handle *deviceHandle = NULL;
    /* libusb_set_debug(NULL, 4 ); */

    if(AIOUSB_Validate(&DeviceIndex) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return deviceHandle;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    deviceHandle = deviceDesc->deviceHandle;
    if(deviceHandle == NULL) {
        int libusbResult = libusb_open(deviceDesc->device, &deviceHandle);
        if( libusbResult == LIBUSB_SUCCESS && deviceHandle != NULL ) {
            int kernelActive = libusb_kernel_driver_active( deviceHandle, 0 );
            if ( kernelActive == 1 ) {
                libusbResult = libusb_claim_interface( deviceHandle, 0 );
                libusbResult = libusb_attach_kernel_driver( deviceHandle, 0 );
            }

            deviceDesc->deviceHandle = deviceHandle;
          }
      }

    AIOUSB_UnLock();
    return deviceHandle;
}



/**
 * @details This function is intended to improve upon
 * libusb_bulk_transfer() by receiving or transmitting packets until
 * the entire transfer request has been satisfied; it intentionally
 * restarts the timeout each time a packet is received, so the timeout
 * parameter specifies the longest permitted delay between packets,
 * not the total time to complete the transfer request
 */
PRIVATE int AIOUSB_BulkTransfer(
                                struct libusb_device_handle *dev_handle,
                                unsigned char endpoint,
                                unsigned char *data,
                                int length,
                                int *transferred,
                                unsigned int timeout
                                ) {
    assert(dev_handle != 0 &&
           data != 0 &&
           transferred != 0);
    int libusbResult = LIBUSB_SUCCESS;
    int total = 0;
    while(length > 0) {
          int bytes;
          libusbResult = libusb_bulk_transfer(dev_handle, endpoint, data, length, &bytes, timeout);
          if(libusbResult == LIBUSB_SUCCESS) {
                if(bytes > 0) {
                      total += bytes;
                      data += bytes;
                      length -= bytes;
                  }
            } else if(libusbResult == LIBUSB_ERROR_TIMEOUT) {
            /**
             * @note
             * even if we get a timeout, some data may have been transferred; if so, then
             * this timeout is not an error; if we get a timeout and no data was transferred,
             * then treat it as an error condition
             */
                if(bytes > 0) {
                      total += bytes;
                      data += bytes;
                      length -= bytes;
                  } else
                    break;              // from while() and return timeout result
            } else
              break;            // from while() and return error result
      }
    *transferred = total;
    return libusbResult;
}




unsigned long AIOUSB_GetDevices(void)
{
    unsigned long deviceMask = 0;

    if(!AIOUSB_Lock())
        return deviceMask;

    if(AIOUSB_IsInit()) {
/*
 * we clear the device table to erase references to devices
 * which may have been unplugged; any device indexes to devices
 * that have not been unplugged, which the user may be using,
 * _should_ still be valid
 */
          ClearDevices();
          int index;
          for(index = 0; index < MAX_USB_DEVICES; index++) {
                if(deviceTable[ index ].device != NULL)
                    deviceMask = (deviceMask << 1) | 1;
            }
      }

    AIOUSB_UnLock();
    return deviceMask;
}


unsigned long GetDevices(void) {
    unsigned long deviceMask = 0;

    if(!AIOUSB_Lock())
        return deviceMask;

    if(AIOUSB_IsInit()) {
      /**
       * @note
       * we clear the device table to erase references to devices
       * which may have been unplugged; any device indexes to devices
       * that have not been unplugged, which the user may be using,
       * _should_ still be valid
       */
          ClearDevices();
          int index;
          for(index = 0; index < MAX_USB_DEVICES; index++) {
                if(deviceTable[ index ].device != NULL)
                    deviceMask = (deviceMask << 1) | 1;
            }
      }

    AIOUSB_UnLock();
    return deviceMask;
}


unsigned long QueryDeviceInfo(
                                            unsigned long DeviceIndex,
                                            unsigned long *pPID,
                                            unsigned long *pNameSize,
                                            char *pName,
                                            unsigned long *pDIOBytes,
                                            unsigned long *pCounters
                                            ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(pPID != NULL)
        *pPID = deviceDesc->ProductID;

    if(pDIOBytes != NULL)
        *pDIOBytes = deviceDesc->DIOBytes;

    if(pCounters != NULL)
        *pCounters = deviceDesc->Counters;

    AIOUSB_UnLock();                                                            // unlock while communicating with device

    if(
        pNameSize != NULL &&
        pName != NULL
        ) {
          const char *deviceName = GetSafeDeviceName(DeviceIndex);
          if(deviceName != 0) {
            /*
             * got a device name, so return it; pName[] is a character array, not a
             * null-terminated string, so don't append null terminator
             */
                int length = strlen(deviceName);
                if(length > ( int )*pNameSize)
                    length = ( int )*pNameSize;
                else
                    *pNameSize = length;
                memcpy(pName, deviceName, length);          // not null-terminated
            }
      }

    return result;
}




unsigned long ClearDevices(void) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;
    CloseAllDevices();
    InitDeviceTable();
    PopulateDeviceTable();
    AIOUSB_UnLock();
    return AIOUSB_SUCCESS;
}



unsigned long ResolveDeviceIndex(unsigned long DeviceIndex) {
    return (AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS)
           ? DeviceIndex
           : diNone;
}
/*------------------------------------------------------------------------*/
DeviceDescriptor *DeviceTableAtIndex( unsigned long DeviceIndex ) { 
    unsigned long result = AIOUSB_Validate( &DeviceIndex  );
    if ( !result != AIOUSB_SUCCESS ) {
        
    }
    DeviceDescriptor * deviceDesc = &deviceTable[ DeviceIndex ];

    return deviceDesc;
}
/*------------------------------------------------------------------------*/
/**
 * @todo Replace AIOUSB_Lock() with thread safe lock on a per device index basis
 * @todo Insert correct error messages into global error string in case of failure
 */
DeviceDescriptor *DeviceTableAtIndex_Lock( unsigned long DeviceIndex ) { 
    unsigned long result = AIOUSB_Validate( &DeviceIndex  );
    if ( !result != AIOUSB_SUCCESS ) {
        return NULL;
    }
    DeviceDescriptor * deviceDesc = &deviceTable[ DeviceIndex ];
    if( deviceDesc ) 
        AIOUSB_Lock();
    return deviceDesc;
}

DeviceDescriptor *AIOUSB_GetDevice_Lock(unsigned long DeviceIndex, unsigned long *result) {
    *result = AIOUSB_Validate(&DeviceIndex);
    if(*result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return NULL;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(!deviceDesc) {
          AIOUSB_UnLock();
          *result = AIOUSB_ERROR_DEVICE_NOT_FOUND;
      }
    return deviceDesc;
}


AIORET_TYPE AIOUSB_GetDeviceSerialNumber( unsigned long DeviceIndex ) {
    unsigned long val = 0;
    unsigned long retval;
    retval = GetDeviceSerialNumber( DeviceIndex, &val );
    if( retval != AIOUSB_SUCCESS ) {
      return -1*(AIORET_TYPE)retval;
    } else {
      return (AIORET_TYPE)val;
    }
}

/**
 * @brief 
 * @param DeviceIndex 
 * @param pSerialNumber 
 * @return 0 if successful, otherwise
 */
unsigned long GetDeviceSerialNumber(
                                    unsigned long DeviceIndex,
                                    unsigned long *pSerialNumber
                                    ) {
    if( !pSerialNumber )
        return AIOUSB_ERROR_INVALID_PARAMETER;

    unsigned long bytes_read = sizeof(unsigned long);
    unsigned long buffer_data;
    unsigned long result;
    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
    if(!deviceDesc || result != AIOUSB_SUCCESS) {
        goto out_GetDeviceSerialNumber;
    }

    result = GenericVendorRead( DeviceIndex, AUR_EEPROM_READ , EEPROM_SERIAL_NUMBER_ADDRESS, 0 , &buffer_data, &bytes_read );
    if( result != AIOUSB_SUCCESS )
        goto out_GetDeviceSerialNumber;

    *pSerialNumber = (unsigned long)buffer_data;

out_GetDeviceSerialNumber:
    AIOUSB_UnLock();

    return result;
}

/**
 * @brief This function is deprecated.
 * @param DeviceIndex
 * @param BlockSize
 * @return 0 or greater if the blocksize is correct, negative number on
 *              error
 */
long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex) {
    long BlockSize;

    if(!AIOUSB_Lock())
        return -AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return -result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream || deviceDesc->bDIOStream)
        BlockSize = deviceDesc->StreamingBlockSize;
    else
        BlockSize = -AIOUSB_ERROR_NOT_SUPPORTED;

    AIOUSB_UnLock();
    return BlockSize;
}

unsigned long AIOUSB_SetStreamingBlockSize(
                             unsigned long DeviceIndex,
                             unsigned long BlockSize
                             ) {
     if( BlockSize == 0 || BlockSize > 31ul * 1024ul * 1024ul )
          return AIOUSB_ERROR_INVALID_PARAMETER;
     
     if(!AIOUSB_Lock())
          return AIOUSB_ERROR_INVALID_MUTEX;

     unsigned long result = AIOUSB_Validate(&DeviceIndex);
     if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
     }

     DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
     if(deviceDesc->bADCStream) {
          if((BlockSize & 0x1FF) != 0)
               BlockSize = (BlockSize & 0xFFFFFE00ul) + 0x200;
          deviceDesc->StreamingBlockSize = BlockSize;
     } else if(deviceDesc->bDIOStream) {
          if((BlockSize & 0xFF) != 0)
               BlockSize = (BlockSize & 0xFFFFFF00ul) + 0x100;
          deviceDesc->StreamingBlockSize = BlockSize;
     } else
          result = AIOUSB_ERROR_NOT_SUPPORTED;

     AIOUSB_UnLock();
     return result;
}

unsigned long AIOUSB_ClearFIFO(
                                             unsigned long DeviceIndex,
                                             FIFO_Method Method
                                             ) {
  if(!VALID_ENUM(FIFO_Method, Method))
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
      /*
       * translate method into vendor request message
       */
          int request;
          switch(Method) {
            /* case CLEAR_FIFO_METHOD_IMMEDIATE: */
            default:
                request = AUR_GEN_CLEAR_FIFO;
                break;

            case CLEAR_FIFO_METHOD_AUTO:
                request = AUR_GEN_CLEAR_FIFO_NEXT;
                break;

            case CLEAR_FIFO_METHOD_IMMEDIATE_AND_ABORT:
                request = AUR_GEN_ABORT_AND_CLEAR;
                break;

            case CLEAR_FIFO_METHOD_WAIT:
                request = AUR_GEN_CLEAR_FIFO_WAIT;
                break;
            }
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                                                    // unlock while communicating with device
          const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                               USB_WRITE_TO_DEVICE, request, 0, 0, 0, 0 /* wLength */, timeout);
          if(bytesTransferred != 0)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}




const char *AIOUSB_GetVersion() {
    return VERSION_NUMBER;
}

const char *AIOUSB_GetVersionDate() {
    return VERSION_DATE;
}

unsigned long AIOUSB_InitTest(void) {
    InitDeviceTable();
    aiousbInit = AIOUSB_INIT_PATTERN;
    return AIOUSB_SUCCESS;
}

/**
 * @brief AIOUSB_Init() and AIOUSB_Exit() are not thread-safe and
 * should not be called while other threads might be accessing global
 * variables. Hence you should just run AIOUSB_Init() once at the beginning
 * and then the AIOUSB_Exit() once at the end after every thread acquiring
 * data has been stopped.
 */
unsigned long AIOUSB_Init(void) {
    unsigned long result = AIOUSB_SUCCESS;

    if(!AIOUSB_IsInit()) {
          InitDeviceTable();
#if defined(AIOUSB_ENABLE_MUTEX)
          pthread_mutexattr_t mutexAttr;
          if(pthread_mutexattr_init(&mutexAttr) == 0) {
                if(pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE) == 0) {
                      if(pthread_mutex_init(&aiousbMutex, &mutexAttr) == 0) {
                            const int libusbResult = libusb_init( NULL );

                            if(libusbResult == LIBUSB_SUCCESS) {
                              /**
                               * @note
                               * populate device table so users can use diFirst and diOnly immediately; be
                               * sure to call PopulateDeviceTable() after 'aiousbInit = AIOUSB_INIT_PATTERN;'
                               */
                                  aiousbInit = AIOUSB_INIT_PATTERN;
                                  PopulateDeviceTable();
                              } else {
                                  pthread_mutex_destroy(&aiousbMutex);
                                  result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                              }
                        } else
                          result = AIOUSB_ERROR_INVALID_MUTEX;
                  } else
                    result = AIOUSB_ERROR_INVALID_MUTEX;
                pthread_mutexattr_destroy(&mutexAttr);
            } else
              result = AIOUSB_ERROR_INVALID_MUTEX;
#else
          const int libusbResult = libusb_init(NULL);
          if(libusbResult == LIBUSB_SUCCESS) {
            /**
             * @note
             * populate device table so users can use diFirst and diOnly immediately; be
             * sure to call PopulateDeviceTable() after 'aiousbInit = AIOUSB_INIT_PATTERN;'
             */
                aiousbInit = AIOUSB_INIT_PATTERN;
                PopulateDeviceTable();
            } else {
                result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
            }
#endif
      }
    return result;
}

void AIOUSB_Exit() {
    if(AIOUSB_IsInit()) {
          CloseAllDevices();
          libusb_exit(NULL);
#if defined(AIOUSB_ENABLE_MUTEX)
          pthread_mutex_destroy(&aiousbMutex);
#endif
          aiousbInit = 0;
      }
}

unsigned long AIOUSB_Reset(
                                         unsigned long DeviceIndex
                                         ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          AIOUSB_UnLock();                                                    // unlock while communicating with device
          const int libusbResult = libusb_reset_device(deviceHandle);
          if(libusbResult != LIBUSB_SUCCESS)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
          usleep(250000);                                                     // give device a little time to reset itself
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}

double AIOUSB_GetMiscClock(
                                         unsigned long DeviceIndex
                                         ) {
    double clockHz = 0;                                                         // return reasonable value on error

    if(AIOUSB_Lock()) {
          if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS)
              clockHz = deviceTable[ DeviceIndex ].miscClockHz;
          AIOUSB_UnLock();
      }
    return clockHz;
}

unsigned long AIOUSB_SetMiscClock(
                                                unsigned long DeviceIndex,
                                                double clockHz
                                  ) {
    if(clockHz <= 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result == AIOUSB_SUCCESS)
        deviceTable[ DeviceIndex ].miscClockHz = clockHz;

    AIOUSB_UnLock();
    return result;
}

unsigned AIOUSB_GetCommTimeout(
                                             unsigned long DeviceIndex
                                             ) {
    unsigned timeout = 1000;                                            // return reasonable value on error

    if(AIOUSB_Lock()) {
          if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS)
              timeout = deviceTable[ DeviceIndex ].commTimeout;
          AIOUSB_UnLock();
      }
    return timeout;
}

unsigned long AIOUSB_SetCommTimeout(
                                                  unsigned long DeviceIndex,
                                                  unsigned timeout
                                    ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result == AIOUSB_SUCCESS)
        deviceTable[ DeviceIndex ].commTimeout = timeout;

    AIOUSB_UnLock();
    return result;
}

unsigned long AIOUSB_Validate_Device(unsigned long DeviceIndex) {
    unsigned long result;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    if(!AIOUSB_Lock()) {
          result = AIOUSB_ERROR_INVALID_MUTEX;
          goto RETURN_AIOUSB_Validate_Device;
      }

    result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_AIOUSB_Validate_Device;
      }

    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          result = AIOUSB_ERROR_NOT_SUPPORTED;
          goto RETURN_AIOUSB_Validate_Device;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
      }
RETURN_AIOUSB_Validate_Device:
    return result;
}

PRIVATE void _Initialize_Device_Desc(unsigned long DeviceIndex) {
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    deviceDesc->DIOBytes = 0;
    deviceDesc->DIOConfigBits = 0;
    deviceDesc->Counters = 0;
    deviceDesc->RootClock = 0;
    deviceDesc->Tristates = 0;
    deviceDesc->bGetName = AIOUSB_FALSE;
    deviceDesc->ConfigBytes = 0;
    deviceDesc->bGateSelectable = AIOUSB_FALSE;
    deviceDesc->bDACBoardRange = AIOUSB_FALSE;
    deviceDesc->bDACChannelCal = AIOUSB_FALSE;
    deviceDesc->ImmDACs = 0;
    deviceDesc->ImmADCs = 0;
    deviceDesc->ADCChannels = 0;
    deviceDesc->ADCMUXChannels = 0;
    deviceDesc->bDACStream = AIOUSB_FALSE;
    deviceDesc->bADCStream = AIOUSB_FALSE;
    deviceDesc->RangeShift = 0;
    deviceDesc->bDIOStream = AIOUSB_FALSE;
    deviceDesc->StreamingBlockSize = 31 * 1024;
    deviceDesc->bDIODebounce = AIOUSB_FALSE;
    deviceDesc->bDIOSPI = AIOUSB_FALSE;
    deviceDesc->bClearFIFO = AIOUSB_FALSE;
    deviceDesc->FlashSectors = 0;
    deviceDesc->WDGBytes = 0;
    deviceDesc->bSetCustomClocks = AIOUSB_FALSE;
}


PRIVATE unsigned long  _Card_Specific_Settings(unsigned long DeviceIndex) {
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    unsigned long result = AIOUSB_SUCCESS;

    switch(deviceDesc->ProductID) {
      case 0x8001:
          deviceDesc->DIOBytes = 4;
          deviceDesc->Counters = 3;
          deviceDesc->RootClock = 3000000;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bSetCustomClocks = AIOUSB_TRUE;
          deviceDesc->bDIODebounce = AIOUSB_TRUE;
          break;

      case 0x8004:
          deviceDesc->DIOBytes = 4;
          deviceDesc->DIOConfigBits = 32;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bSetCustomClocks = AIOUSB_TRUE;
          break;

      case 0x8002:
          deviceDesc->DIOBytes = 6;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8003:
          deviceDesc->DIOBytes = 12;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8008:
      case 0x8009:
      case 0x800A:
          deviceDesc->DIOBytes = 1;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bDIOStream = AIOUSB_TRUE;
          deviceDesc->bDIOSPI = AIOUSB_TRUE;
          deviceDesc->bClearFIFO = AIOUSB_TRUE;
          break;

      case 0x800C:
      case 0x800D:
      case 0x800E:
      case 0x800F:
          deviceDesc->DIOBytes = 4;
          deviceDesc->Tristates = 2;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bDIOStream = AIOUSB_TRUE;
          deviceDesc->bDIOSPI = AIOUSB_TRUE;
          deviceDesc->bClearFIFO = AIOUSB_TRUE;
          break;

          //USB-IIRO-16 family
      case 0x8010:
      case 0x8011:
      case 0x8012:
      case 0x8014:
      case 0x8015:
      case 0x8016:
        //USB-IDIO-16 family
      case 0x8018:
      case 0x801a:
      case 0x801c:
      case 0x801e:
      case 0x8019:
      case 0x801d:
      case 0x801f:
          deviceDesc->DIOBytes = 4;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->WDGBytes = 2;
          break;

      case 0x4001:
      case 0x4002:
          deviceDesc->bGetName = AIOUSB_FALSE;
          deviceDesc->bDACStream = AIOUSB_TRUE;
          deviceDesc->ImmDACs = 8;
          deviceDesc->DACsUsed = 5;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x4003:
          deviceDesc->bGetName = AIOUSB_FALSE;
          deviceDesc->ImmDACs = 8;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8020:
          deviceDesc->Counters = 5;
          deviceDesc->bGateSelectable = AIOUSB_TRUE;
          deviceDesc->RootClock = 10000000;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8030:
      case 0x8031:
          deviceDesc->DIOBytes = 2;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8032:
          deviceDesc->DIOBytes = 3;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8033:
          deviceDesc->DIOBytes = 3;
          deviceDesc->bGetName = AIOUSB_TRUE;
          break;

      case 0x8036:
          deviceDesc->DIOBytes = 2;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->ImmADCs = 2;
          break;

      case 0x8037:
          deviceDesc->DIOBytes = 2;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->ImmADCs = 2;
          break;

      case 0x8040:
      case 0x8041:
      case 0x8042:
      case 0x8043:
      case 0x8044:
      case 0x8140:
      case 0x8141:
      case 0x8142:
      case 0x8143:
      case 0x8144:
          deviceDesc->DIOBytes = 2;
          deviceDesc->Counters = 1;
          deviceDesc->RootClock = 10000000;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bADCStream = AIOUSB_TRUE;
          deviceDesc->ADCChannels = 16;
          deviceDesc->ADCMUXChannels = 16;
          deviceDesc->ConfigBytes = 20;
          deviceDesc->RangeShift = 0;
          deviceDesc->bClearFIFO = AIOUSB_TRUE;
          if((deviceDesc->ProductID & 0x0100) != 0) {
                deviceDesc->bDACBoardRange = AIOUSB_TRUE;
                deviceDesc->ImmDACs = 2;
            }
          break;

      case 0x8045:
      case 0x8046:
      case 0x8047:
      case 0x8048:
      case 0x8049:
      case 0x8145:
      case 0x8146:
      case 0x8147:
      case 0x8148:
      case 0x8149:
          deviceDesc->DIOBytes = 2;
          deviceDesc->Counters = 1;
          deviceDesc->RootClock = 10000000;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bADCStream = AIOUSB_TRUE;
          deviceDesc->ADCChannels = 16;
          deviceDesc->ADCMUXChannels = 64;
          deviceDesc->ConfigBytes = 21;
          deviceDesc->RangeShift = 2;
          deviceDesc->bClearFIFO = AIOUSB_TRUE;
          if((deviceDesc->ProductID & 0x0100) != 0) {
                deviceDesc->bDACBoardRange = AIOUSB_TRUE;
                deviceDesc->ImmDACs = 2;
            }
          break;

      case 0x804a:
      case 0x804b:
      case 0x804c:
      case 0x804d:
      case 0x804e:
      case 0x804f:
      case 0x8050:
      case 0x8051:
      case 0x8052:
      case 0x8053:
      case 0x8054:
      case 0x8055:
      case 0x8056:
      case 0x8057:
      case 0x8058:
      case 0x8059:
      case 0x805a:
      case 0x805b:
      case 0x805c:
      case 0x805d:
      case 0x805e:
      case 0x805f:
      case 0x814a:
      case 0x814b:
      case 0x814c:
      case 0x814d:
      case 0x814e:
      case 0x814f:
      case 0x8150:
      case 0x8151:
      case 0x8152:
      case 0x8153:
      case 0x8154:
      case 0x8155:
      case 0x8156:
      case 0x8157:
      case 0x8158:
      case 0x8159:
      case 0x815a:
      case 0x815b:
      case 0x815c:
      case 0x815d:
      case 0x815e:
      case 0x815f:
          deviceDesc->DIOBytes = 2;
          deviceDesc->Counters = 1;
          deviceDesc->RootClock = 10000000;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->bADCStream = AIOUSB_TRUE;
          deviceDesc->ADCChannels = 16;
          deviceDesc->ADCMUXChannels = 32 * ((((deviceDesc->ProductID - 0x804A) & (~0x0100)) / 5) + 1);
          deviceDesc->ConfigBytes = 21;
          deviceDesc->RangeShift = 3;
          deviceDesc->bClearFIFO = AIOUSB_TRUE;
          if((deviceDesc->ProductID & 0x0100) != 0) {
                deviceDesc->bDACBoardRange = AIOUSB_TRUE;
                deviceDesc->ImmDACs = 2;
            }
          break;

      case 0x8060:
      case 0x8070:
      case 0x8071:
      case 0x8072:
      case 0x8073:
      case 0x8074:
      case 0x8075:
      case 0x8076:
      case 0x8077:
      case 0x8078:
      case 0x8079:
      case 0x807a:
      case 0x807b:
      case 0x807c:
      case 0x807d:
      case 0x807e:
      case 0x807f:
          deviceDesc->DIOBytes = 2;
          deviceDesc->bGetName = AIOUSB_TRUE;
          deviceDesc->FlashSectors = 32;
          deviceDesc->bDACBoardRange = AIOUSB_TRUE;
          deviceDesc->bDACChannelCal = AIOUSB_TRUE;
          //deviceDesc->bClearFIFO = AIOUSB_TRUE;
          //Add a new-style DAC streaming
          switch(deviceDesc->ProductID & 0x06) {
            case 0x00:
                deviceDesc->ImmDACs = 16;
                break;

            case 0x02:
                deviceDesc->ImmDACs = 12;
                break;

            case 0x04:
                deviceDesc->ImmDACs = 8;
                break;

            case 0x06:
                deviceDesc->ImmDACs = 4;
                break;
            }
          if((deviceDesc->ProductID & 1) == 0)
              deviceDesc->ImmADCs = 2;
          break;

      default:
          deviceDesc->bADCStream = AIOUSB_TRUE;
          deviceDesc->bDIOStream = AIOUSB_TRUE;
          deviceDesc->bDIOSPI = AIOUSB_TRUE;
          result = AIOUSB_SUCCESS;
          break;
      }
    return result;
}

/**
 * @param DeviceIndex
 * @return
 */
unsigned long AIOUSB_EnsureOpen(unsigned long DeviceIndex) {
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    unsigned long result = AIOUSB_SUCCESS;

    if(deviceDesc->deviceHandle == 0) {
          if(deviceDesc->bDeviceWasHere)
              result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          else
              result = AIOUSB_ERROR_FILE_NOT_FOUND;
          goto RETURN_AIOUSB_EnsureOpen;
      }

    if(deviceDesc->bOpen) {
          result = AIOUSB_ERROR_OPEN_FAILED;
          goto RETURN_AIOUSB_EnsureOpen;
      }

    if(result == AIOUSB_SUCCESS) {
          _Initialize_Device_Desc(DeviceIndex);

          result |= _Card_Specific_Settings(DeviceIndex);
          if(result != AIOUSB_SUCCESS)
              goto RETURN_AIOUSB_EnsureOpen;

          if(deviceDesc->DIOConfigBits == 0)
              deviceDesc->DIOConfigBits = deviceDesc->DIOBytes;
      }

RETURN_AIOUSB_EnsureOpen:
    return result;
}



/**
 * @param DeviceIndex
 * @param channel
 * @param counts
 * @return
 */
double AIOUSB_CountsToVolts(
    unsigned long DeviceIndex,
    unsigned channel,
    unsigned short counts
    )
{
    double volts;

    if(AIOUSB_ArrayCountsToVolts(DeviceIndex, channel, 1, &counts, &volts) != AIOUSB_SUCCESS)
        volts = 0.0;
    return volts;
}



/**
 * @param DeviceIndex
 * @param startChannel
 * @param endChannel
 * @param counts
 * @return
 */
unsigned long AIOUSB_MultipleCountsToVolts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const unsigned short counts[],     /* deviceDesc->ADCMUXChannels */
    double volts[]     /* deviceDesc->ADCMUXChannels */
    )
{
    return AIOUSB_ArrayCountsToVolts(DeviceIndex, 
                                     startChannel, 
                                     endChannel - startChannel + 1,
                                     counts + startChannel, 
                                     volts + startChannel
                                     );
}
/*------------------------------------------------------------------------*/
/**
 * @param DeviceIndex
 * @param channel
 * @param volts
 * @return
 */
unsigned short AIOUSB_VoltsToCounts(
    unsigned long DeviceIndex,
    unsigned channel,
    double volts
    )
{
    unsigned short counts;

    if(AIOUSB_ArrayVoltsToCounts(DeviceIndex, channel, 1, &volts, &counts) != AIOUSB_SUCCESS)
        counts = 0;
    return counts;
}
/*------------------------------------------------------------------------*/
/**
 * @param DeviceIndex
 * @param startChannel
 * @param endChannel
 * @param volts
 * @return
 */
unsigned long AIOUSB_MultipleVoltsToCounts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const double volts[],     /* deviceDesc->ADCMUXChannels */
    unsigned short counts[]     /* deviceDesc->ADCMUXChannels */
    )
{
    return AIOUSB_ArrayVoltsToCounts(DeviceIndex, startChannel, endChannel - startChannel + 1,
                                     volts + startChannel, counts + startChannel);
}
/*------------------------------------------------------------------------*/
/**
 * @param config
 * @param DeviceIndex
 * @param defaults
 */
void AIOUSB_InitConfigBlock(ADConfigBlock *config, unsigned long DeviceIndex, AIOUSB_BOOL defaults)
{
    assert(config != 0);
    if(config != 0) {
/*
 * mark as uninitialized unless this function succeeds
 */
          config->device = 0;
          config->size = 0;
          if(AIOUSB_Lock()) {
                if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS) {
                      const DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
                      config->device = deviceDesc;
                      config->size = deviceDesc->ConfigBytes;
                      assert(config->size == AD_CONFIG_REGISTERS ||
                             config->size == AD_MUX_CONFIG_REGISTERS);
                      if(defaults) {
                            AIOUSB_SetAllGainCodeAndDiffMode(config, AD_GAIN_CODE_0_10V, AIOUSB_FALSE);
                            AIOUSB_SetCalMode(config, AD_CAL_MODE_NORMAL);
                            AIOUSB_SetTriggerMode(config, 0);
                            AIOUSB_SetScanRange(config, 0, deviceDesc->ADCMUXChannels - 1);
                            AIOUSB_SetOversample(config, 0);
                        }
                  }
                AIOUSB_UnLock();
            }
      }
}
/*------------------------------------------------------------------------*/
/**
 * @param DeviceIndex
 * @param fileName
 * @return
 */
unsigned long AIOUSB_ADC_LoadCalTable(
                                      unsigned long DeviceIndex,
                                      const char *fileName
                                      )
{
    if(fileName == 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    AIOUSB_UnLock();
    unsigned short *const calTable = ( unsigned short* )malloc(CAL_TABLE_WORDS * sizeof(unsigned short));
    assert(calTable != 0);
    if(calTable != 0) {
          struct stat fileInfo;
          if(stat(fileName, &fileInfo) == 0) {
                if(fileInfo.st_size == CAL_TABLE_WORDS * sizeof(unsigned short)) {
                      FILE *const calFile = fopen(fileName, "r");
                      if(calFile != NULL) {
                            const size_t wordsRead = fread(calTable, sizeof(unsigned short), CAL_TABLE_WORDS, calFile);
                            fclose(calFile);
                            if(wordsRead == ( size_t )CAL_TABLE_WORDS)
                                result = AIOUSB_ADC_SetCalTable(DeviceIndex, calTable);
                            else
                                result = AIOUSB_ERROR_FILE_NOT_FOUND;
                        }else
                          result = AIOUSB_ERROR_FILE_NOT_FOUND;
                  }else
                    result = AIOUSB_ERROR_INVALID_DATA;              // file size incorrect
            }else
              result = AIOUSB_ERROR_FILE_NOT_FOUND;
          free(calTable);
      }else
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;

    return result;
}
/*------------------------------------------------------------------------*/
/**
 * @param DeviceIndex
 * @param calTable
 * @return
 */
unsigned long AIOUSB_ADC_SetCalTable(
                                     unsigned long DeviceIndex,
                                     const unsigned short calTable[]
                                     )
{
    if(calTable == 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
/*
 * send calibration table to SRAM one block at a time; according to the documentation,
 * the proper procedure is to bulk transfer a block of calibration data to "endpoint 2"
 * and then send a control message to load it into the SRAM
 */
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                              // unlock while communicating with device
          const int SRAM_BLOCK_WORDS = 1024;       // can send 1024 words at a time to SRAM
          int sramAddress = 0;
          int wordsRemaining = CAL_TABLE_WORDS;
          while(wordsRemaining > 0) {
                const int wordsWritten
                    = (wordsRemaining < SRAM_BLOCK_WORDS)
                      ? wordsRemaining
                      : SRAM_BLOCK_WORDS;
                int bytesTransferred;
                const int libusbResult = AIOUSB_BulkTransfer(deviceHandle,
                                                             LIBUSB_ENDPOINT_OUT | USB_BULK_WRITE_ENDPOINT,
                                                             ( unsigned char* )(calTable + sramAddress), wordsWritten * sizeof(unsigned short),
                                                             &bytesTransferred, timeout);
                if(libusbResult != LIBUSB_SUCCESS) {
                      result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                      break;                                  // from while()
                  }else if(bytesTransferred != ( int )(wordsWritten * sizeof(unsigned short))) {
                      result = AIOUSB_ERROR_INVALID_DATA;
                      break;                                  // from while()
                  }else {
                      bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                 USB_WRITE_TO_DEVICE, AUR_LOAD_BULK_CALIBRATION_BLOCK,
                                                                 sramAddress, wordsWritten, 0, 0, timeout);
                      if(bytesTransferred != 0) {
                            result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                            break;                            // from while()
                        }
                  }
                wordsRemaining -= wordsWritten;
                sramAddress += wordsWritten;
            }
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}




/** 
 * @brief Performs a generic vendor USB write
 */
unsigned long GenericVendorWrite(
                                 unsigned long deviceIndex,
                                 unsigned char Request,
                                 unsigned short Value,
                                 unsigned short Index,
                                 void *bufData,
                                 unsigned long *bytes_written
                                 ) {
    unsigned long result;
    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( deviceIndex, &result );
    libusb_device_handle *deviceHandle;
    int bytesTransferred;

    if ( !deviceDesc || result != AIOUSB_SUCCESS )
        goto out_GenericVendorWrite;

    deviceHandle = AIOUSB_GetDeviceHandle(deviceIndex);

    if (!deviceHandle)  {
        result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
        goto out_GenericVendorWrite;
    }

    AIOUSB_UnLock();  /*  unlock while communicating with device */
    bytesTransferred = libusb_control_transfer(deviceHandle,
                                               USB_WRITE_TO_DEVICE,
                                               Request,
                                               Value,
                                               Index,
                                               (unsigned char*)bufData,
                                               *bytes_written,
                                               deviceDesc->commTimeout
                                               );
    if (bytesTransferred != (int)*bytes_written) {
        result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
    }

out_GenericVendorWrite:
    AIOUSB_UnLock();
    return result;
}

/**
 * @brief Performs basic low level USB vendor request
 * @return
 */
unsigned long GenericVendorRead(
                                unsigned long deviceIndex,
                                unsigned char Request,
                                unsigned short Value,
                                unsigned short Index,
                                void *bufData,
                                unsigned long *bytes_read
                                ) {
    unsigned long result;
    DeviceDescriptor *const deviceDesc = &deviceTable[ deviceIndex ];
    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(deviceIndex);

    if(!AIOUSB_Lock()) {
          result = AIOUSB_ERROR_INVALID_MUTEX;
          goto RETURN_GenericVendorRead;
      }

    result = AIOUSB_Validate(&deviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_GenericVendorRead;
      }

    result = AIOUSB_EnsureOpen(deviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_GenericVendorRead;
      }

    if(deviceHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();       // unlock while communicating with device
          const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                               USB_READ_FROM_DEVICE,
                                                               Request,
                                                               Value,
                                                               Index,
                                                               (unsigned char*)bufData,
                                                               *bytes_read,
                                                               timeout
                                                               );
          if(bytesTransferred != (int)*bytes_read)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }
RETURN_GenericVendorRead:
    return result;
}

#ifdef __cplusplus
}       /* namespace AIOUSB */
#endif

#ifdef SELF_TEST


#include <math.h>
#include "gtest/gtest.h"
#include "tap.h"
#include "AIOUSB_Core.h"

#ifdef __cplusplus
using namespace AIOUSB;
#endif

TEST(AIOUSB_Core,FakeInit ) {

    AIOUSB_InitTest();    
    EXPECT_EQ(AIOUSB_TRUE, AIOUSB_IsInit() );

}

TEST(AIOUSB_Core,MockObjects) {
    int numDevices = 0;
    InitDeviceTable();    
    AddDeviceToDeviceTable( &numDevices, USB_AIO16_16A );
    EXPECT_EQ( numDevices, 1 );
    AddDeviceToDeviceTable( &numDevices, USB_DIO_32 );
    EXPECT_EQ( numDevices, 2 );

    EXPECT_EQ( ((DeviceDescriptor *)&deviceTable[0])->ProductID, USB_AIO16_16A  );
    EXPECT_EQ( ((DeviceDescriptor *)&deviceTable[1])->ProductID, USB_DIO_32  );

}


int main( int argc , char *argv[] ) 
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
    delete listeners.Release(listeners.default_result_printer());
#endif
    listeners.Append( new tap::TapListener() );
   
    return RUN_ALL_TESTS();  
}


#endif




