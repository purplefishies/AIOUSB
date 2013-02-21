
#include "TestCaseSetup.h"


using namespace AIOUSB;





int CURRENT_DEBUG_LEVEL = 0;

TestCaseSetup::TestCaseSetup() : deviceIndex(0) , deviceFound(false) , CAL_CHANNEL(DEF_CAL_CHANNEL), 
                   MAX_CHANNELS(DEF_MAX_CHANNELS) , NUM_CHANNELS(DEF_NUM_CHANNELS) {
  counts                  = (unsigned short*)malloc( sizeof(unsigned short)*MAX_CHANNELS);
  volts                   = (double *)malloc(sizeof(double)* MAX_CHANNELS);
  gainCodes               = (unsigned char *)malloc(sizeof(unsigned char)*NUM_CHANNELS);
  //CURRENT_DEBUG_LEVEL = FATAL_LEVEL;

  // Turn on the Debug level 
}

// counts = new unsigned short[MAX_CHANNELS];
// volts = new double[MAX_CHANNELS];
// gainCodes = new unsigned char[NUM_CHANNELS];



void TestCaseSetup::doSomething(void) {
  printf("Reached\n");
}

void TestCaseSetup::resetCPU(void) {
  LOG("Need to do something");

}


void TestCaseSetup::findDevice(void) {
  unsigned long deviceMask = GetDevices();
  unsigned long productID, nameSize, numDIOBytes, numCounters;
  unsigned long result;
  char name[ MAX_NAME_SIZE + 2 ];

  while( deviceMask != 0 ) {
    if( ( deviceMask & 1 ) != 0 ) {
      // found a device, but is it the correct type?
      nameSize = MAX_NAME_SIZE;
      result = QueryDeviceInfo( this->deviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
      if( result == AIOUSB_SUCCESS ) {
        if( productID >= USB_AI16_16A && 
            productID <= USB_AI12_128E ) { 
          deviceFound = true;
          break;
        }
      } else {
        throw std::string("Unable to find device");
      }
    }	// if( ( deviceMask ...
    this->deviceIndex++;
    deviceMask >>= 1;
  }	// while( deviceMask ...
}

void TestCaseSetup::doPreSetup(void)
{

  AIOUSB_Reset( deviceIndex );
  AIOUSB_SetCommTimeout( deviceIndex, 1000 );
  AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB_TRUE );
  
  __uint64_t serialNumber;
  int result = GetDeviceSerialNumber( deviceIndex, &serialNumber );
  if( result != AIOUSB_SUCCESS  ) {
    std::stringstream er;
    er << "Error '" << AIOUSB_GetResultCodeAsString( result );
    er << "' getting serial number of device at index " << deviceIndex ;
    throw Error( er.str().c_str() );
  }
}

//
// @desc Uploads a bulk configuration block
//
void TestCaseSetup::doBulkConfigBlock(void)
{
   ADConfigBlock configBlock;
   AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
   AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
   AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL );
   AIOUSB_SetTriggerMode( &configBlock, 0 );
   AIOUSB_SetScanRange( &configBlock, 2, 13 );
   AIOUSB_SetOversample( &configBlock, 0 );
   int result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );
   if( result != AIOUSB_SUCCESS  ) {
     std::stringstream er;
     er << "Error '" << AIOUSB_GetResultCodeAsString( result ) << "' setting A/D configuration";
     throw Error(er.str().c_str());
   }                  
}



