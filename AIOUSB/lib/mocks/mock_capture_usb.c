/**
 * @file   mock_capture_usb.c
 * @author Jimi Damon <james.damon@accesio.com>
 * @date   Tue Feb 17 12:01:40 2015
 * 
 * @brief  This file will allow capturing of all USB traffic, in and out
 * 
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "AIOTypes.h"
#include "USBDevice.h"
#include "AIOUSB_Core.h"
#include "AIOUSB_Log.h"

#include <dlfcn.h>

int (*orig_usb_control_transfer)( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout );

int (*orig_usb_bulk_transfer)( struct aiousb_device *dev_handle,
                  unsigned char endpoint, unsigned char *data, int length,
                  int *actual_length, unsigned int timeout );

int (*orig_usb_request)( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout );
int (*orig_usb_reset_device)(struct aiousb_device *usbdev );
int (*orig_usb_put_config)( struct aiousb_device *usb, ADCConfigBlock *configBlock );
int (*orig_usb_get_config)( struct aiousb_device *usb, ADCConfigBlock *configBlock );

int mock_usb_control_transfer( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout )
{
    printf("Wrapping control ");
    if ( request_type ==  USB_WRITE_TO_DEVICE ) {
        printf("(out)\n");
    } else if ( request_type == USB_READ_FROM_DEVICE ) {
        printf("(in)\n");
    }
    return orig_usb_control_transfer( usbdev, request_type, bRequest, wValue, wIndex, data, wLength, timeout );
}

int mock_usb_bulk_transfer( struct aiousb_device *dev_handle,
                            unsigned char endpoint, unsigned char *data, int length,
                            int *actual_length, unsigned int timeout )
{

    printf("Wrapping bulk ");
    if ( endpoint & LIBUSB_ENDPOINT_OUT ) {
        printf("(out)\n");
    } else if ( endpoint & LIBUSB_ENDPOINT_IN ) {
        printf("(in)\n");
    }

    return orig_usb_bulk_transfer( dev_handle, endpoint, data, length, actual_length, timeout );

}

int mock_usb_request( struct aiousb_device *usbdev, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout )
{
    printf("wrapping usb_request\n");
    return orig_usb_request( usbdev, request_type, bRequest, wValue, wIndex, data, wLength, timeout );
}

int mock_usb_reset_device(struct aiousb_device *usbdev )
{
    printf("Wrapping reset_device\n");
    return orig_usb_reset_device( usbdev );
}
 
int mock_usb_put_config( struct aiousb_device *usb, ADCConfigBlock *configBlock )
{
    printf("Wrapping put_config\n");
    return orig_usb_put_config( usb, configBlock );
}

int mock_usb_get_config( struct aiousb_device *usb, ADCConfigBlock *configBlock )
{
    printf("Wrapping get_config\n");
    return orig_usb_get_config( usb, configBlock );
}

/**
 * @brief Wraps the initial IntializeUSBDevice, and records mock
 * functions that will call the initial values.
 *
 */
AIOEither InitializeUSBDevice( USBDevice *usb, LIBUSBArgs *args )
{
    AIOEither retval = {0};
    static AIOEither (*init_usb_device)(USBDevice *usb, LIBUSBArgs *args ) = NULL;

    unsigned char *c;
    int port,ok=1;

    if (!init_usb_device) 
        init_usb_device = dlsym(RTLD_NEXT,"InitializeUSBDevice");

    printf("Wrapped the original !!\n");
    retval = init_usb_device( usb, args );

    if ( !AIOEitherHasError( &retval ) ) {
        orig_usb_control_transfer  = usb->usb_control_transfer;
        orig_usb_bulk_transfer     = usb->usb_bulk_transfer;
        orig_usb_request           = usb->usb_request;
        orig_usb_reset_device      = usb->usb_reset_device;
        orig_usb_put_config        = usb->usb_put_config;
        orig_usb_get_config        = usb->usb_get_config;

        usb->usb_control_transfer  = mock_usb_control_transfer;
        usb->usb_bulk_transfer     = mock_usb_bulk_transfer;
        usb->usb_put_config        = mock_usb_put_config;
        usb->usb_get_config        = mock_usb_get_config;
        usb->usb_reset_device      = mock_usb_reset_device;
        usb->usb_request           = mock_usb_request;
    }

    printf("Done calling the original !!\n");

    return retval;
}


