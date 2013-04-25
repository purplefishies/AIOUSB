/*
 * $RCSfile: AIOUSB_GetDeviceBySerialNumber.c,v $
 * $Revision: 1.5 $
 * $Date: 2009/11/11 19:33:26 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */



#include "AIOUSB_Core.h"
#include <stdio.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif


unsigned long GetDeviceBySerialNumber( const __uint64_t *pSerialNumber ) {
	unsigned long deviceIndex = diNone;

	if( pSerialNumber == NULL )
		return deviceIndex;

	if( ! AIOUSB_Lock() )
		return deviceIndex;

	if( ! AIOUSB_IsInit() ) {
		AIOUSB_UnLock();
		return deviceIndex;
	}	

	int index;
	for( index = 0; index < MAX_USB_DEVICES; index++ ) {
		if( deviceTable[ index ].device != NULL ) {
			AIOUSB_UnLock();					// unlock while communicating with device
			__uint64_t deviceSerialNumber;
			const unsigned long result = GetDeviceSerialNumber( index, &deviceSerialNumber );
			AIOUSB_Lock();
			if(
				result == AIOUSB_SUCCESS
				&& deviceSerialNumber == *pSerialNumber
			) {
				deviceIndex = index;
				break;							// from for()
			}	
			/*
			 * else, even if we get an error requesting the serial number from
			 * this device, keep searching
			 */
		}	
	}	

	AIOUSB_UnLock();
	return deviceIndex;
}	// GetDeviceBySerialNumber()


#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