// 
// Demonstrate bulk acquire
// 
void TestCaseSetup::doBulkAcquire(void)
{
  int BLOCK_SIZE   = 100000;
  int OVER_SAMPLE  = 2;
  //                                     scans       *    bytes / sample        *    1 sample + OVER_SAMPLEs */
  const int BULK_BYTES = BLOCK_SIZE  * NUM_CHANNELS  * sizeof( unsigned short ) *    11;
  const double CLOCK_SPEED = 100000;	// Hz
  double clockHz = 0;
  int result;
  int SLEEP_TIME = 2;

  AIOUSB_Reset( deviceIndex );
  ADC_SetOversample( deviceIndex, OVER_SAMPLE );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  AIOUSB_SetStreamingBlockSize( deviceIndex, BLOCK_SIZE );

  AIOUSB_Reset( deviceIndex );
  ADC_SetOversample( deviceIndex, OVER_SAMPLE );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  AIOUSB_SetStreamingBlockSize( deviceIndex, BLOCK_SIZE );
    
  unsigned short *const dataBuf = ( unsigned short * ) malloc( BULK_BYTES );

  if( dataBuf != 0 ) {
    /*
     * make sure counter is stopped
     */
    CTR_StartOutputFreq( deviceIndex, 0, &clockHz );

    // configure A/D for timer-triggered acquisition
       
    ADC_ADMode( deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );

    
    // start bulk acquire; ADC_BulkAcquire() will take care of starting
    // and stopping the counter; but we do have to tell it what clock
    // speed to use, which is why we call AIOUSB_SetMiscClock()
    
    AIOUSB_SetMiscClock( deviceIndex, CLOCK_SPEED );
    result = ADC_BulkAcquire( deviceIndex, BULK_BYTES, dataBuf );

    THROW_IF_ERROR( result, "attempting to start bulk acquire" );
    INFO("Started bulk acquire of %d bytes\n", BULK_BYTES );

    // Use bulk poll to monitor progress
    if( result == AIOUSB_SUCCESS ) {
      unsigned long bytesRemaining = BULK_BYTES;
      for( int seconds = 0; seconds < 100; seconds++ ) {
        sleep( SLEEP_TIME );
        result = TEST_ADC_BulkPoll( deviceIndex, &bytesRemaining );
        if( result == AIOUSB_SUCCESS ) {
          printf( "  %lu bytes remaining\n", bytesRemaining );
          if( bytesRemaining == 0 )
            break;	// from for()
        } else {
          printf( "Error '%s' polling bulk acquire progress\n" , AIOUSB_GetResultCodeAsString( result ) );
          break;
        }
      }
      
      // Turn off timer-triggered mode
      ADC_ADMode( deviceIndex, 0, AD_CAL_MODE_NORMAL );
    }
  } 
  free( dataBuf );
}


// 
// Demonstrate bulk acquire
// 
void TestCaseSetup::doBulkAcquire(int block_size, int over_sample, double clock_speed )
{
  int BLOCK_SIZE   = block_size;
  int OVER_SAMPLE  = over_sample;
  //                                     scans       *    bytes / sample        *    1 sample + OVER_SAMPLEs */
  const int BULK_BYTES = BLOCK_SIZE  * NUM_CHANNELS  * sizeof( unsigned short ) *    11;
  //const double CLOCK_SPEED = 100000;	// Hz

  double CLOCK_SPEED   = clock_speed;
  INFO("Doing something");

  double clockHz = 0;
  int result;
  int SLEEP_TIME = 2;

  AIOUSB_Reset( deviceIndex );
  ADC_SetOversample( deviceIndex, OVER_SAMPLE );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  AIOUSB_SetStreamingBlockSize( deviceIndex, BLOCK_SIZE );

  AIOUSB_Reset( deviceIndex );
  ADC_SetOversample( deviceIndex, OVER_SAMPLE );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  AIOUSB_SetStreamingBlockSize( deviceIndex, BLOCK_SIZE );
    
  unsigned short *const dataBuf = ( unsigned short * ) malloc( BULK_BYTES );

  if( dataBuf != 0 ) {
    /*
     * make sure counter is stopped
     */
    CTR_StartOutputFreq( deviceIndex, 0, &clockHz );

    // configure A/D for timer-triggered acquisition
       
    ADC_ADMode( deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );

    
    // start bulk acquire; ADC_BulkAcquire() will take care of starting
    // and stopping the counter; but we do have to tell it what clock
    // speed to use, which is why we call AIOUSB_SetMiscClock()
    
    AIOUSB_SetMiscClock( deviceIndex, CLOCK_SPEED );
    result = ADC_BulkAcquire( deviceIndex, BULK_BYTES, dataBuf );

    THROW_IF_ERROR( result, "attempting to start bulk acquire" );
    INFO("Started bulk acquire of %d bytes\n", BULK_BYTES );

    // Use bulk poll to monitor progress
    if( result == AIOUSB_SUCCESS ) {
      unsigned long bytesRemaining = BULK_BYTES;
      for( int seconds = 0; seconds < 100; seconds++ ) {
        sleep( SLEEP_TIME );
        result = TEST_ADC_BulkPoll( deviceIndex, &bytesRemaining );
        if( result == AIOUSB_SUCCESS ) {
          // printf( "  %lu bytes remaining\n", bytesRemaining );
          if( bytesRemaining == 0 )
            break;	// from for()
        } else {
          // printf( "Error '%s' polling bulk acquire progress\n" , AIOUSB_GetResultCodeAsString( result ) );
          // break;
          THROW_IF_ERROR( result, " polling bulk acquire progress" );
        }
      }
      
      // Turn off timer-triggered mode
      ADC_ADMode( deviceIndex, 0, AD_CAL_MODE_NORMAL );
    }
  } 
  free( dataBuf );
}



