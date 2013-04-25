/*
 * $RCSfile: AIOUSB_CustomEEPROMRead.c,v $
 * $Revision: 1.4 $
 * $Date: 2009/11/02 17:17:08 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */



#include "AIOUSB_Core.h"



#ifdef __cplusplus
namespace AIOUSB {
#endif


unsigned long CustomEEPROMRead(
	unsigned long DeviceIndex
	, unsigned long StartAddress
	, unsigned long *DataSize
	, void *Data
) {
	/*
	 * EEPROM layout:
	 *   program code: 0x0000 -> EEPROM_CUSTOM_BASE_ADDRESS - 1
	 *   user space  : EEPROM_CUSTOM_BASE_ADDRESS -> EEPROM_CUSTOM_BASE_ADDRESS + EEPROM_CUSTOM_MAX_ADDRESS - 1
	 *                 (user space is addressed as 0 -> EEPROM_CUSTOM_MAX_ADDRESS - 1)
	 */
	if(
		StartAddress > EEPROM_CUSTOM_MAX_ADDRESS
		|| ( StartAddress + *DataSize ) > EEPROM_CUSTOM_MAX_ADDRESS + 1
		|| Data == NULL
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle
			, USB_READ_FROM_DEVICE, AUR_EEPROM_READ
			, EEPROM_CUSTOM_BASE_ADDRESS + StartAddress, 0
			, ( unsigned char * ) Data, *DataSize, timeout );
		if( bytesTransferred != ( int ) *DataSize )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CustomEEPROMRead()


#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
