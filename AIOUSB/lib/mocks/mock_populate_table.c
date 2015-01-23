#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <unistd.h>
#include <sys/stat.h>
#include "AIOTypes.h"
#include "USBDevice.h"
#include "AIOUSB_Core.h"

#include <dlfcn.h>

void get_device_ids( unsigned int **products, int *size );

void PopulateDeviceTable(void) 
{
    static  void *(*populate_table)(void)=NULL;
    if (!populate_table) populate_table = dlsym(RTLD_NEXT,"PopulateDeviceTable");
    
    unsigned int *products = NULL;
    int size = 0;
    get_device_ids( &products, &size );
    PopulateDeviceTableTest( products, size );
}

void get_device_ids( unsigned int **products, int *size ) 
{
    char *tmp;
    char *orig;
    if ( (tmp = getenv("AIOUSB_DEVICES" )) ) { 
        *size = 0;
        tmp = strdup( tmp );
        orig = tmp;
        char delim[] = ",";
        char *pos = NULL;
        char *savepos;
        for ( char *token = strtok_r( tmp, delim, &pos );  token ; token = strtok_r(NULL, delim , &pos) ) {
            if (token == NULL)
                break;
            if( strlen(token) > 3 && strncmp(token,"USB",3 ) == 0 ) {
                unsigned int tmpproduct = ProductNameToID( token );
                if( tmpproduct ) { 
                    *size += 1;
                    *products = (unsigned int *)realloc( *products, (*size)*sizeof(unsigned int)) ;
                    *products[*size-1] = tmpproduct;
                }
            }
        }
    }
}