// 
// Test for the bulk poll that might determine the problems
// 
// 
unsigned long TestCaseSetup::TEST_ADC_BulkPoll( unsigned long DeviceIndex,
                                                unsigned long *BytesLeft
                                                ) 
{
  if( BytesLeft == NULL )
    return AIOUSB::AIOUSB_ERROR_INVALID_PARAMETER;

  if( ! AIOUSB::AIOUSB_Lock() )
    return AIOUSB::AIOUSB_ERROR_INVALID_MUTEX;

  unsigned long result;
  result = AIOUSB::AIOUSB_Validate( &DeviceIndex );
  if( result != AIOUSB::AIOUSB_SUCCESS ) {
    ERROR("Time out occured here");
    AIOUSB::AIOUSB_UnLock();
    return result;
  }

  DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
  if( deviceDesc->bADCStream == AIOUSB::AIOUSB_FALSE ) {
    AIOUSB::AIOUSB_UnLock();
    return AIOUSB::AIOUSB_ERROR_NOT_SUPPORTED;
  }

  *BytesLeft = deviceDesc->workerStatus;
  result     = deviceDesc->workerResult;

  if( result != AIOUSB::AIOUSB_SUCCESS )  {
    ERROR("Timeout from workerResult\n");
  }
  AIOUSB::AIOUSB_UnLock();

  return result;
}



//
// \desc verify that A/D ground calibration is correct 
// 
void TestCaseSetup::doVerifyGroundCalibration(void)
{
  INFO("doVerifyGroundCalibration:\tVerifying the ground counts\n");
  ADC_SetOversample( deviceIndex, 0 );
  ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL );
  ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_GROUND );

  int result = ADC_GetScan( deviceIndex, this->counts );
  THROW_IF_ERROR( result, "attempting to read ground counts" );
  INFO("doVerifyGroundCalibration:\tGround counts = %u (should be approx. 0)\n", this->counts[ CAL_CHANNEL ] );
}

void TestCaseSetup::THROW_IF_ERROR(int result , const char *format, ... ) {
  char buffer[BUFSIZ];
  va_list args;
  va_start(args, format );
  vsprintf(buffer, format, args );
  va_end(args);
}


int TestCaseSetup::envGetInteger(const char *env)
{
  char *tmp = getenv(env);
  if( !tmp ) {
    return 0;
  } else {
    return atoi(tmp);
  }
}

double TestCaseSetup::envGetDouble(const char *env)
{
  char *tmp = getenv(env);
  if( !tmp ) {
    return (double)0;
  } else {
    return  atol(env);
  }
}



double *TestCaseSetup::getVolts() { return volts ; }

unsigned short *TestCaseSetup::getCounts() { return counts ;}
unsigned char *TestCaseSetup::getGainCodes() { return gainCodes ;}


