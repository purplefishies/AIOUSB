/**
 * @file   AIOUSB_Core.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief 
 *
 * @note build instructions
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aiousb.h"
#include "AIOUSB_Core.h"



int main( int argc, char *argv[] ) {
    char *VERSION = "1.6, 25 December 2013";
    printf( "USB-AI16-16A sample program version 1.3, 22 December 2009\n"
            "  AIOUSB library version %s, %s\n"
            "  This program demonstrates controlling a USB-AI16-16A device on\n"
            "  the USB bus. For simplicity, it uses the first such device found\n"
            "  on the bus.\n", 
            AIOUSB_GetVersion(), 
            AIOUSB_GetVersionDate()
            );
    unsigned long result = AIOUSB_Init();
    unsigned long deviceMask = GetDevices();
    
    AIOUSB_ListDevices();

    const int MAX_NAME_SIZE = 20;
    char name[ MAX_NAME_SIZE + 2 ];
    unsigned long productID, nameSize, numDIOBytes, numCounters;
    unsigned long deviceIndex = 0;
    AIOUSB_BOOL deviceFound = AIOUSB_FALSE;

    result = AIOUSB_Reset( deviceIndex );
    printf("Result was %d\n", (int)result );
    result = AIOUSB_SetCommTimeout( deviceIndex, 1000 );
}

/* end of file */
