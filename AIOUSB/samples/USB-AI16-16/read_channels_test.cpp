/*
 * sample program to write out the calibration table and then 
 * reload it again, verify that the data is in fact reversed
 */

#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <exception>
#include <iostream>       
#include <unistd.h>
#include "TestCaseSetup.h"
#include <string.h>

using namespace AIOUSB;

struct options {
  int maxcount;
  int use_maxcount;
  int number_channels;
  int debug_level;
};

struct options get_options(struct options *opts, int argc, char **argv );


int main( int argc, char **argv ) {

  unsigned long result = AIOUSB_Init();
  struct options opts = { 0, 0 , 16 , INFO_LEVEL };
  opts = get_options(&opts,argc, argv);
  CURRENT_DEBUG_LEVEL = opts.debug_level;

  if( result == AIOUSB_SUCCESS ) {
          
    unsigned long deviceMask = GetDevices();
    if( deviceMask != 0 ) {
      // at least one ACCES device detected, but we want one of a specific type
      // AIOUSB_ListDevices();
        AIOUSB_ListDevices();
      TestCaseSetup tcs( 0, opts.number_channels );
      try { 
        tcs.findDevice( [](AIOUSBDevice *dev)->AIOUSB_BOOL {
                if ( (dev->ProductID >= USB_AI16_16A && 
                      dev->ProductID <= USB_AI12_128E ) || 
                     (dev->ProductID >= USB_AIO16_16A  &&
                      dev->ProductID <= USB_AIO12_128E
                      )
                     ) 
                    return AIOUSB_TRUE;
                else
                    return AIOUSB_FALSE;
            }
            );

        tcs.doPreSetup();
        tcs.doBulkConfigBlock();
        tcs.doSetAutoCalibration();
        tcs.doVerifyGroundCalibration();
        tcs.doVerifyReferenceCalibration();
        tcs.doPreReadImmediateVoltages();

        if( opts.use_maxcount ) 
          tcs.maxcounts = opts.maxcount;

        tcs.doCSVReadVoltages();
        usleep(0.1);

      } catch ( Error &e  ) {
        std::cout << "Errors" << e.what() << std::endl;
      }
    }
  }
}

//
// Gets the options 
//
struct options get_options( struct options *opts, int argc, char **argv )
{
  int opt;

  struct options retoptions;
  if( opts ) {
    memcpy(&retoptions, opts, sizeof( struct options ));
  }

  while ((opt = getopt(argc, argv, "C:c:tD:")) != -1) {
    switch (opt) {
    case 'C':
      retoptions.number_channels = atoi(optarg);
      break;
    case 'c':
      retoptions.maxcount     = atoi(optarg);
      retoptions.use_maxcount = 1;
      break;
    case 't':

      break;
    case 'D':
      retoptions.debug_level = atoi(optarg);  
      break;

    default: /* '?' */
      fprintf(stderr, "Usage: %s [-c maxcounts]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  return retoptions;
}


