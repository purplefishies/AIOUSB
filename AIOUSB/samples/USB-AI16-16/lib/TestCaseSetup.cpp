
#include "TestCaseSetup.h"

using namespace AIOUSB;

int CURRENT_DEBUG_LEVEL = 1;

TestCaseSetup::TestCaseSetup() : DeviceIndex(0) , 
                                 deviceFound(false) , 
                                 CAL_CHANNEL(DEF_CAL_CHANNEL), 
                                 MAX_CHANNELS(DEF_MAX_CHANNELS) , 
                                 NUM_CHANNELS(DEF_NUM_CHANNELS) ,
                                 number_oversamples(10)
{
  counts                  = (unsigned short*)malloc( sizeof(unsigned short)*MAX_CHANNELS);
  volts                   = (double *)malloc(sizeof(double)* MAX_CHANNELS);
  gainCodes               = (unsigned char *)malloc(sizeof(unsigned char)*NUM_CHANNELS);
  dataBuf                 = NULL;

  // Turn on the Debug level 
}

TestCaseSetup::~TestCaseSetup() 
{
  AIOUSB_Exit();
  free(counts);
  free(volts);
  free(gainCodes);
}


void TestCaseSetup::setCurrentDeviceIndex( int DI )
{
  this->DeviceIndex = DeviceIndex;
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
      result = QueryDeviceInfo( this->DeviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
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
    this->DeviceIndex++;
    deviceMask >>= 1;
  }	// while( deviceMask ...
}

void TestCaseSetup::doPreSetup()
{

  AIOUSB_Reset( DeviceIndex );
  AIOUSB_SetCommTimeout( DeviceIndex, 1000 );
  AIOUSB_SetDiscardFirstSample( DeviceIndex, AIOUSB_TRUE );
  
  __uint64_t serialNumber;
  int result = GetDeviceSerialNumber( DeviceIndex, &serialNumber );
  if( result != AIOUSB_SUCCESS  ) {
    std::stringstream er;
    er << "Error '" << AIOUSB_GetResultCodeAsString( result );
    er << "' getting serial number of device at index " << DeviceIndex ;
    throw Error( er.str().c_str() );
  }
}


/** 
 * @desc Exception handler
 * 
 * @param result 
 * @param linnum 
 */
void 
TestCaseSetup::ThrowError( unsigned long result , int linnum )
{
  std::stringstream er;
  er << "Error at line:" << linnum << " " << AIOUSB_GetResultCodeAsString( result ) << "' setting A/D configuration";
  throw Error(er.str().c_str());
}


void 
TestCaseSetup::doFastITScanSetup()
{

  DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
  unsigned long result;

  result = ADC_SetCal( DeviceIndex, ":AUTO:");
  CHECK_RESULT( result );

  result = ADC_GetConfig( DeviceIndex, ADC_GetADConfigBlock_Registers( &deviceDesc->cachedConfigBlock ), &deviceDesc->ConfigBytes );
  CHECK_RESULT( result );

  for( int i = 0 ; i <= 15 ; i ++ ) 
    AIOUSB_SetRegister( &deviceDesc->cachedConfigBlock, i , 0x00 );

  AIOUSB_SetRegister( &deviceDesc->cachedConfigBlock, 0x13 , 0x07 );
  

  result = ADC_SetConfig( DeviceIndex, &deviceDesc->cachedConfigBlock.registers[0] , &deviceDesc->ConfigBytes );
  CHECK_RESULT( result );
}

void
TestCaseSetup::doFastITScan( int numgets )
{
  unsigned long result;
  double *data;

  result = ADC_InitFastITScanV( DeviceIndex  );
  CHECK_RESULT( result );
  data = (double *)malloc( sizeof(double)*16 );

  
  for( int i = 0 ; i < numgets ; i ++ )  {
    result = ADC_GetFastITScanV( DeviceIndex , data);
    for( int j = 0; j <= 15; j ++ ) { 
      std::cout << data[j] << ",";
    }
    std::cout << std::endl;
    // Display the results
    
    CHECK_RESULT( result );
  }

  free(data);

  // Now perform a reset at the end
  ADC_ResetFastITScanV( DeviceIndex );
 
}


/** 
 * @desc Uploads a bulk configuration block
 * 
 */
void TestCaseSetup::doBulkConfigBlock()
{
   AIOUSB_InitConfigBlock( &configBlock, DeviceIndex, AIOUSB_FALSE );

   AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
   AIOUSB_SetCalMode( &configBlock, AD_CAL_MODE_NORMAL );

   AIOUSB_SetTriggerMode( &configBlock, 0 );
   AIOUSB_SetScanRange( &configBlock, 2, 13 );
   AIOUSB_SetOversample( &configBlock, 0 );
   int result = ADC_SetConfig( DeviceIndex, configBlock.registers, &configBlock.size );
   if( result != AIOUSB_SUCCESS  ) {
     std::stringstream er;
     er << "Error '" << AIOUSB_GetResultCodeAsString( result ) << "' setting A/D configuration";
     throw Error(er.str().c_str());
   }                  
}