void TestCaseSetup::doGenericVendorWrite(unsigned char Request, unsigned short Value, unsigned short Index, unsigned long *DataSize, void *pData)
{
  int result = 0;
  INFO( "doGenericVendorWrite:\tDoing Generic Vendor Write\n" );
  /*result = GenericVendorWrite( deviceIndex, Request, Value, Index, DataSize, pData );*/
  THROW_IF_ERROR( result, "performing GenericVendorWrite" );
  INFO( "doGenericVendorWrite:\tCompleted Generic Vendor Write\n" );
}


// 
// \desc Sets up the :auto: calibration mode
// 
void TestCaseSetup::doSetAutoCalibration(void)
{
  int result;
  INFO( "doSetAutoCalibtration:\tSetting Auto Calibration\n" );
  /*
   * demonstrate automatic A/D calibration
   */
  result = ADC_SetCal( deviceIndex, ":AUTO:" );
  THROW_IF_ERROR( result, "performing automatic A/D calibration" );
  INFO("doSetAutoCalibtration:\tA/D settings successfully configured\n");
}

void TestCaseSetup::doTestSetAutoCalibration(void)
{
  int result = 3;
  INFO("doTestSetAutoCalibration:\tTesting the auto calibration");
  
  
  THROW_IF_ERROR( result, "Performing check of Auto calibration" );
  INFO("doTestSetAutoCalibration:\tAuto calibration testing completed successfully");
}


//
// \desc Verify that A/D reference calibration is correct
// 
void TestCaseSetup::doVerifyReferenceCalibration(void)
{
  INFO( "doVerifyReferenceCalibration:\tChecking Reference Calibration\n");
  ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_REFERENCE );
  int result = ADC_GetScan( deviceIndex, counts );
  THROW_IF_ERROR( result, "attempting to read reference counts" );
  INFO( "doVerifyReferenceCalibration:\tReference counts = %u (should be approx. 65130)\n", counts[ CAL_CHANNEL ] );

}

//
// @desc DEMONSTRATE SCANNING CHANNELS AND MEASURING VOLTAGES
// 
void TestCaseSetup::doDemonstrateReadVoltages() 
{
  LOG("Running test inside of the doDemonstrateReadVoltage\n");
  int result;
  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
  ADC_SetOversample( deviceIndex, 10 );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
  result = ADC_GetScanV( deviceIndex, volts );
  
  THROW_IF_ERROR( result, " performing A/D channel scan" );

  LOG("Volts Read:\n");
  for( int channel = 0; channel < NUM_CHANNELS ; channel ++ ) { 
    LOG("\tChannel %d = %f\n", channel, volts[channel] );
  }
}


// 
// Simple version that just outputs data to csv file
// 
void TestCaseSetup::doCSVReadVoltages()
{
  DEBUG("Running test inside of the doDemonstrateReadVoltage\n");
  int result;
  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  // ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
  // ADC_SetOversample( deviceIndex, 10 );
  // ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  // ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
  result = ADC_GetScanV( deviceIndex, volts );
  THROW_IF_ERROR( result, " performing A/D channel scan" );
  for( int channel = 0; channel < NUM_CHANNELS - 1 ; channel ++ ) { 
    LOG("%.3f,", volts[channel] );
  }
  LOG("%f\n", volts[NUM_CHANNELS-1] );

}

void TestCaseSetup::doPreReadImmediateVoltages()
{
  DEBUG("Running test inside of the doPreReadImmediateVoltages\n");

  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
  ADC_SetOversample( deviceIndex, 10 );
  ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
  ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );

  DEBUG("Completed doPreReadImmediateVoltages\n");

}



// 
// \desc demonstrate reading a single channel in volts
// 
void TestCaseSetup::doScanSingleChannel()
{

  int result = ADC_GetChannelV( deviceIndex, CAL_CHANNEL, &volts[ CAL_CHANNEL ] );
  THROW_IF_ERROR( result, " reading A/D channel " );
  LOG("Volts read from A/D channel %d = %f\n", CAL_CHANNEL, volts[ CAL_CHANNEL ] );
}



//
// \desc sets up the voltage parameters for runs
// 
// 
void TestCaseSetup::setupVoltageParameters(void)
{
  volts     = new double[MAX_CHANNELS];
  gainCodes = new unsigned char[NUM_CHANNELS];
}



