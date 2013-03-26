/*
 * $RCSfile: AIOUSB_ListDevices.c,v $
 * $Revision: 1.7 $
 * $Date: 2009/11/15 20:56:05 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */



#include "AIOUSB_Core.h"
#include <stdio.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif


void AIOUSB_ListDevices() {
	AIOUSB_BOOL found = AIOUSB_FALSE;
	if( AIOUSB_Lock() ) {
		if( AIOUSB_IsInit() ) {
			int index;
			for( index = 0; index < MAX_USB_DEVICES; index++ ) {
				if( deviceTable[ index ].device != NULL ) {
					const int MAX_NAME_SIZE = 100;
					char name[ MAX_NAME_SIZE + 1 ];
					unsigned long productID;
					unsigned long nameSize = MAX_NAME_SIZE;
					unsigned long numDIOBytes;
					unsigned long numCounters;
					AIOUSB_UnLock();			// unlock while communicating with device
					const unsigned long result = QueryDeviceInfo( index, &productID, &nameSize, name, &numDIOBytes, &numCounters );
					if( result == AIOUSB_SUCCESS ) {
						name[ nameSize ] = '\0';
						if( ! found ) {
							// print a heading before the first device found
							printf( "ACCES devices found:\n" );
							found = AIOUSB_TRUE;
						}	// if( ! found )
						printf(
							"  Device at index %d:\n"
							"    Product ID: %#lx\n"
							"    Product name: %s\n"
							"    Number of digital I/O bytes: %lu\n"
							"    Number of counters: %lu\n"
							, index
							, productID
							, name
							, numDIOBytes
							, numCounters
						);
					}
					// else, unknown error occurred; a device was detected, but query failed
					AIOUSB_Lock();
				}
			}
		}
		AIOUSB_UnLock();
	}
	if( ! found )
		printf( "No ACCES devices found\n" );
}


#ifdef __cplusplus
}	// namespace AIOUSB
#endif


