/*
 * sample program to write out the calibration table and then 
 * reload it again, verify that the data is in fact reversed
 * 
 *
 * 
 */

#include <aiousb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <string.h>
#include "TestCaseSetup.h"


using namespace AIOUSB;

#ifdef __MACH__
#include <sys/time.h>
//clock_gettime is not implemented on OSX
#define CLOCK_REALTIME 0 
#define CLOCK_MONOTONIC 0 

int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif


extern int CURRENT_DEBUG_LEVEL;

#define DEFAULT_VALUE( x, y ) ( x ? x : y ) 
char *get_time( void );



int main( int argc, char *argv[] ) {
  // printf("Sample test for Checking the Calibration on the board: %s, %s", AIOUSB_GetVersion(), AIOUSB_GetVersionDate());
  // CURRENT_DEBUG_LEVEL = VERBOSE_LOGGING;
  CURRENT_DEBUG_LEVEL = DEFAULT_VALUE( TestCaseSetup::envGetInteger("DEBUG_LEVEL"), VERBOSE_LOGGING  );
  unsigned long result = AIOUSB_Init();
  int block_size;
  int over_sample;
  double clock_speed;
  char *start = get_time();
  // const char *entries[] = {"BLOCK_SIZE","OVER_SAMPLE","CLOCK_SPEED"};
  // int CURRENT_DEBUG_LEVEL = 2;
  
  block_size  = TestCaseSetup::envGetInteger("BLOCK_SIZE");
  over_sample = TestCaseSetup::envGetInteger("OVER_SAMPLE");
  clock_speed = TestCaseSetup::envGetDouble("CLOCK_SPEED");

  if( result == AIOUSB_SUCCESS ) {
          
    unsigned long deviceMask = GetDevices();
    char out_file_name[] = "test_bulk_output.csv";
    if( deviceMask != 0 ) {
      // at least one ACCES device detected, but we want one of a specific type
      TestCaseSetup tcs;
      try { 
        tcs.findDevice();
        tcs.doPreSetup();
        tcs.doBulkConfigBlock();
        tcs.doSetAutoCalibration();
        tcs.doVerifyGroundCalibration();
        tcs.doVerifyReferenceCalibration();
        start = get_time();
        tcs.doBulkAcquire( block_size, over_sample, clock_speed );
        tcs.writeBuffer( out_file_name );
        // unsigned char CPUCSByte = 0x01;
        // unsigned long numBytes = 1;
        // tcs.doGenericVendorWrite(0xA0, 0xE600, 0 , &numBytes , &CPUCSByte );
        // CPUCSByte = 0x00;
        // tcs.doGenericVendorWrite(0xA0, 0xE600, 0 , &numBytes , &CPUCSByte );

        unsigned short *tmp = tcs.doGetBuffer();
        tcs.doCleanupAfterBulk();

        std::cout << start << "," << block_size << "," << over_sample << "," << clock_speed << "," << "Passed,"; 
        std::cout << get_time() << std::endl;
      } catch ( Error &e  ) {
        std::cout << start << "," << block_size << "," << over_sample << "," << clock_speed << "," << "Failed,";
        std::cout << get_time() << std::endl;
        _exit(-1);
      }
    }
  }
}

char *get_time () 
{

  struct timespec tv;
  struct tm* ptm; 
  char time_string[7]; 
  long milliseconds; 
  static char buf[47];


  /* Obtain the time of day, and convert it to a tm struct. */ 
  //gettimeofday (&tv, NULL); 
  clock_gettime( CLOCK_REALTIME, &tv );
  ptm = localtime (&tv.tv_sec); 
  /* Format the date and time, down to a single second. */ 
  strftime(buf, 40, "%Y-%m-%d %H:%M:%S", ptm); 
  /* Compute milliseconds from microseconds. */ 
  milliseconds = tv.tv_nsec / 1000; 
  /* Print the formatted time, in seconds, followed by a decimal point 
     and the milliseconds. */ 
  sprintf(time_string,".%.06ld", milliseconds );
  // snprintf(&buf[0],strlen(buf) + 6, "%s.%06ld", buf, milliseconds);
  strncat( buf, time_string, 47 );
  return(buf);
} 
