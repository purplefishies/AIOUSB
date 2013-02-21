/*
 * $RCSfile: AIOUSB_GetResultCodeAsString.c,v $
 * $Revision: 1.7 $
 * $Date: 2009/12/07 17:49:52 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */


// {{{ includes
#include "AIOUSB_Core.h"
#include <assert.h>
#include <stdlib.h>
// }}}

// {{{ C++ support
#ifdef __cplusplus
namespace AIOUSB {
#endif
// }}}

// {{{ resultCodeTable[]
#ifdef __cplusplus
const int RESULT_TEXT_SIZE = 40;
#else
#define RESULT_TEXT_SIZE 40
#endif
static const struct ResultCodeName {
	unsigned int result;
	char text[ RESULT_TEXT_SIZE + 2 ];
} resultCodeTable[] = {
	// AIOUSB result codes
	  { AIOUSB_SUCCESS,							"AIOUSB_SUCCESS" }
	, { AIOUSB_ERROR_DEVICE_NOT_CONNECTED,		"AIOUSB_ERROR_DEVICE_NOT_CONNECTED" }
	, { AIOUSB_ERROR_DUP_NAME,					"AIOUSB_ERROR_DUP_NAME" }
	, { AIOUSB_ERROR_FILE_NOT_FOUND,			"AIOUSB_ERROR_FILE_NOT_FOUND" }
	, { AIOUSB_ERROR_INVALID_DATA,				"AIOUSB_ERROR_INVALID_DATA" }
	, { AIOUSB_ERROR_INVALID_INDEX,				"AIOUSB_ERROR_INVALID_INDEX" }
	, { AIOUSB_ERROR_INVALID_MUTEX,				"AIOUSB_ERROR_INVALID_MUTEX" }
	, { AIOUSB_ERROR_INVALID_PARAMETER,			"AIOUSB_ERROR_INVALID_PARAMETER" }
	, { AIOUSB_ERROR_INVALID_THREAD,			"AIOUSB_ERROR_INVALID_THREAD" }
	, { AIOUSB_ERROR_NOT_ENOUGH_MEMORY,			"AIOUSB_ERROR_NOT_ENOUGH_MEMORY" }
	, { AIOUSB_ERROR_NOT_SUPPORTED,				"AIOUSB_ERROR_NOT_SUPPORTED" }
	, { AIOUSB_ERROR_OPEN_FAILED,				"AIOUSB_ERROR_OPEN_FAILED" }

	// this is a special case that should not occur, but ...
	, { AIOUSB_ERROR_LIBUSB,					"AIOUSB_ERROR_LIBUSB" }

	// libusb result codes
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_IO ),				"LIBUSB_ERROR_IO" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_INVALID_PARAM ),	"LIBUSB_ERROR_INVALID_PARAM" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_ACCESS ),			"LIBUSB_ERROR_ACCESS" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_NO_DEVICE ),		"LIBUSB_ERROR_NO_DEVICE" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_NOT_FOUND ),		"LIBUSB_ERROR_NOT_FOUND" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_BUSY ),			"LIBUSB_ERROR_BUSY" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_TIMEOUT ),			"LIBUSB_ERROR_TIMEOUT" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_OVERFLOW ),		"LIBUSB_ERROR_OVERFLOW" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_PIPE ),			"LIBUSB_ERROR_PIPE" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_INTERRUPTED ),		"LIBUSB_ERROR_INTERRUPTED" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_NO_MEM ),			"LIBUSB_ERROR_NO_MEM" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_NOT_SUPPORTED ),	"LIBUSB_ERROR_NOT_SUPPORTED" }
	, { LIBUSB_RESULT_TO_AIOUSB_RESULT( LIBUSB_ERROR_OTHER ),			"LIBUSB_ERROR_OTHER" }
};	// resultCodeTable[]
#ifdef __cplusplus
const int NUM_RESULT_CODES = sizeof( resultCodeTable ) / sizeof( resultCodeTable[ 0 ] );
#else
#define NUM_RESULT_CODES ( sizeof( resultCodeTable ) / sizeof( resultCodeTable[ 0 ] ) )
#endif
// }}}

static int CompareResultCodes( const void *p1, const void *p2 ) {
	assert( p1 != 0
		&& ( *( struct ResultCodeName ** ) p1 ) != 0
		&& p2 != 0
		&& ( *( struct ResultCodeName ** ) p2 ) != 0 );
	const unsigned int result1 = ( *( struct ResultCodeName ** ) p1 )->result
		, result2 = ( *( struct ResultCodeName ** ) p2 )->result;
	if( result1 < result2 )
		return -1;
	else if( result1 > result2 )
		return 1;
	else
		return 0;
}	// CompareResultCodes()


const char *AIOUSB_GetResultCodeAsString( unsigned long result ) {
	const char *resultText = "UNKNOWN";
	if( AIOUSB_Lock() ) {
		/*
		 * resultCodeIndex[] represents an index into resultCodeTable[], sorted by result code;
		 * specifically, it contains pointers into resultCodeTable[]; to get the actual result
		 * code, the pointer in resultCodeIndex[] must be dereferenced
		 */
		static struct ResultCodeName const *resultCodeIndex[ NUM_RESULT_CODES ];	// index of result codes in resultCodeTable[]
		const unsigned long INIT_PATTERN = 0x100c48b9ul;							// random pattern
		static unsigned long resultCodeIndexCreated = 0;							// == INIT_PATTERN if index has been created
		if( resultCodeIndexCreated != INIT_PATTERN ) {
			/*
			 * build index of result codes
			 */
			int index;
			for( index = 0; index < NUM_RESULT_CODES; index++ )
				resultCodeIndex[ index ] = &resultCodeTable[ index ];
			qsort( resultCodeIndex, NUM_RESULT_CODES, sizeof( struct ResultCodeName * ), CompareResultCodes );
			resultCodeIndexCreated = INIT_PATTERN;
		}	// if( resultCodeIndexCreated ...

		struct ResultCodeName key;				// key.name not used
		key.result = result;
		const struct ResultCodeName *const pKey = &key;
		const struct ResultCodeName **resultCode
			= ( const struct ResultCodeName ** ) bsearch( &pKey, resultCodeIndex, NUM_RESULT_CODES, sizeof( struct ResultCodeName * ), CompareResultCodes );
		if( resultCode != 0 )
			resultText = ( *resultCode )->text;
		AIOUSB_UnLock();
	}	// if( AIOUSB_Lock() )
	return resultText;
}	// AIOUSB_GetResultCodeAsString()

// {{{ C++ support
#ifdef __cplusplus
}	// namespace AIOUSB
#endif
// }}}


/* end of file */
