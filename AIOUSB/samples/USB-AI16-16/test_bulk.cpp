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
#include "TestCaseSetup.h"
using namespace AIOUSB;


// extern int CURRENT_DEBUG_LEVEL;


int main( int argc, char **argv ) {
  // printf("Sample test for Checking the Calibration on the board: %s, %s", AIOUSB_GetVersion(), AIOUSB_GetVersionDate());
  unsigned long result = AIOUSB_Init();
  int block_size;
  int over_sample;
  double clock_speed;
  // const char *entries[] = {"BLOCK_SIZE","OVER_SAMPLE","CLOCK_SPEED"};
  int CURRENT_DEBUG_LEVEL = 2;
  
  block_size  = TestCaseSetup::envGetInteger("BLOCK_SIZE");
  over_sample = TestCaseSetup::envGetInteger("OVER_SAMPLE");
  clock_speed = TestCaseSetup::envGetDouble("CLOCK_SPEED");

  if( result == AIOUSB_SUCCESS ) {
          
    unsigned long deviceMask = GetDevices();
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
        std::cout << "Running something" << std::endl;
        // tcs.doBulkAcquire();
        tcs.doBulkAcquire( block_size, over_sample, clock_speed );
        // unsigned char CPUCSByte = 0x01;
        // unsigned long numBytes = 1;
        // tcs.doGenericVendorWrite(0xA0, 0xE600, 0 , &numBytes , &CPUCSByte );
        // CPUCSByte = 0x00;
        // tcs.doGenericVendorWrite(0xA0, 0xE600, 0 , &numBytes , &CPUCSByte );
        std::cout << block_size << "," << over_sample << "," << clock_speed << "," << "Passed" << std::endl;
      } catch ( Error &e  ) {
        std::cout << block_size << "," << over_sample << "," << clock_speed << "," << "Failed" << std::endl;
      }
    }
  }
}

