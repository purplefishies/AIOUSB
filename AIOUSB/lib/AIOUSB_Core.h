/**
 * @file   AIOUSB_Core.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 * 
 * 
 */

#if !defined( AIOUSB_Core_h )
#define AIOUSB_Core_h


#include "aiousb.h"
#include "libusb.h"
#include <pthread.h>
#include <semaphore.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif



#define PRIVATE_EXTERN extern
#define PRIVATE



enum {
    // partially defined in "Low-Level Vendor Request Reference.pdf"
    AUR_DIO_WRITE                           = 0x10,
    AUR_DIO_READ                            = 0x11,
    AUR_DIO_CONFIG                          = 0x12,
    AUR_DIO_CONFIG_QUERY                    = 0x13,
    AUR_CTR_READ                            = 0x20,
    AUR_CTR_MODE                            = 0x21,
    AUR_CTR_LOAD                            = 0x22,
    AUR_CTR_MODELOAD                        = 0x23,
    AUR_CTR_SELGATE                         = 0x24,
    AUR_CTR_READALL                         = 0x25,
    AUR_CTR_READLATCHED                     = 0x26,
    AUR_CTR_COS_BULK_GATE2                  = 0x27,
    AUR_CTR_PUR_FIRST                       = 0x28,   // 0x28 is correct; not used with device, for index offsetting
    AUR_CTR_PUR_OFRQ                        = 0x28,   // 0x28 is correct
    AUR_CTR_COS_BULK_ABORT                  = 0x29,
    AUR_CTR_PUR_MFRQ                        = 0x2C,
    AUR_CTR_PUR_EVCT                        = 0x2D,
    AUR_CTR_PUR_MPUL                        = 0x2E,
    AUR_DIO_WDG16_DEPREC                    = 0x2F,
    AUR_READBACK_GLOBAL_STATE               = 0x30,
    AUR_SAVE_GLOBAL_STATE                   = 0x31,
    AUR_GEN_CLEAR_FIFO_NEXT                 = 0x34,
    AUR_GEN_CLEAR_FIFO                      = 0x35,
    AUR_GEN_CLEAR_FIFO_WAIT                 = 0x36,
    AUR_GEN_ABORT_AND_CLEAR                 = 0x38,
    AUR_WDG                                 = 0x44,
    AUR_OFFLINE_READWRITE                   = 0x50,
    AUR_SELF_TEST_1                         = 0x91,
    AUR_EEPROM_READ                         = 0xA2,
    AUR_EEPROM_WRITE                        = 0XA2,
    AUR_DAC_CONTROL                         = 0xB0,
    AUR_DAC_DATAPTR                         = 0xB1,
    AUR_DAC_DIVISOR                         = 0xB2,
    AUR_DAC_IMMEDIATE                       = 0xB3,
    AUR_GEN_STREAM_STATUS                   = 0xB4,
    AUR_FLASH_READWRITE                     = 0xB5,
    AUR_DAC_RANGE                           = 0xB7,
    AUR_PROBE_CALFEATURE                    = 0xBA,
    AUR_LOAD_BULK_CALIBRATION_BLOCK         = 0xBB,
    AUR_DIO_STREAM_OPEN_OUTPUT              = 0xBB,
    AUR_START_ACQUIRING_BLOCK               = 0xBC,
    AUR_DIO_STREAM_OPEN_INPUT               = 0xBC,
    AUR_DIO_SETCLOCKS                       = 0xBD,
    AUR_ADC_SET_CONFIG                      = 0xBE,
    AUR_ADC_IMMEDIATE                       = 0xBF,
    AUR_DIO_SPI_WRITE                       = 0xC0,
    AUR_DIO_SPI_READ                        = 0xC1,
    AUR_ADC_GET_CONFIG                      = 0xD2
};    // enum



enum {
    BITS_PER_BYTE                 = 8,
    AI_16_MAX_COUNTS              = 65535,
    MAX_IMM_ADCS                  = 2,                // maximum number of "immediate" A/Ds in any device
    CAL_TABLE_WORDS               = ( 64 * 1024 ),    // 64K 2-byte words
    COUNTERS_PER_BLOCK            = 3,
    COUNTER_NUM_MODES             = 6,
    DAC_RESET                     = 0x80,

    EEPROM_SERIAL_NUMBER_ADDRESS  = 0x1DF8,
    EEPROM_CUSTOM_BASE_ADDRESS    = 0x1E00,
    EEPROM_CUSTOM_MIN_ADDRESS     = 0,
    EEPROM_CUSTOM_MAX_ADDRESS     = 0x1FF,

    AD_CONFIG_REGISTERS           = 20,             // number of "registers" (bytes) in A/D config. block of boards without MUX
    AD_MUX_CONFIG_REGISTERS       = 21,             // number of "registers" (bytes) in A/D config. block of boards with MUX

    USB_WRITE_TO_DEVICE           = 0x40,
    USB_READ_FROM_DEVICE          = 0xC0,
    USB_BULK_WRITE_ENDPOINT       = 2,
    USB_BULK_READ_ENDPOINT        = 6
};    // enum




