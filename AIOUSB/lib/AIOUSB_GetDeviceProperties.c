/*
 * $RCSfile: AIOUSB_GetDeviceProperties.c,v $
 * $Revision: 1.9 $
 * $Date: 2009/12/23 22:41:21 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */



#include "AIOUSB_Core.h"



#ifdef __cplusplus
namespace AIOUSB {
#endif


/*
 * AIOUSB_GetDeviceProperties() returns a richer amount of information than QueryDeviceInfo()
 */
unsigned long AIOUSB_GetDeviceProperties(
	unsigned long DeviceIndex
	, DeviceProperties *properties
) {
	if( properties == 0 )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	properties->Name = deviceDesc->cachedName;	// if NULL, name will be requested from device
	properties->SerialNumber = deviceDesc->cachedSerialNumber;	// if 0, serial number will be requested from device
	properties->ProductID = deviceDesc->ProductID;
	properties->DIOPorts = deviceDesc->DIOBytes;
	properties->Counters = deviceDesc->Counters;
	properties->Tristates = deviceDesc->Tristates;
	properties->RootClock = deviceDesc->RootClock;
	properties->DACChannels = deviceDesc->ImmDACs;
	properties->ADCChannels = deviceDesc->ADCChannels;
	properties->ADCMUXChannels = deviceDesc->ADCMUXChannels;
	properties->ADCChannelsPerGroup = deviceDesc->ADCChannelsPerGroup;
	AIOUSB_UnLock();							// unlock while communicating with device
	if( properties->Name == NULL )
		properties->Name = GetSafeDeviceName( DeviceIndex );
	if( properties->SerialNumber == 0 )
		result = GetDeviceSerialNumber( DeviceIndex, &properties->SerialNumber );

	return result;
}	// AIOUSB_GetDeviceProperties()


#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