/** 
 * @desc Demonstrate bulk acquire
 * 
 */
void TestCaseSetup::doBulkAcquire(void)
{
  int BLOCK_SIZE   = 100000;
  int OVER_SAMPLE  = 2;
  const double CLOCK_SPEED = 100000;	// Hz
  doBulkAcquire( BLOCK_SIZE, OVER_SAMPLE, CLOCK_SPEED );
}

/** 
 * @desc Demonstrate bulk acquire
 * 
 * @param block_size 
 * @param over_sample 
 * @param clock_speed 
 */
void TestCaseSetup::doBulkAcquire(int block_size, int over_sample, double clock_speed )
{

  int BLOCK_SIZE      = ( block_size <= 0 ? 100000  : block_size );
  int OVER_SAMPLE     = ( over_sample <= 0 || over_sample > 255 ? 10 : over_sample );
  double CLOCK_SPEED  = ( clock_speed <= 0 ? 100000 : clock_speed );
  //                                     scans       *    bytes / sample        *    1 sample + OVER_SAMPLEs */
  const int BULK_BYTES = BLOCK_SIZE  * NUM_CHANNELS  * sizeof( unsigned short ) *    11;


  double clockHz = 0;
  int result;
  int SLEEP_TIME = 1;

  INFO("Setting up over samples and scan limits\n");

  AIOUSB_Reset( DeviceIndex );
  ADC_SetOversample( DeviceIndex, OVER_SAMPLE );
  ADC_SetScanLimits( DeviceIndex, 0, NUM_CHANNELS - 1 );
  AIOUSB_SetStreamingBlockSize( DeviceIndex, BLOCK_SIZE );

  // AIOUSB_Reset( DeviceIndex );
  // ADC_SetOversample( DeviceIndex, OVER_SAMPLE );
  // ADC_SetScanLimits( DeviceIndex, 0, NUM_CHANNELS - 1 );
  // AIOUSB_SetStreamingBlockSize( DeviceIndex, BLOCK_SIZE );
    
  // unsigned short *const dataBuf = ( unsigned short * ) malloc( BULK_BYTES );
  dataBuf = ( unsigned short * ) malloc( BULK_BYTES );

  if( dataBuf != 0 ) {
    /*
     * make sure counter is stopped
     */
    CTR_StartOutputFreq( DeviceIndex, 0, &clockHz );

    // configure A/D for timer-triggered acquisition
       
    ADC_ADMode( DeviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL );

    
    // start bulk acquire; ADC_BulkAcquire() will take care of starting
    // and stopping the counter; but we do have to tell it what clock
    // speed to use, which is why we call AIOUSB_SetMiscClock()
    
    AIOUSB_SetMiscClock( DeviceIndex, CLOCK_SPEED );
    result = ADC_BulkAcquire( DeviceIndex, BULK_BYTES, dataBuf );

    THROW_IF_ERROR( result, "attempting to start bulk acquire" );
    INFO("Started bulk acquire of %d bytes\n", BULK_BYTES );
    // Use bulk poll to monitor progress
    if( result == AIOUSB_SUCCESS ) {
      unsigned long bytesRemaining = BULK_BYTES;
      for( int seconds = 0; seconds < 100; seconds++ ) {
        TRACE("Sleeping %d seconds\n", SLEEP_TIME );
        sleep(SLEEP_TIME );
        result = TEST_ADC_BulkPoll( DeviceIndex, &bytesRemaining );
        if( result == AIOUSB_SUCCESS ) {
          DEBUG( "  %lu bytes remaining\n", bytesRemaining );

          if( bytesRemaining == 0 )
            break;	// from for()

        } else {

          ERROR( "Error '%s' polling bulk acquire progress\n" , AIOUSB_GetResultCodeAsString( result ) );

          THROW_IF_ERROR( result, " polling bulk acquire progress" );
        }
      }
      
      // Turn off timer-triggered mode
      ADC_ADMode( DeviceIndex, 0, AD_CAL_MODE_NORMAL );
    }
  } 
}

unsigned short *
TestCaseSetup::doGetBuffer()
{
  return this->dataBuf;
}


