/**
 * @file   
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @copy
 * @brief
 *  ACCES I/O USB API for Linux
 *
 */

#include "AIOUSB_Core.h"
#include <math.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif



unsigned long CTR_8254Mode(
	unsigned long DeviceIndex, 
        unsigned long BlockIndex, 
        unsigned long CounterIndex, 
        unsigned long Mode
) {
	if( Mode >= COUNTER_NUM_MODES )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	}

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const unsigned short controlValue
			= ( ( unsigned short ) CounterIndex << ( 6 + 8 ) )
			| ( 0x3u << ( 4 + 8 ) )
			| ( ( unsigned short ) Mode << ( 1 + 8 ) )
			| ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, 
                                                                      USB_WRITE_TO_DEVICE, 
                                                                      AUR_CTR_MODE,
                                                                      controlValue, 
                                                                      0, 
                                                                      0, 
                                                                      0 /* wLength */, 
                                                                      timeout 
                                                                      );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CTR_8254Mode()



unsigned long CTR_8254Load(
	unsigned long DeviceIndex, 
        unsigned long BlockIndex, 
        unsigned long CounterIndex, 
        unsigned short LoadValue
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	}

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock(); // unlock while communicating with device
		const unsigned short controlValue
			= ( ( unsigned short ) CounterIndex << ( 6 + 8 ) )
			// | ( 0x3u << ( 4 + 8 ) )
			// | ( ( unsigned short ) Mode << ( 1 + 8 ) )
			| ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_CTR_LOAD
			, controlValue, LoadValue, 0, 0 /* wLength */, timeout );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long CTR_8254ModeLoad(
	unsigned long DeviceIndex, 
        unsigned long BlockIndex, 
        unsigned long CounterIndex, 
        unsigned long Mode, 
        unsigned short LoadValue
) {
	if( Mode >= COUNTER_NUM_MODES )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	}

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const unsigned short controlValue
			= ( ( unsigned short ) CounterIndex << ( 6 + 8 ) )
			| ( 0x3u << ( 4 + 8 ) )
			| ( ( unsigned short ) Mode << ( 1 + 8 ) )
			| ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_CTR_MODELOAD
			, controlValue, LoadValue, 0, 0 /* wLength */, timeout );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long CTR_8254ReadModeLoad(
	unsigned long DeviceIndex, 
        unsigned long BlockIndex, 
        unsigned long CounterIndex, 
        unsigned long Mode, 
        unsigned short LoadValue, 
        unsigned short *pReadValue
) {
	if(
		Mode >= COUNTER_NUM_MODES
		|| pReadValue == NULL
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
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	}

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		unsigned short readValue;
		const unsigned short controlValue
			= ( ( unsigned short ) CounterIndex << ( 6 + 8 ) )
			| ( 0x3u << ( 4 + 8 ) )
			| ( ( unsigned short ) Mode << ( 1 + 8 ) )
			| ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_CTR_MODELOAD
			, controlValue, LoadValue, ( unsigned char * ) &readValue, sizeof( readValue ), timeout );
		if( bytesTransferred == sizeof( readValue ) ) {
			// TODO: verify endian mode; original code had it reversed
			*pReadValue = readValue;
		} else
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long CTR_8254Read(
	unsigned long DeviceIndex, 
        unsigned long BlockIndex, 
        unsigned long CounterIndex, 
        unsigned short *pReadValue
) {
	if( pReadValue == NULL )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}
	}

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		unsigned short readValue;
		const unsigned short controlValue = (( unsigned short ) CounterIndex << 8 ) | ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, 
                                                                      USB_READ_FROM_DEVICE, 
                                                                      AUR_CTR_READ,			
                                                                      controlValue, 
                                                                      0, 
                                                                      ( unsigned char * ) &readValue, 
                                                                      sizeof( readValue ), 
                                                                      timeout 
                                                                      );
		if( bytesTransferred == sizeof( readValue ) ) {
			// TODO: verify endian mode; original code had it reversed
			*pReadValue = readValue;
		} else
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long CTR_8254ReadAll(
	unsigned long DeviceIndex, 
        unsigned short *pData
) {
	if( pData == NULL )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const int READ_BYTES = deviceDesc->Counters * COUNTERS_PER_BLOCK * sizeof( unsigned short );
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_READ_FROM_DEVICE, AUR_CTR_READALL
			, 0, 0, ( unsigned char * ) pData, READ_BYTES, timeout );
		if( bytesTransferred != READ_BYTES )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CTR_8254ReadAll()



unsigned long CTR_8254ReadStatus(
	unsigned long DeviceIndex
	, unsigned long BlockIndex
	, unsigned long CounterIndex
	, unsigned short *pReadValue
	, unsigned char *pStatus
) {
	if(
		pReadValue == NULL
		|| pStatus == NULL
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
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	if( BlockIndex == 0 ) {
		// contiguous counter addressing
		BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
		CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
		if( BlockIndex >= deviceDesc->Counters ) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}	
	} else {
		if(
			BlockIndex >= deviceDesc->Counters
			|| CounterIndex >= COUNTERS_PER_BLOCK
		) {
			AIOUSB_UnLock();
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}	
	}	

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int READ_BYTES = 3;
		unsigned char readData[ READ_BYTES ];
		const unsigned short controlValue
			= ( ( unsigned short ) CounterIndex << 8 )
			| ( unsigned short ) BlockIndex;
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_READ_FROM_DEVICE, AUR_CTR_READ
			, controlValue, 0, readData, READ_BYTES, timeout );
		if( bytesTransferred == READ_BYTES ) {
			// TODO: verify endian mode; original code had it reversed
			*pReadValue = *( unsigned short * ) readData;
			*pStatus = readData[ 2 ];
		} else
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CTR_8254ReadStatus()



