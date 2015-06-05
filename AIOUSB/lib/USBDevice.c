/**
 * @file   USBDevice.c
 * @author  $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 */

#include "AIOTypes.h"
#include "USBDevice.h"
#include "libusb.h"
#include "AIODeviceTable.h"
#include "AIOEither.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
AIOEither InitializeUSBDevice( USBDevice *usb, LIBUSBArgs *args )
{
    AIOEither retval = {0};

    AIO_ASSERT_AIOEITHER(-AIOUSB_ERROR_INVALID_USBDEVICE,"Invalid usb object", usb );
    AIO_ASSERT_AIOEITHER(-AIOUSB_ERROR_INVALID_PARAMETER,"Invalid args", args ); 

    usb->device                = args->dev;
    usb->deviceHandle          = args->handle;
    usb->deviceDesc            = *args->deviceDesc;

    int libusbResult = libusb_open(  usb->device, &usb->deviceHandle );
    
    if( libusbResult == LIBUSB_SUCCESS && usb->deviceHandle != NULL ) {
        int kernelActive = libusb_kernel_driver_active( usb->deviceHandle, 0 );
        if ( kernelActive == 1 ) {
            libusbResult = libusb_claim_interface( usb->deviceHandle, 0 );
            libusbResult = libusb_attach_kernel_driver( usb->deviceHandle, 0 );
        }

        usb->debug = AIOUSB_FALSE;
        usb->usb_control_transfer  = usb_control_transfer;
        usb->usb_bulk_transfer     = usb_bulk_transfer;
        usb->usb_request           = usb_request;
        usb->usb_reset_device      = usb_reset_device;
        usb->usb_put_config        = USBDevicePutADCConfigBlock;
        usb->usb_get_config        = USBDeviceFetchADCConfigBlock;

    } else {
        retval.left = -libusbResult;
        asprintf(&retval.errmsg,"Error with libusb_open: %d\n", libusbResult );
    }

    return retval;
}

/*----------------------------------------------------------------------------*/
USBDevice * NewUSBDevice( libusb_device *dev, libusb_device_handle *handle)
{
    USBDevice *obj = (USBDevice *)calloc(sizeof(USBDevice), 1 );
    if ( obj ) {
        LIBUSBArgs args = { dev, handle, NULL };
        InitializeUSBDevice( obj, &args ) ;
    }
    return obj;
}

/*----------------------------------------------------------------------------*/
USBDevice *CopyUSBDevice( USBDevice *usb )
{
    USBDevice *newusb = (USBDevice *)calloc(sizeof(USBDevice), 1 );
    memcpy(newusb, usb, sizeof(USBDevice));
    return newusb;
}


/*----------------------------------------------------------------------------*/
int USBDeviceClose( USBDevice *usb )
{
    AIO_ASSERT_USB(usb);
    
    libusb_close(usb->deviceHandle);
    usb->deviceHandle = NULL;

    libusb_unref_device( usb->device );

    return AIOUSB_SUCCESS;
}