void TestCaseSetup::doCleanupAfterBulk()
{
  TRACE("doCleanupAfterBulk:\n");
  INFO("Deallocating buffer\n");
  free( dataBuf );
  TRACE("doCleanupAfterBulk: Leaving\n");
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
// @desc verify that A/D ground calibration is correct 
// 
void TestCaseSetup::doVerifyGroundCalibration(void)
{
  TRACE("doVerifyGroundCalibration:\tVerifying the ground counts\n");
  ADC_SetOversample( DeviceIndex, 0 );
  ADC_SetScanLimits( DeviceIndex, CAL_CHANNEL, CAL_CHANNEL );
  ADC_ADMode( DeviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_GROUND );

  int result = ADC_GetScan( DeviceIndex, this->counts );
  THROW_IF_ERROR( result, "attempting to read ground counts" );
  INFO("Ground counts = %u (should be approx. 0)\n", this->counts[ CAL_CHANNEL ] );
  TRACE("leaving doVerifyGroundCalibration\n");
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
  /*result = GenericVendorWrite( DeviceIndex, Request, Value, Index, DataSize, pData );*/
  THROW_IF_ERROR( result, "performing GenericVendorWrite" );
  INFO( "doGenericVendorWrite:\tCompleted Generic Vendor Write\n" );
}


/** 
 * @desc Sets up the :auto: calibration mode
 * 
 */
void TestCaseSetup::doSetAutoCalibration(void)
{
  int result;
  TRACE("doSetAutoCalibtration:");
  INFO("Setting Auto Calibration\n" );
  /*
   * demonstrate automatic A/D calibration
   */
  result = ADC_SetCal( DeviceIndex, ":AUTO:" );
  THROW_IF_ERROR( result, "performing automatic A/D calibration" );
  INFO("A/D settings successfully configured\n");
  TRACE("Done doSetAutoCalibtration\n");
}

void TestCaseSetup::doTestSetAutoCalibration(void)
{
  int result = 3;
  INFO("doTestSetAutoCalibration:\tTesting the auto calibration");
  
  
  THROW_IF_ERROR( result, "Performing check of Auto calibration" );
  INFO("doTestSetAutoCalibration:\tAuto calibration testing completed successfully");
}


/** 
 * @desc Verify that A/D reference calibration is correct
 * 
 */
void TestCaseSetup::doVerifyReferenceCalibration(void)
{
  TRACE("doVerifyReferenceCalibration:\t");
  INFO( "Checking Reference Calibration\n");
  ADC_ADMode( DeviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_REFERENCE );
  int result = ADC_GetScan( DeviceIndex, counts );
  THROW_IF_ERROR( result, "attempting to read reference counts" );
  INFO( "eference counts = %u (should be approx. 65130)\n", counts[ CAL_CHANNEL ] );
  TRACE("doVerifyReferenceCalibration:\tCompleted");
}

/** 
 *  @desc DEMONSTRATE SCANNING CHANNELS AND MEASURING VOLTAGES 
 * 
 */
void TestCaseSetup::doDemonstrateReadVoltages() 
{
  LOG("Running test inside of the doDemonstrateReadVoltage\n");
  int result;
  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  ADC_RangeAll( DeviceIndex, gainCodes, AIOUSB_TRUE );
  ADC_SetOversample( DeviceIndex, 10 );
  ADC_SetScanLimits( DeviceIndex, 0, NUM_CHANNELS - 1 );
  ADC_ADMode( DeviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
  result = ADC_GetScanV( DeviceIndex, volts );
  
  THROW_IF_ERROR( result, " performing A/D channel scan" );

  LOG("Volts Read:\n");
  for( int channel = 0; channel < NUM_CHANNELS ; channel ++ ) { 
    LOG("\tChannel %d = %f\n", channel, volts[channel] );
  }
}

/** 
 * @desc Simple version that just outputs data to csv file
 * 
 */
void TestCaseSetup::doCSVReadVoltages()
{
  DEBUG("Running test inside of the doDemonstrateReadVoltage\n");
  int result;
  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  // ADC_RangeAll( DeviceIndex, gainCodes, AIOUSB_TRUE );
  // ADC_SetOversample( DeviceIndex, 10 );
  // ADC_SetScanLimits( DeviceIndex, 0, NUM_CHANNELS - 1 );
  // ADC_ADMode( DeviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
  result = ADC_GetScanV( DeviceIndex, volts );
  THROW_IF_ERROR( result, " performing A/D channel scan" );
  for( int channel = 0; channel < NUM_CHANNELS - 1 ; channel ++ ) { 
    LOG("%.3f,", volts[channel] );
  }
  LOG("%f\n", volts[NUM_CHANNELS-1] );

}


/** 
 * @desc Performs an immediate read of voltages
 * 
 */
void TestCaseSetup::doPreReadImmediateVoltages()
{
  DEBUG("Running test inside of the doPreReadImmediateVoltages\n");

  for( int channel = 0; channel < NUM_CHANNELS; channel++ )
    gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
  
  ADC_RangeAll( DeviceIndex, gainCodes, AIOUSB_TRUE );
  ADC_SetOversample( DeviceIndex, 10 );
  ADC_SetScanLimits( DeviceIndex, 0, NUM_CHANNELS - 1 );
  ADC_ADMode( DeviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );

  DEBUG("Completed doPreReadImmediateVoltages\n");

}

/** 
 * @desc demonstrate reading a single channel in volts
 * 
 */
void TestCaseSetup::doScanSingleChannel()
{

  int result = ADC_GetChannelV( DeviceIndex , CAL_CHANNEL, &volts[ CAL_CHANNEL ] );
  THROW_IF_ERROR( result, " reading A/D channel " );
  LOG("Volts read from A/D channel %d = %f\n", CAL_CHANNEL, volts[ CAL_CHANNEL ] );
}

/** 
 * @desc sets up the voltage parameters for runs
 * 
 */
void TestCaseSetup::setupVoltageParameters(void)
{
  volts     = new double[MAX_CHANNELS];
  gainCodes = new unsigned char[NUM_CHANNELS];
}



