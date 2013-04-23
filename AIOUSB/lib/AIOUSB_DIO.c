/*
 * $RCSfile: AIOUSB_DIO.c,v $
 * $Revision: 1.8 $
 * $Date: 2009/11/11 19:33:26 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */

#include "AIOUSB_Core.h"
#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include <string.h>


#ifdef __cplusplus
namespace AIOUSB {
#endif


static unsigned short OctaveDacFromFreq( double *Hz ) {
	assert( Hz != 0 );
	unsigned short octaveOffset = 0;
	if( *Hz > 0 ) {
		if( *Hz > 40000000.0 )
			*Hz = 40000000.0;
		int offset
			, octave = ( int ) floor( 3.322 * log10( *Hz / 1039 ) );
		if( octave < 0 ) {
			octave = offset = 0;
		} else {
			offset = ( int ) round( 2048.0 - ( ldexp( 2078, 10 + octave ) / *Hz ) );
			octaveOffset = ( ( unsigned short ) octave << 12 ) | ( ( unsigned short ) offset << 2 );
			octaveOffset = htons( octaveOffset );	// oscillator wants the value in big-endian format
		}	// if( octave ...
		*Hz = ( 2078 << octave ) / ( 2.0 - offset / 1024.0 );
	}	// if( *Hz ...
	return octaveOffset;
}	// OctaveDacFromFreq()


unsigned long DIO_Configure(
	unsigned long DeviceIndex, 
        unsigned char bTristate, 
        void *pOutMask, 
        void *pData
) {
	if(
		pOutMask == NULL
		|| pData == NULL
		|| (
			bTristate != AIOUSB_FALSE
			&& bTristate != AIOUSB_TRUE
		)
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	if( deviceDesc->LastDIOData != 0 ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		memcpy( deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes );
		libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
		if( deviceHandle != NULL ) {
			const int maskBytes = ( deviceDesc->DIOBytes + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
			const int bufferSize = deviceDesc->DIOBytes + 2 * maskBytes;
			unsigned char *const configBuffer = ( unsigned char * ) malloc( bufferSize );
			assert( configBuffer != 0 );
			if( configBuffer != 0 ) {
				unsigned char *dest = configBuffer;
				memcpy( dest, pData, deviceDesc->DIOBytes );
				dest += deviceDesc->DIOBytes;
				memcpy( dest, pOutMask, maskBytes );
				dest += maskBytes;
				memset( dest, 0, maskBytes );
				const unsigned timeout = deviceDesc->commTimeout;
				AIOUSB_UnLock();				// unlock while communicating with device
				const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_CONFIG
					, bTristate, 0, configBuffer, bufferSize, timeout );
				if( bytesTransferred != bufferSize )
					result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
				free( configBuffer );
			} else {
				result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
				AIOUSB_UnLock();
			}
		} else {
			result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
			AIOUSB_UnLock();
		}
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long DIO_ConfigureEx(
	unsigned long DeviceIndex, 
        void *pOutMask, 
        void *pData, 
        void *pTristateMask
) {
	if(
		pOutMask == NULL
		|| pData == NULL
		|| pTristateMask == NULL
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if(
		deviceDesc->DIOBytes == 0
		|| deviceDesc->Tristates == 0
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}

	if( deviceDesc->LastDIOData != 0 ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		memcpy( deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes );
		libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
		if( deviceHandle != NULL ) {
			const int maskBytes = ( deviceDesc->DIOBytes + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
			const int tristateBytes = ( deviceDesc->Tristates + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
			const int bufferSize = deviceDesc->DIOBytes + maskBytes + tristateBytes;
			unsigned char *const configBuffer = ( unsigned char * ) malloc( bufferSize );
			assert( configBuffer != 0 );
			if( configBuffer != 0 ) {
				unsigned char *dest = configBuffer;
				memcpy( dest, pData, deviceDesc->DIOBytes );
				dest += deviceDesc->DIOBytes;
				memcpy( dest, pOutMask, maskBytes );
				dest += maskBytes;
				memcpy( dest, pTristateMask, tristateBytes );
				const int dioBytes = deviceDesc->DIOBytes;
				const unsigned timeout = deviceDesc->commTimeout;
				AIOUSB_UnLock();				// unlock while communicating with device
				const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_CONFIG
					, 0, dioBytes, configBuffer, bufferSize, timeout );
				if( bytesTransferred != bufferSize )
					result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
				free( configBuffer );
			} else {
				result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
				AIOUSB_UnLock();
			}
		} else {
			result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
			AIOUSB_UnLock();
		}
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long DIO_ConfigurationQuery(
	unsigned long DeviceIndex, 
        void *pOutMask, 
        void *pTristateMask
) {
	if(
		pOutMask == NULL
		|| pTristateMask == NULL
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->Tristates == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->Tristates ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		const int maskBytes = ( deviceDesc->DIOBytes + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
		const int tristateBytes = ( deviceDesc->Tristates + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;
		const int bufferSize = maskBytes + tristateBytes;
		unsigned char *const configBuffer = ( unsigned char * ) malloc( bufferSize );
		assert( configBuffer != 0 );
		if( configBuffer != 0 ) {
			const int dioBytes = deviceDesc->DIOBytes;
			const unsigned timeout = deviceDesc->commTimeout;
			AIOUSB_UnLock();					// unlock while communicating with device
			const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_READ_FROM_DEVICE, AUR_DIO_CONFIG_QUERY
				, 0, dioBytes, configBuffer, bufferSize, timeout );
			if( bytesTransferred == bufferSize ) {
				memcpy( pOutMask, configBuffer, maskBytes );
				memcpy( pTristateMask, configBuffer + maskBytes, tristateBytes );
			} else
				result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
			free( configBuffer );
		} else {
			result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
			AIOUSB_UnLock();
		}	// if( configBuffer ...
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}	// DIO_ConfigurationQuery()



unsigned long DIO_WriteAll(
	unsigned long DeviceIndex, 
        void *pData
) {
	if( pData == NULL )
		return AIOUSB_ERROR_INVALID_PARAMETER;
        int tmp = AIOUSB_Lock();
        if( !tmp ) {
          AIOUSB_UnLock();
          AIOUSB_Lock();
        }
		/* return AIOUSB_ERROR_INVALID_MUTEX; */

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}

	if( deviceDesc->LastDIOData != 0 ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		memcpy( deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes );
		libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
		if( deviceHandle != NULL ) {
			const int dioBytes = deviceDesc->DIOBytes;
			const unsigned timeout = deviceDesc->commTimeout;
			AIOUSB_UnLock();					// unlock while communicating with device
			const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_WRITE
				, 0, 0, ( unsigned char * ) pData, dioBytes, timeout );
			if( bytesTransferred != dioBytes )
				result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
		} else {
			result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
			AIOUSB_UnLock();
		}
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}

	return result;
}



unsigned long DIO_Write8(
	unsigned long DeviceIndex, 
        unsigned long ByteIndex, 
        unsigned char Data
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	if( ByteIndex >= deviceDesc->DIOBytes ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( ByteIndex ...

	if( deviceDesc->LastDIOData != 0 ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		deviceDesc->LastDIOData[ ByteIndex ] = Data;
		libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
		if( deviceHandle != NULL ) {
			const int dioBytes = deviceDesc->DIOBytes;
			unsigned char *const dataBuffer = ( unsigned char * ) malloc( dioBytes );
			assert( dataBuffer != 0 );
			if( dataBuffer != 0 ) {
				memcpy( dataBuffer, deviceDesc->LastDIOData, dioBytes );
				const unsigned timeout = deviceDesc->commTimeout;
				AIOUSB_UnLock();				// unlock while communicating with device
				const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_WRITE
					, 0, 0, dataBuffer, dioBytes, timeout );
				if( bytesTransferred != dioBytes )
					result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
				free( dataBuffer );
			} else {
				result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
				AIOUSB_UnLock();
			}	// if( dataBuffer ...
		} else {
			result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
			AIOUSB_UnLock();
		}	// if( deviceHandle ...
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}	// if( deviceDesc->LastDIOData ...

	return result;
}	// DIO_Write8()



unsigned long DIO_Write1(
	unsigned long DeviceIndex, 
        unsigned long BitIndex, 
        unsigned char bData
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	const unsigned byteIndex = BitIndex / BITS_PER_BYTE;
	if(
		(
			bData != AIOUSB_FALSE
			&& bData != AIOUSB_TRUE
		)
		|| byteIndex >= deviceDesc->DIOBytes
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( ( bData ...

	if( deviceDesc->LastDIOData != 0 ) {
		unsigned char value = deviceDesc->LastDIOData[ byteIndex ];
		const unsigned char bitMask = 1 << ( BitIndex % BITS_PER_BYTE );
		if( bData == AIOUSB_FALSE )
			value &= ~bitMask;
		else
			value |= bitMask;
		AIOUSB_UnLock();						// unlock while communicating with device
		result = DIO_Write8( DeviceIndex, byteIndex, value );
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}	// if( deviceDesc->LastDIOData ...

	return result;
}


unsigned long DIO_ReadAll(
	unsigned long DeviceIndex, 
        void *Buffer
) {
	if( Buffer == NULL )
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		assert( deviceDesc->DIOBytes <= 1000 );	// arbitrary sanity check
		const int dioBytes = deviceDesc->DIOBytes;
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_READ_FROM_DEVICE, AUR_DIO_READ
			, 0, 0, ( unsigned char * ) Buffer, dioBytes, timeout );
		if( bytesTransferred != dioBytes )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}



unsigned long DIO_Read8(
	unsigned long DeviceIndex, 
        unsigned long ByteIndex, 
        unsigned char *pBuffer
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	if(
		pBuffer == NULL
		|| ByteIndex >= deviceDesc->DIOBytes
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( pBuffer ...

	assert( deviceDesc->DIOBytes <= 1000 );		// arbitrary sanity check
	unsigned char *const readBuffer = ( unsigned char * ) malloc( deviceDesc->DIOBytes );
	assert( readBuffer != 0 );
	if( readBuffer != 0 ) {
		AIOUSB_UnLock();						// unlock while communicating with device
		if( ( result = DIO_ReadAll( DeviceIndex, readBuffer ) ) == AIOUSB_SUCCESS )
			*pBuffer = readBuffer[ ByteIndex ];
		free( readBuffer );
	} else {
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		AIOUSB_UnLock();
	}	// if( readBuffer ...

	return result;
}



unsigned long DIO_Read1(
	unsigned long DeviceIndex, 
        unsigned long BitIndex, 
        unsigned char *pBuffer
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->DIOBytes == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->DIOBytes ...

	if(
		pBuffer == NULL
		|| BitIndex >= deviceDesc->DIOBytes * BITS_PER_BYTE
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( pBuffer ...

	AIOUSB_UnLock();							// unlock while communicating with device
	unsigned char readBuffer;
	if( ( result = DIO_Read8( DeviceIndex, BitIndex / BITS_PER_BYTE, &readBuffer ) ) == AIOUSB_SUCCESS ) {
		const unsigned char bitMask = 1 << ( ( int ) BitIndex % BITS_PER_BYTE );
		if( ( readBuffer & bitMask ) != 0 )
			*pBuffer = AIOUSB_TRUE;
		else
			*pBuffer = AIOUSB_FALSE;
	}	// if( ( result ...

	return result;
}


unsigned long DIO_StreamOpen(
	unsigned long DeviceIndex, 
        unsigned long bIsRead
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->bDIOStream == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bDIOStream ...

	if( deviceDesc->bDIOOpen ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_OPEN_FAILED;
	}	// if( deviceDesc->bDIOOpen )

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle
			, USB_WRITE_TO_DEVICE
			, bIsRead
				? AUR_DIO_STREAM_OPEN_INPUT
				: AUR_DIO_STREAM_OPEN_OUTPUT
			, 0, 0, 0, 0, timeout );
		if( bytesTransferred == 0 ) {
			AIOUSB_Lock();
			deviceDesc->bDIOOpen = AIOUSB_TRUE;
			deviceDesc->bDIORead
				= bIsRead
					? AIOUSB_TRUE
					: AIOUSB_FALSE;
			AIOUSB_UnLock();
		} else
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}



unsigned long DIO_StreamClose(
	unsigned long DeviceIndex
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->bDIOStream == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bDIOStream ...

	if( deviceDesc->bDIOOpen == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_FILE_NOT_FOUND;
	}	// if( deviceDesc->bDIOOpen ...

	deviceDesc->bDIOOpen = AIOUSB_FALSE;

	AIOUSB_UnLock();
	return result;
}



unsigned long DIO_StreamSetClocks(
	unsigned long DeviceIndex, 
        double *ReadClockHz, 
        double *WriteClockHz
) {
	if(
		*ReadClockHz < 0
		|| *WriteClockHz < 0
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->bDIOStream == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bDIOStream ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		/*
		 * fill in data for the vendor request
		 * byte 0 used enable/disable read and write clocks
		 *   bit 0 is write clock
		 *   bit 1 is read  clock
		 *     1 = off/disable
		 *     0 = enable (1000 Khz is default value whenever enabled)
		 * bytes 1-2 = write clock value
		 * bytes 3-4 = read clock value
		 */
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int CONFIG_BLOCK_SIZE = 5;
		unsigned char configBlock[ CONFIG_BLOCK_SIZE ];
		configBlock[ 0 ] = 0x03;				// disable read and write clocks by default
		if( *WriteClockHz > 0 )
			configBlock[ 0 ] &= ~0x01;			// enable write clock
		if( *ReadClockHz > 0 )
			configBlock[ 0 ] &= ~0x02;			// enable read clock
		*( unsigned short * ) &configBlock[ 1 ] = OctaveDacFromFreq( WriteClockHz );
		*( unsigned short * ) &configBlock[ 3 ] = OctaveDacFromFreq( ReadClockHz );
		const int bytesTransferred = libusb_control_transfer( deviceHandle
			, USB_WRITE_TO_DEVICE, AUR_DIO_SETCLOCKS
			, 0, 0, configBlock, CONFIG_BLOCK_SIZE, timeout );
		if( bytesTransferred != CONFIG_BLOCK_SIZE )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}


unsigned long DIO_StreamFrame(
	unsigned long DeviceIndex, 
        unsigned long FramePoints, 
        unsigned short *pFrameData, 
        unsigned long *BytesTransferred
) {
	if(
		FramePoints == 0
		|| pFrameData == NULL
		|| BytesTransferred == NULL
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->bDIOStream == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bDIOStream ...

	if( deviceDesc->bDIOOpen == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_FILE_NOT_FOUND;
	}	// if( deviceDesc->bDIOOpen ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		const unsigned char endpoint
			= deviceDesc->bDIORead
				? ( LIBUSB_ENDPOINT_IN | USB_BULK_READ_ENDPOINT )
				: ( LIBUSB_ENDPOINT_OUT | USB_BULK_WRITE_ENDPOINT );
		const int streamingBlockSize = ( int ) deviceDesc->StreamingBlockSize * sizeof( unsigned short );
		AIOUSB_UnLock();						// unlock while communicating with device

		/*
		 * convert parameter types to those that libusb likes
		 */
		unsigned char *data = ( unsigned char * ) pFrameData;
		int remaining = ( int ) FramePoints * sizeof( unsigned short );
		int total = 0;
		while( remaining > 0 ) {
			int bytes;
			const int libusbResult = AIOUSB_BulkTransfer( deviceHandle, endpoint, data
				, ( remaining < streamingBlockSize )
					? remaining
					: streamingBlockSize
				, &bytes, timeout );
			if( libusbResult == LIBUSB_SUCCESS ) {
				if( bytes > 0 ) {
					total += bytes;
					data += bytes;
					remaining -= bytes;
				}	// if( bytes ...
			} else {
				result = LIBUSB_RESULT_TO_AIOUSB_RESULT( libusbResult );
				break;							// from while()
			}	// if( libusbResult ...
		}	// while( remaining ...
		if( result == AIOUSB_SUCCESS )
			*BytesTransferred = total;
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}


#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