// parameters passed from ADC_BulkAcquire() to its worker thread
struct BulkAcquireWorkerParams {
    unsigned long DeviceIndex;
    unsigned long BufSize;
    void *pBuf;
};    // struct BulkAcquireWorkerParams





/*
 * DeviceDescriptor maintains property and state information for each ACCES USB device
 * on the bus; deviceTable[] is populated by PopulateDeviceTable(), which is automatically
 * called by AIOUSB_Init()
 */

enum {
    MAX_USB_DEVICES		  = 32
};    // enum

typedef struct {
    // libusb handles
    libusb_device *device;                                  // NULL == no device
    libusb_device_handle *deviceHandle;
    AIOUSB_BOOL bOpen;
    unsigned long PID;

    unsigned long DIOConfigBits;
    


    // run-time settings
    AIOUSB_BOOL discardFirstSample;                         // AIOUSB_TRUE == discard first A/D sample in all A/D read methods
    unsigned commTimeout;                                   // timeout for device communication (ms.)
    double miscClockHz;                                           // miscellaneous clock frequency setting

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

    /*
     * state of worker thread; these fields are deliberately unspecific so that
     * the library can employ worker threads in a variety of situations
     */
    AIOUSB_BOOL workerBusy;                                 // AIOUSB_TRUE == worker thread is busy
    unsigned long workerStatus;                             // thread-defined status information (e.g. bytes remaining to receive or transmit)
    unsigned long workerResult;                             // standard AIOUSB_* result code from worker thread (if workerBusy == AIOUSB_FALSE)

  /* New entries for the FastIT behavior */
  
  ADConfigBlock *FastITConfig;
  ADConfigBlock *FastITBakConfig;
  unsigned long FastITConfig_size;
 
  unsigned char *ADBuf;
  int ADBuf_size;
 
} DeviceDescriptor;

DeviceDescriptor *AIOUSB_getDevice( unsigned long DeviceIndex );

extern unsigned long AIOUSB_INIT_PATTERN;
extern unsigned long aiousbInit ;


PRIVATE_EXTERN ADConfigBlock *AIOUSB_GetConfigBlock( unsigned long DeviceIndex );
PRIVATE_EXTERN unsigned long AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry);

#ifndef SWIG
PRIVATE_EXTERN DeviceDescriptor deviceTable[ MAX_USB_DEVICES ];

PRIVATE_EXTERN AIOUSB_BOOL AIOUSB_Lock(void);
PRIVATE_EXTERN AIOUSB_BOOL AIOUSB_UnLock(void);

#define AIOUSB_IsInit()  ( aiousbInit == AIOUSB_INIT_PATTERN )

/* PRIVATE_EXTERN AIOUSB_BOOL AIOUSB_IsInit(void); */
PRIVATE_EXTERN unsigned long AIOUSB_Validate( unsigned long *DeviceIndex );
PRIVATE_EXTERN unsigned long AIOUSB_Validate_Lock(  unsigned long *DeviceIndex ) ;
PRIVATE_EXTERN DeviceDescriptor *AIOUSB_GetDevice_Lock( unsigned long DeviceIndex , 
                                                        unsigned long *result
                                                        );




PRIVATE_EXTERN unsigned long AIOUSB_EnsureOpen( unsigned long DeviceIndex );
PRIVATE_EXTERN const char *ProductIDToName( unsigned int productID );
PRIVATE_EXTERN unsigned int ProductNameToID( const char *name );
PRIVATE_EXTERN const char *GetSafeDeviceName( unsigned long DeviceIndex );
PRIVATE_EXTERN struct libusb_device_handle *AIOUSB_GetDeviceHandle( unsigned long DeviceIndex );
PRIVATE_EXTERN int AIOUSB_BulkTransfer( struct libusb_device_handle *dev_handle,
                                        unsigned char endpoint, unsigned char *data, 
                                        int length, int *transferred, unsigned int timeout );



PRIVATE_EXTERN unsigned long AIOUSB_GetScan( unsigned long DeviceIndex, unsigned short counts[] );
PRIVATE_EXTERN unsigned long AIOUSB_ArrayCountsToVolts( unsigned long DeviceIndex, int startChannel,
                                                        int numChannels, const unsigned short counts[], double volts[] );
PRIVATE_EXTERN unsigned long AIOUSB_ArrayVoltsToCounts( unsigned long DeviceIndex, int startChannel,
                                                        int numChannels, const double volts[], unsigned short counts[] );


#endif

#if 0
/*
 * these will be moved to aiousb.h when they are ready to be made public
 */
extern unsigned long GenericVendorRead( unsigned long DeviceIndex, unsigned char Request, unsigned short Value, unsigned short Index, unsigned long *DataSize, void *Data );
extern unsigned long GenericVendorWrite( unsigned long DeviceIndex, unsigned char Request, unsigned short Value, unsigned short Index, unsigned long DataSize, void *Data );
extern unsigned long DACOutputProcess( unsigned long DeviceIndex, double *ClockHz, unsigned long NumSamples, unsigned short *pSampleData );
#endif



#ifdef __cplusplus
}     // namespace AIOUSB
#endif


#endif


/* end of file */
