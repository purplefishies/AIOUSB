#ifndef _USB_DEVICE_H
#define _USB_DEVICE_H

#include <stdint.h>
#include <libusb.h>
#include <stdlib.h>
#include "ADCConfigBlock.h"
#include "AIOEither.h"

#ifdef __aiousb_cplusplus
namespace AIOUSB {
#endif

typedef struct aiousb_device { 

    int (*usb_control_transfer)( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout );

    int (*usb_bulk_transfer)( struct aiousb_device *dev_handle,
                      unsigned char endpoint, unsigned char *data, int length,
                      int *actual_length, unsigned int timeout );

    int (*usb_request)( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout );
    int (*usb_reset_device)(struct aiousb_device *usbdev );
    int (*usb_put_config)( struct aiousb_device *usb, ADCConfigBlock *configBlock );
    int (*usb_get_config)( struct aiousb_device *usb, ADCConfigBlock *configBlock );

    uint8_t timeout;
    libusb_device *device;
    libusb_device_handle *deviceHandle;
    struct libusb_device_descriptor deviceDesc;
    AIOUSB_BOOL debug;
    int usblp_attached;
    int iface;
    int verbose;
    int conf;
    int origconf;
    int altset;
} USBDevice;

typedef struct aiousb_libusb_args {
    struct libusb_device *dev;
    struct libusb_device_handle *handle;
    struct libusb_device_descriptor *deviceDesc;
} LIBUSBArgs;


USBDevice * NewUSBDevice(libusb_device *dev, libusb_device_handle *handle );
void DeleteUSBDevice( USBDevice *dev );
USBDevice *CopyUSBDevice( USBDevice *usb );

AIOEither InitializeUSBDevice( USBDevice *usb, LIBUSBArgs *args );

AIORET_TYPE AddAllACCESUSBDevices( libusb_device **deviceList , USBDevice **devs , int *size );
void DeleteUSBDevices( USBDevice *devs);

int USBDeviceClose( USBDevice *dev );


USBDevice *AIODeviceTableGetUSBDevice( unsigned long DeviceIndex , AIORESULT *res );
int USBDeviceReadADCConfigBlock( USBDevice *usb, ADCConfigBlock *configBlock );
int USBDeviceGetIdProduct( USBDevice *device );
int USBDeviceFetchADCConfigBlock( USBDevice *device, ADCConfigBlock *config );
int USBDevicePutADCConfigBlock( USBDevice *usb, ADCConfigBlock *configBlock );

int usb_control_transfer(struct aiousb_device *dev_handle,
                                 uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                 unsigned char *data, uint16_t wLength, unsigned int timeout);
int usb_bulk_transfer(struct aiousb_device *dev_handle,
                              unsigned char endpoint, unsigned char *data, int length,
                              int *actual_length, unsigned int timeout);
int usb_request(struct aiousb_device *dev_handle,
                        uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                        unsigned char *data, uint16_t wLength, unsigned int timeout);
int usb_reset_device( struct aiousb_device *usb );

 
libusb_device_handle *get_usb_device( USBDevice *dev );
libusb_device_handle *USBDeviceGetUSBDeviceHandle( USBDevice *usb );


#ifdef __aiousb_cplusplus
}
#endif

#endif