unsigned long CTR_StartOutputFreq(
	unsigned long DeviceIndex
	, unsigned long BlockIndex
	, double *pHz
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	const DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	if(
		BlockIndex >= deviceDesc->Counters
		|| pHz == NULL
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	

    if( *pHz <= 0 ) {
    	/*
    	 * turn off counters
    	 */
		AIOUSB_UnLock();						// unlock while communicating with device
		result = CTR_8254Mode( DeviceIndex, BlockIndex, 1, 2 );
		if( result != AIOUSB_SUCCESS )
			return result;
		result = CTR_8254Mode( DeviceIndex, BlockIndex, 2, 3 );
		if( result != AIOUSB_SUCCESS )
			return result;
		*pHz = 0;								// actual clock speed
	} else {
		const long rootClock = deviceDesc->RootClock;
		AIOUSB_UnLock();						// unlock while communicating with device
		const long frequency = ( long ) *pHz;
		const long MIN_DIVISOR = 2;
		const long MAX_DIVISOR = 65535;
		long bestHighDivisor = MIN_DIVISOR
			, bestLowDivisor = MIN_DIVISOR
			, minFreqError = 0;
		AIOUSB_BOOL minFreqErrorValid = AIOUSB_FALSE;
		const long divisor = ( long ) round( ( double ) rootClock / ( double ) frequency );
#if defined( DEBUG_START_CLOCK )
		printf(
			"Calculating divisors (total divisor = %ld)\n"
			"  %8s  %8s  %8s\n"
			, divisor, "High", "Low", "Error"
		);
#endif
		if( divisor > MIN_DIVISOR * MIN_DIVISOR ) {
			long lowDivisor;
			for( lowDivisor = ( long ) sqrt( divisor ); lowDivisor >= MIN_DIVISOR; lowDivisor-- ) {
				long highDivisor = divisor / lowDivisor;
				const long freqError = labs( frequency - rootClock / ( highDivisor * lowDivisor ) );
#if defined( DEBUG_START_CLOCK )
				printf( "  %8ld  %8ld  %8ld\n", highDivisor, lowDivisor, freqError );
#endif
				if( highDivisor > MAX_DIVISOR ) {
					// this divisor would exceed the maximum; use best divisor calculated thus far
					break;						// from for()
				} else if( freqError == 0 ) {
					// these divisors have no error; no need to continue searching for divisors
					minFreqErrorValid = AIOUSB_TRUE;
					minFreqError = freqError;
					bestHighDivisor = highDivisor;
					bestLowDivisor = lowDivisor;
					break;						// from for()
				} else if(
					! minFreqErrorValid
					|| freqError < minFreqError
				) {
					minFreqErrorValid = AIOUSB_TRUE;
					minFreqError = freqError;
					bestHighDivisor = highDivisor;
					bestLowDivisor = lowDivisor;
				}	
			}	
		}	
#if defined( DEBUG_START_CLOCK )
		printf( "  %8ld  %8ld  %8ld (final)\n", bestHighDivisor, bestLowDivisor, minFreqError );
#endif
		result = CTR_8254ModeLoad( DeviceIndex, BlockIndex, 1, 2, ( unsigned short ) bestHighDivisor );
		if( result != AIOUSB_SUCCESS )
			return result;
		result = CTR_8254ModeLoad( DeviceIndex, BlockIndex, 2, 3, ( unsigned short ) bestLowDivisor );
		if( result != AIOUSB_SUCCESS )
			return result;
		*pHz = rootClock / ( bestHighDivisor * bestLowDivisor );	// actual clock speed
	}	

	return result;
}	// CTR_StartOutputFreq()



unsigned long CTR_8254SelectGate(
	unsigned long DeviceIndex
	, unsigned long GateIndex
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if(
		deviceDesc->Counters == 0
		|| deviceDesc->bGateSelectable == AIOUSB_FALSE
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	if( GateIndex >= deviceDesc->Counters * COUNTERS_PER_BLOCK ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_CTR_SELGATE
			, GateIndex, 0, 0, 0 /* wLength */, timeout );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CTR_8254SelectGate()



unsigned long CTR_8254ReadLatched(
	unsigned long DeviceIndex, 
        unsigned short *pData
) {
	if( pData == NULL )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Counters == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const int READ_BYTES = deviceDesc->Counters * COUNTERS_PER_BLOCK * sizeof( unsigned short ) + 1 /* for "old data" flag */;
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_READ_FROM_DEVICE, AUR_CTR_READLATCHED
			, 0, 0, ( unsigned char * ) pData, READ_BYTES, timeout );
		if( bytesTransferred != READ_BYTES )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	

	return result;
}	// CTR_8254ReadLatched()



#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