/*----------------------------------------------------------------------------*/
int FindUSBDevices( USBDevice **devs, int *size )
{
    int result = 0;

    libusb_device **deviceList = 0;

    AIO_ASSERT_VALID_DATA( -AIOUSB_ERROR_INVALID_DATA, devs );
    AIO_ASSERT( size );

    *size = 0;

    AddAllACCESUSBDevices( deviceList, devs, size );

    libusb_free_device_list(deviceList, AIOUSB_TRUE);

    return result;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AddAllACCESUSBDevices( libusb_device **deviceList , USBDevice **devs , int *size )
{
    AIORET_TYPE result = AIOUSB_SUCCESS;
    int numAccesDevices = 0;
    int numDevices = libusb_get_device_list(NULL, &deviceList);
    if (numDevices > 0) {
        for ( int index = 0; index < numDevices && numAccesDevices < MAX_USB_DEVICES; index++, numAccesDevices ++) {
            struct libusb_device_descriptor libusbDeviceDesc;
            libusb_device *usb_device = deviceList[ index ];

            int libusbResult = libusb_get_device_descriptor(usb_device, &libusbDeviceDesc);

            if (libusbResult == LIBUSB_SUCCESS) {

                if (libusbDeviceDesc.idVendor == ACCES_VENDOR_ID) {
                    *size += 1;
                    *devs = (USBDevice*)realloc( *devs, (*size )*(sizeof(USBDevice)));
                    LIBUSBArgs args = { libusb_ref_device(usb_device), NULL, &libusbDeviceDesc };
                    AIOEither usbretval = InitializeUSBDevice( &( *devs)[*size-1] , &args );
                    if ( AIOEitherHasError( &usbretval ) )
                        return -AIOUSB_ERROR_USB_INIT;
                    result += 1;
                }
            }
        }
    }
    return (AIORET_TYPE)result;
}


/*----------------------------------------------------------------------------*/
 AIORET_TYPE AddDevice( int *size , int index, libusb_device **deviceList, USBDevice **devs , struct libusb_device_descriptor *libusbDeviceDesc )
{
    AIORET_TYPE retval  = AIOUSB_SUCCESS;
    libusb_device *usb_device = deviceList[ index ];
    *size += 1;
    *devs = (USBDevice*)realloc( *devs, (*size )*(sizeof(USBDevice)));
    LIBUSBArgs args = { libusb_ref_device(usb_device), NULL, libusbDeviceDesc };
    AIOEither usbretval = InitializeUSBDevice( &( *devs)[*size-1] , &args );
    if ( AIOEitherHasError( &usbretval ) )
        return -AIOUSB_ERROR_USB_INIT;
    retval = (AIORET_TYPE)1;
    return retval;
}

/*----------------------------------------------------------------------------*/
int USBDeviceGetIdProduct( USBDevice *device )
{
    AIO_ASSERT_USB(device);

    return (int)device->deviceDesc.idProduct;
}

/*----------------------------------------------------------------------------*/
void DeleteUSBDevices( USBDevice *devices )
{
    AIO_ASSERT_NO_RETURN( devices );

    free(devices);
    libusb_exit(NULL);
}

/*----------------------------------------------------------------------------*/
void DeleteUSBDevice( USBDevice *dev )
{
    AIO_ASSERT_NO_RETURN(dev);

    free(dev);
}

/*----------------------------------------------------------------------------*/
int USBDeviceSetDebug( USBDevice *usb, AIOUSB_BOOL debug )
{
    AIO_ASSERT_USB(usb);

    usb->debug = debug;
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
libusb_device_handle *USBDeviceGetUSBDeviceHandle( USBDevice *usb )
{
    if( !usb )
        return NULL;
    return usb->deviceHandle;
}

/*----------------------------------------------------------------------------*/
libusb_device_handle *get_usb_device( USBDevice *dev )
{
    
    if ( !dev ) 
        return NULL;
    return dev->deviceHandle;
}

/*----------------------------------------------------------------------------*/
int USBDeviceFetchADCConfigBlock( USBDevice *usb, ADCConfigBlock *configBlock )
{
    int result = AIOUSB_SUCCESS;
    AIOUSBDevice dev;
    ADCConfigBlock config;

    AIO_ASSERT_CONFIG(configBlock);
    AIO_ASSERT_USB(usb);

    ADCConfigBlockInitializeFromAIOUSBDevice( &config , &dev );
    config.timeout = configBlock->timeout;

    if( configBlock->testing != AIOUSB_TRUE ) {
        int bytesTransferred = usb->usb_control_transfer( usb, 
                                                          USB_READ_FROM_DEVICE,
                                                          AUR_ADC_GET_CONFIG,
                                                          0,
                                                          0,
                                                          config.registers,
                                                          config.size,
                                                          config.timeout
                                                          );
        
        if ( bytesTransferred != ( int ) config.size)
            result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
        else
            result = ADCConfigBlockCopy( configBlock, &config );
    } else {
        result = configBlock->size;
    }

    return result;
}

/*----------------------------------------------------------------------------*/
int USBDevicePutADCConfigBlock( USBDevice *usb, ADCConfigBlock *configBlock )
{
    int retval;

    AIO_ASSERT_USB(usb);
    AIO_ASSERT_CONFIG( configBlock );

    if( configBlock->testing == AIOUSB_TRUE ) {
        retval = (int)configBlock->size;
    } else {
        int bytesTransferred = usb->usb_control_transfer( usb, 
                                                          USB_WRITE_TO_DEVICE,
                                                          AUR_ADC_SET_CONFIG,
                                                          0,
                                                          0,
                                                          configBlock->registers,
                                                          configBlock->size,
                                                          configBlock->timeout
                                                          );
        if ( bytesTransferred != (int)configBlock->size ) {
            retval = -LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
        } else {
            retval = bytesTransferred;
        }
    }
    return retval;
}

/*----------------------------------------------------------------------------*/
int usb_control_transfer(struct aiousb_device *dev_handle,
                         uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                         unsigned char *data, uint16_t wLength, unsigned int timeout)
{

    libusb_device_handle *handle = get_usb_device( dev_handle );
    AIO_ERROR_VALID_DATA(-AIOUSB_ERROR_INVALID_LIBUSB_DEVICE_HANDLE, handle );

    return libusb_control_transfer( handle,
                                    request_type,
                                    bRequest,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength, 
                                    timeout
                                    );

}

/*----------------------------------------------------------------------------*/
/**
 * @details This function is intended to improve upon
 * libusb_bulk_transfer() by receiving or transmitting packets until
 * the entire transfer request has been satisfied; it intentionally
 * restarts the timeout each time a packet is received, so the timeout
 * parameter specifies the longest permitted delay between packets,
 * not the total time to complete the transfer request
 */
int usb_bulk_transfer( USBDevice *usb,
                       unsigned char endpoint, 
                       unsigned char *data, 
                       int length,
                       int *actual_length, 
                       unsigned int timeout
                      )
{
    int libusbResult = LIBUSB_SUCCESS;
    int total = 0;

    AIO_ASSERT_USB( usb );
    AIO_ASSERT( data );
    AIO_ASSERT( actual_length );

    libusb_device_handle *handle = get_usb_device( usb );
    AIO_ERROR_VALID_DATA(-AIOUSB_ERROR_INVALID_LIBUSB_DEVICE_HANDLE, handle );

    while (length > 0) {
          int bytes;
          libusbResult = libusb_bulk_transfer( handle , 
                                               endpoint, 
                                               data, 
                                               length, 
                                               &bytes, 
                                               timeout
                                               );
          if (libusbResult == LIBUSB_SUCCESS) {
              if(bytes > 0) {
                  total += bytes;
                  data += bytes;
                  length -= bytes;
              }
          } else if(libusbResult == LIBUSB_ERROR_TIMEOUT) {
            /**
             * @note even if we get a timeout, some data may have been
             * transferred; if so, then this timeout is not an error;
             * if we get a timeout and no data was transferred, then
             * treat it as an error condition
             */
              if (bytes > 0) {
                  total += bytes;
                  data += bytes;
                  length -= bytes;
              } else
                  break;
          } else
              break;
    }
    *actual_length = total;
    return libusbResult;
}

/*----------------------------------------------------------------------------*/
int usb_request(struct aiousb_device *dev_handle,
                        uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                        unsigned char *data, uint16_t wLength, unsigned int timeout)
{
    return 1;
}

/*----------------------------------------------------------------------------*/
int usb_reset_device( struct aiousb_device *usb )
{
    AIO_ASSERT_USB( usb );

    int libusbResult = libusb_reset_device( usb->deviceHandle  );
    return libusbResult;
}



#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST
/**
 * @brief Self test for verifying basic functionality of the AIOChannelMask interface
 */ 

#include "gtest/gtest.h"
#include "tap.h"
using namespace AIOUSB;


TEST(USBDevice,FindDevices ) 
{
    USBDevice *devs = NULL;
    int size = 0;
    libusb_init( NULL );        /* Required for FindUSBDevices */
    FindUSBDevices( &devs, &size );
    
    EXPECT_GE( size, 0 );
    
    for ( int i = 0 ;i < size ; i ++ ) {
        EXPECT_GE( USBDeviceGetIdProduct( &devs[i] ), 0 );
    }

}

TEST(USBDevice,FailsCorrectly)
{
    unsigned char endpoint;
    unsigned char *data = 0;
    int length;
    int *actual_length = 0;
    unsigned int timeout;
    USBDevice *usb = NULL; 
    ASSERT_DEATH( { USBDeviceClose(usb); }, "Assertion `usb' failed.");
    ASSERT_DEATH( { usb_bulk_transfer(usb,endpoint,data,length,actual_length,timeout); }, "Assertion `usb' failed.");
    usb = (USBDevice *)42;
    ASSERT_DEATH( { usb_bulk_transfer(usb,endpoint,data,length,actual_length,timeout); }, "Assertion `data' failed");
    data = (unsigned char *)42;
    ASSERT_DEATH( { usb_bulk_transfer(usb,endpoint,data,length,actual_length,timeout); }, "Assertion `actual_length' failed");

    usb = NULL;
    LIBUSBArgs *args = NULL;
    ASSERT_DEATH( { InitializeUSBDevice(usb, args); } , "Assertion `usb' failed" );
    usb = (USBDevice*)42;
    ASSERT_DEATH( { InitializeUSBDevice(usb, args); } , "Assertion `args' failed" );
}


int main(int argc, char *argv[] )
{
  int retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
  return RUN_ALL_TESTS();  

}

#endif

