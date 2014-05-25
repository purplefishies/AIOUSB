/**
 * @file   AIOUSB_Core.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 */

#ifndef AIOUSB_CORE_H
#define AIOUSB_CORE_H

#define PUBLIC_EXTERN extern
#define PRIVATE

#include "AIODataTypes.h"
#include "libusb.h"
#include <pthread.h>
#include <semaphore.h>

#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif

PUBLIC_EXTERN int aio_errno;

/* parameters passed from ADC_BulkAcquire() to its worker thread */
struct BulkAcquireWorkerParams {
    unsigned long DeviceIndex;
    unsigned long BufSize;
    void *pBuf;
};


enum {
    MAX_USB_DEVICES		  = 32
};

/*
 * DeviceDescriptor maintains property and state information for each ACCES USB device
 * on the bus; deviceTable[] is populated by PopulateDeviceTable(), which is automatically
 * called by AIOUSB_Init()
 */

typedef struct {
    libusb_device *device;              /**< NULL == no device */
    libusb_device_handle *deviceHandle; /**< libusb handles */
    AIOUSB_BOOL bOpen;
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
    unsigned DACsUsed;
    AIOUSB_BOOL bADCStream;
    unsigned ADCChannels;
    unsigned ADCMUXChannels;
    
    unsigned char RangeShift;

    unsigned ADCChannelsPerGroup;                     // number of A/D channels in each config. group (1, 4 or 8 depending on model)
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
    __uint64_t cachedSerialNumber;
    ADConfigBlock cachedConfigBlock;                  // .size == 0 == uninitialized

    /**
     * state of worker thread; these fields are deliberately unspecific so that
     * the library can employ worker threads in a variety of situations
     */
    AIOUSB_BOOL workerBusy;                                 // AIOUSB_TRUE == worker thread is busy
    unsigned long workerStatus;                             // thread-defined status information (e.g. bytes remaining to receive or transmit)
    unsigned long workerResult;                             // standard AIOUSB_* result code from worker thread (if workerBusy == AIOUSB_FALSE)

                                /** New entries for the FastIT behavior */
  
  ADConfigBlock *FastITConfig;
  ADConfigBlock *FastITBakConfig;
  unsigned long FastITConfig_size;
 
  unsigned char *ADBuf;
  int ADBuf_size;
 
} DeviceDescriptor;


#define PROD_NAME_SIZE 40

typedef struct  {
     unsigned int id;
     char name[ PROD_NAME_SIZE + 2 ];
} ProductIDName;


struct ADRange {
  double minVolts;
  double range;
};

extern struct ADRange adRanges[ AD_NUM_GAIN_CODES ];

extern unsigned long AIOUSB_INIT_PATTERN;
extern unsigned long aiousbInit ;


PUBLIC_EXTERN ADConfigBlock *AIOUSB_GetConfigBlock( unsigned long DeviceIndex );
PUBLIC_EXTERN unsigned long AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry);
PUBLIC_EXTERN AIOBuf *CreateSmartBuffer( unsigned long DeviceIndex );
PUBLIC_EXTERN unsigned long ADC_CopyConfig(unsigned long DeviceIndex, ADConfigBlock *config  );
PUBLIC_EXTERN unsigned long ADC_ResetDevice( unsigned long DeviceIndex  );
PUBLIC_EXTERN AIORET_TYPE AIOUSB_GetDeviceSerialNumber( unsigned long DeviceIndex );
PUBLIC_EXTERN AIORET_TYPE ADC_WriteADConfigBlock( unsigned long DeviceIndex , ADConfigBlock *config );

PUBLIC_EXTERN void PopulateDeviceTableTest(unsigned long *products, int length );


#ifndef SWIG
PUBLIC_EXTERN DeviceDescriptor deviceTable[ MAX_USB_DEVICES ];
PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_Lock(void);
PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_UnLock(void);

#define AIOUSB_IsInit()  ( aiousbInit == AIOUSB_INIT_PATTERN )
PUBLIC_EXTERN unsigned long AIOUSB_InitTest(void);
PUBLIC_EXTERN unsigned long AIOUSB_Validate( unsigned long *DeviceIndex );
PUBLIC_EXTERN unsigned long AIOUSB_Validate_Lock(  unsigned long *DeviceIndex ) ;
PUBLIC_EXTERN DeviceDescriptor *AIOUSB_GetDevice_Lock( unsigned long DeviceIndex , 
                                                        unsigned long *result
                                                        );


PUBLIC_EXTERN unsigned long AIOUSB_EnsureOpen( unsigned long DeviceIndex );
PUBLIC_EXTERN const char *ProductIDToName( unsigned int productID );
PUBLIC_EXTERN unsigned int ProductNameToID( const char *name );
PUBLIC_EXTERN const char *GetSafeDeviceName( unsigned long DeviceIndex );
PUBLIC_EXTERN struct libusb_device_handle *AIOUSB_GetDeviceHandle( unsigned long DeviceIndex );
PUBLIC_EXTERN int AIOUSB_BulkTransfer( struct libusb_device_handle *dev_handle,
                                        unsigned char endpoint, unsigned char *data, 
                                        int length, int *transferred, unsigned int timeout );

PUBLIC_EXTERN unsigned ADC_GetOversample_Cached( ADConfigBlock *config );
PUBLIC_EXTERN unsigned ADC_GainCode_Cached( ADConfigBlock *config, unsigned channel);
PUBLIC_EXTERN DeviceDescriptor *AIOUSB_GetDevice_NoCheck( unsigned long DeviceIndex  );
PUBLIC_EXTERN AIORET_TYPE cull_and_average_counts( unsigned long DeviceIndex, 
                                                   unsigned short *counts,
                                                   unsigned *size ,
                                                   unsigned numChannels
                                                   );

PUBLIC_EXTERN unsigned long AIOUSB_GetScan( unsigned long DeviceIndex, unsigned short counts[] );
PUBLIC_EXTERN unsigned long AIOUSB_ArrayCountsToVolts( unsigned long DeviceIndex, int startChannel,
                                                        int numChannels, const unsigned short counts[], double volts[] );
PUBLIC_EXTERN unsigned long AIOUSB_ArrayVoltsToCounts( unsigned long DeviceIndex, int startChannel,
                                                        int numChannels, const double volts[], unsigned short counts[] );


PUBLIC_EXTERN unsigned long GenericVendorRead( unsigned long deviceIndex, unsigned char Request, unsigned short Value, unsigned short Index, void *bufData , unsigned long *bytes_read  );

PUBLIC_EXTERN unsigned long GenericVendorWrite( unsigned long DeviceIndex, unsigned char Request, unsigned short Value, unsigned short Index, void *bufData, unsigned long *bytes_write );
PUBLIC_EXTERN unsigned long AIOUSB_Validate_Device( unsigned long DeviceIndex );




#endif




#if 0
/*
 * these will be moved to aiousb.h when they are ready to be made public
 */
extern unsigned long DACOutputProcess( unsigned long DeviceIndex, double *ClockHz, unsigned long NumSamples, unsigned short *pSampleData );
#endif



#ifdef __aiousb_cplusplus
}
#endif


#endif


/* end of file */
