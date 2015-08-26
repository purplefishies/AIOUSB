#ifndef  _AIOUSB_DEVICE_H
#define  _AIOUSB_DEVICE_H

#include "AIOTypes.h"
#include "ADCConfigBlock.h"
#include "USBDevice.h"
#include "cJSON.h"
#include <string.h>
#include <semaphore.h>
#include <libusb.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

typedef struct aio_usb_driver {
    USBDevice *usb_device;
    AIOUSB_BOOL bOpen;
    int deviceIndex;
    AIOUSB_BOOL isInit;
    unsigned long PID;
    unsigned long DIOConfigBits;
    
    // run-time settings
    AIOUSB_BOOL discardFirstSample; /**< AIOUSB_TRUE == discard first A/D sample in all A/D read methods */
    unsigned commTimeout;           /**< timeout for device communication (ms.) */
    double miscClockHz;             /**< miscellaneous clock frequency setting */

    // device-specific properties
    unsigned ProductID;
    unsigned DIOBytes;
    unsigned Counters;
    unsigned Tristates;
    AIOUSB_BOOL bGateSelectable;
    long RootClock;
    AIOUSB_BOOL bGetName;
    unsigned long ConfigBytes;
    unsigned ImmDACs;
    AIOUSB_BOOL bDACStream;
    AIOUSB_BOOL bDACDIOClock;
    unsigned DACsUsed;
    AIOUSB_BOOL bADCStream;
    unsigned ADCChannels;
    unsigned ADCMUXChannels;
    
    unsigned char RangeShift;

    unsigned ADCChannelsPerGroup;      /**< number of A/D channels in each config. group (1, 4 or 8 depending on model) */
    AIOUSB_BOOL bDIOStream;
    unsigned long StreamingBlockSize;
    AIOUSB_BOOL bDIODebounce;
    AIOUSB_BOOL bDIOSPI;
    AIOUSB_BOOL bSetCustomClocks;

    unsigned WDGBytes;
    AIOUSB_BOOL bClearFIFO;
    unsigned ImmADCs;
    AIOUSB_BOOL bDACBoardRange;
    AIOUSB_BOOL bDACChannelCal;
    unsigned FlashSectors;

    // device state
    AIOUSB_BOOL bDACOpen;
    AIOUSB_BOOL bDACClosing;
    AIOUSB_BOOL bDACAborting;
    AIOUSB_BOOL bDACStarted;
    unsigned char **DACData;
    unsigned char *PendingDACData;
    pthread_mutex_t hDACDataMutex;
    sem_t hDACDataSem;
    AIOUSB_BOOL bDIOOpen;
    AIOUSB_BOOL bDIORead;
    AIOUSB_BOOL bDeviceWasHere;
    unsigned char *LastDIOData;
    char *cachedName;
    unsigned long cachedSerialNumber;
    ADCConfigBlock cachedConfigBlock; /**< .size == 0 == uninitialized */

    /**
     * state of worker thread; these fields are deliberately unspecific so that
     * the library can employ worker threads in a variety of situations
     */
    AIOUSB_BOOL workerBusy;     /**< AIOUSB_TRUE == worker thread is busy */
    unsigned long workerStatus; /**< thread-defined status information (e.g. bytes remaining to receive or transmit) */
    unsigned long workerResult; /**< standard AIOUSB_* result code from worker thread (if workerBusy == AIOUSB_FALSE) */

    /** New entries for the FastIT behavior */
    ADCConfigBlock *FastITConfig;
    ADCConfigBlock *FastITBakConfig;
    unsigned long FastITConfig_size;
 
    unsigned char *ADBuf;
    int ADBuf_size;
    AIOUSB_BOOL testing;
    AIOUSB_BOOL valid;
} AIOUSBDevice;

typedef AIOUSBDevice DeviceDescriptor;


char *AIOUSBDeviceToJSON( AIOUSBDevice *device );
AIOUSBDevice *NewAIOUSBDeviceFromJSON( char *str );
AIOUSBDevice *NewAIOUSBDeviceFromJSON( char *str );

AIORET_TYPE AIOUSBDeviceInitializeWithProductID( AIOUSBDevice *device , ProductIDS productID );
USBDevice *AIOUSBDeviceGetUSBHandle( AIOUSBDevice *dev );
USBDevice *AIOUSBDeviceGetUSBHandleFromDeviceIndex( unsigned long DeviceIndex, AIOUSBDevice **dev, AIORESULT *res );

AIORET_TYPE AIOUSBDeviceSetUSBHandle( AIOUSBDevice *dev, USBDevice *usb );

AIORET_TYPE AIOUSBDeviceSetADCConfigBlock( AIOUSBDevice *dev, ADCConfigBlock *conf );
ADCConfigBlock * AIOUSBDeviceGetADCConfigBlock( AIOUSBDevice *dev );
AIORET_TYPE AIOUSBDeviceCopyADCConfigBlock( AIOUSBDevice *dev, ADCConfigBlock *newone );


AIORET_TYPE AIOUSBDeviceSetTesting( AIOUSBDevice *dev, AIOUSB_BOOL testing );
AIORET_TYPE AIOUSBDeviceSize();
AIORET_TYPE AIOUSBDeviceGetTesting( AIOUSBDevice *dev  );
AIORET_TYPE AIOUSBDeviceGetStreamingBlockSize( AIOUSBDevice *deviceDesc );
AIORET_TYPE AIOUSBDeviceGetDiscardFirstSample( AIOUSBDevice *device );
AIORET_TYPE AIOUSBDeviceSetDiscardFirstSample( AIOUSBDevice *device , AIOUSB_BOOL discard );
AIORET_TYPE AIOUSBDeviceSetTimeout( AIOUSBDevice *device, unsigned timeout );
AIORET_TYPE AIOUSBDeviceGetTimeout( AIOUSBDevice *device );

AIORET_TYPE AIOUSBDeviceWriteADCConfig( AIOUSBDevice *device, ADCConfigBlock *config );

#ifdef __aiousb_cplusplus
}
#endif

#endif


