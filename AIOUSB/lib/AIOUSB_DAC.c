/*
 * $RCSfile: AIOUSB_DAC.c,v $
 * $Revision: 1.9 $
 * $Date: 2009/11/11 19:33:26 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */


// {{{ includes
#include "AIOUSB_Core.h"
#include <assert.h>
#include <math.h>
#include <string.h>
// }}}

// {{{ C++ support
#ifdef __cplusplus
namespace AIOUSB {
#endif
// }}}

// {{{ DACDirect()
unsigned long DACDirect(
	unsigned long DeviceIndex
	, unsigned short Channel
	, unsigned short Value
) {
	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->ImmDACs == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->ImmDACs ...

	if(
		deviceDesc->bDACStream
		&& (
			deviceDesc->bDACOpen
			|| deviceDesc->bDACClosing
		)
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_OPEN_FAILED;
	}	// if( deviceDesc->bDACStream ...

	if( Channel >= deviceDesc->ImmDACs ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( Channel ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DAC_IMMEDIATE
			, Value, Channel, 0, 0 /* wLength */, timeout );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}	// DACDirect()
// }}}

// {{{ DACMultiDirect()
// {{{ notes
/*
 * pDACData is an array of DACDataCount channel/count 16-bit word pairs:
 *   +----------------+
 *   |    channel     | word 0
 *   |----------------|
 *   |     count      | word 1
 *   |----------------|
 *          ...
 *   |----------------|
 *   |    channel     |
 *   |----------------|
 *   |     count      | word ( DACDataCount * 2 ) - 1
 *   +----------------+
 *
 * this array has to be converted to a different format when passed to the board:
 *        Block 0
 *   +----------------+
 *   |   chan mask    | byte 0
 *   |----------------|
 *   |  chan 0 count  | bytes 1-2
 *   |----------------|
 *          ...
 *   |----------------|
 *   |  chan 6 count  | bytes 13-14
 *   |----------------|
 *   |  chan 7 count  | bytes 15-16
 *   +----------------+
 *        Block 1
 *   +----------------+
 *   |   chan mask    | byte 17
 *   |----------------|
 *   |  chan 0 count  | bytes 18-19
 *   |----------------|
 *          ...
 *   |----------------|
 *   |  chan 6 count  | bytes 30-31
 *   |----------------|
 *   |  chan 7 count  | bytes 32-33
 *   +----------------+
 *          ...
 *        Block n
 *   +----------------+
 *   |   chan mask    |
 *   |----------------|
 *   |  chan 0 count  |
 *   |----------------|
 *          ...
 *   |----------------|
 *   |  chan 6 count  |
 *   |----------------|
 *   |  chan 7 count  | bytes ( ( 17 * n ) - 2 ) - ( ( 17 * n ) - 1 )
 *   +----------------+
 *
 * the channel mask (the first byte of each block) has a bit set to one for each
 * channel whose output is to be set; the count values are zero for channels that
 * aren't to be set; for example, a mask of 0x01 would write to only channel 0 on
 * a given block; a mask of 0x80 would write to only channel 7
 *
 * since the DAC configuration blocks are contiguous, the byte offset to a channel's
 * count within the buffer containing all the configuration blocks can be calculated as:
 *   offset = ( channel * sizeof( unsigned short ) ) + ( channel / 8 ) + 1;
 * although this calculation is correct, it's difficult to follow, so the code below
 * uses a slightly less efficient calculation that's easier to understand
 *
 * when sending the DAC configuration blocks to the device we have to send all the
 * blocks from block 0 up to the block containing the highest channel number being set
 */
// }}}
unsigned long DACMultiDirect(
	unsigned long DeviceIndex
	, unsigned short *pDACData
	, unsigned long DACDataCount
) {
	if(
		pDACData == NULL
		|| DACDataCount > 10000					// arbitrary limit to prevent code from blowing up
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	if( DACDataCount == 0 )
		return AIOUSB_SUCCESS;					// NOP

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->ImmDACs == 0 ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->ImmDACs ...

	if(
		deviceDesc->bDACStream
		&& (
			deviceDesc->bDACOpen
			|| deviceDesc->bDACClosing
		)
	) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_OPEN_FAILED;
	}	// if( deviceDesc->bDACStream ...

	/*
	 * determine highest channel number addressed in pDACData; no checking is
	 * performed to ensure that the same channel is not set more than once
	 */
	int highestChannel = 0
		, channel
		, index;
	for( index = 0; index < ( int ) DACDataCount; index++ ) {
		channel = pDACData[ index * 2 ];		// channel/count pairs
		if( channel > highestChannel )
			highestChannel = channel;
	}	// for( index ...

	if( highestChannel >= ( int ) deviceDesc->ImmDACs ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_INVALID_PARAMETER;
	}	// if( highestChannel ...

	const unsigned timeout = deviceDesc->commTimeout;
	AIOUSB_UnLock();							// unlock while communicating with device
	const int DACS_PER_BLOCK = 8;
	const int CONFIG_BLOCK_BYTES = 1 /* mask */ + DACS_PER_BLOCK * sizeof( unsigned short ) /* 16-bit counts */;
	const int numConfigBlocks = ( highestChannel / DACS_PER_BLOCK ) + 1;
	const int configBytes = CONFIG_BLOCK_BYTES * numConfigBlocks;
	unsigned char *const configBuffer = ( unsigned char * ) malloc( configBytes );
	assert( configBuffer != 0 );
	if( configBuffer != 0 ) {
		/*
		 * sparsely populate DAC configuration blocks
		 */
		memset( configBuffer, 0, configBytes );	// zero out channel masks and count values for unused channels
		for( index = 0; index < ( int ) DACDataCount; index++ ) {
			channel = pDACData[ index * 2 ];	// channel/count pairs
			const int maskOffset = ( channel / DACS_PER_BLOCK ) * CONFIG_BLOCK_BYTES;
			const int countOffset
				= maskOffset					// first byte of block
				+ 1								// skip over mask byte
				+ ( channel % DACS_PER_BLOCK ) * sizeof( unsigned short );	// word within block
			configBuffer[ maskOffset ] |= ( 1u << ( channel % DACS_PER_BLOCK ) );
			*( unsigned short * ) &configBuffer[ countOffset ] = pDACData[ index * 2 + 1 ];
		}	// for( index ...

		libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
		if( deviceHandle != NULL ) {
			const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DAC_IMMEDIATE
				, 0, 0, configBuffer, configBytes, timeout );
			if( bytesTransferred != configBytes )
				result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
		} else
			result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		free( configBuffer );
	} else
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;

	return result;
}	// DACMultiDirect()
// }}}

// {{{ DACSetBoardRange()
unsigned long DACSetBoardRange(
	unsigned long DeviceIndex
	, unsigned long RangeCode
) {
	if(
		RangeCode < DAC_RANGE_MIN
		|| RangeCode > DAC_RANGE_MAX
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
	if( deviceDesc->bDACBoardRange == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bDACBoardRange ...

	libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle( DeviceIndex );
	if( deviceHandle != NULL ) {
		const unsigned timeout = deviceDesc->commTimeout;
		AIOUSB_UnLock();						// unlock while communicating with device
		const int bytesTransferred = libusb_control_transfer( deviceHandle, USB_WRITE_TO_DEVICE, AUR_DAC_RANGE
			, RangeCode, 0, 0, 0 /* wLength */, timeout );
		if( bytesTransferred != 0 )
			result = LIBUSB_RESULT_TO_AIOUSB_RESULT( bytesTransferred );
	} else {
		result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
		AIOUSB_UnLock();
	}	// if( deviceHandle ...

	return result;
}	// DACSetBoardRange()
// }}}

// {{{ DACOutputOpen()
unsigned long DACOutputOpen(
	unsigned long DeviceIndex
	, double *pClockHz
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputOpen()
// }}}

// {{{ DACOutputClose()
unsigned long DACOutputClose(
	unsigned long DeviceIndex
	, unsigned long bWait
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputClose()
// }}}

// {{{ DACOutputCloseNoEnd()
unsigned long DACOutputCloseNoEnd(
	unsigned long DeviceIndex
	, unsigned long bWait
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputCloseNoEnd()
// }}}

// {{{ DACOutputSetCount()
unsigned long DACOutputSetCount(
	unsigned long DeviceIndex
	, unsigned long NewCount
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputSetCount()
// }}}

// {{{ DACOutputFrame()
unsigned long DACOutputFrame(
	unsigned long DeviceIndex
	, unsigned long FramePoints
	, unsigned short *FrameData
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputFrame()
// }}}

// {{{ DACOutputFrameRaw()
unsigned long DACOutputFrameRaw(
	unsigned long DeviceIndex
	, unsigned long FramePoints
	, unsigned short *FrameData
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputFrameRaw()
// }}}

// {{{ DACOutputStart()
unsigned long DACOutputStart(
	unsigned long DeviceIndex
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputStart()
// }}}

// {{{ DACOutputSetInterlock()
unsigned long DACOutputSetInterlock(
	unsigned long DeviceIndex
	, unsigned long bInterlock
) {
	// TODO: this function is not yet implemented
	return AIOUSB_ERROR_NOT_SUPPORTED;
}	// DACOutputSetInterlock()
// }}}

// {{{ C++ support
#ifdef __cplusplus
}	// namespace AIOUSB
#endif
// }}}


/* end of file */
