/**
 * @file   cust_sample.cpp
 * @author Jimi Damon <jdamon@accesio.com>
 * @date   Tue Aug 21 17:11:15 2012
 * 
 * @brief  Sample that allows for the aquisiton of data either in 
 * bulk (burst) or in immediate mode ( pulse ).
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <aiousb.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "TestCaseSetup.h"
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

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


using namespace AIOUSB;

int AioUsbFindDeviceIndex(unsigned long *deviceIndex);
void AioUsbPrintDeviceProps(DeviceProperties *devProps);
char *get_time();
int not_done( struct options *opts);

struct options {
  int stderr;
  int count;
};

struct options get_options(int argc, char **argv );

#define DEFAULT_VALUE( x, y ) ( x ? x : y ) 

int main(int argc, char **argv) {
  unsigned long result = AIOUSB_Init();
  struct options opts = get_options(argc, argv);
  setbuf(stdout, NULL);
  fflush(stdout);

  if (result == AIOUSB_SUCCESS) {
    unsigned long deviceIndex = 0;
    AIOUSB_BOOL deviceFound = AioUsbFindDeviceIndex(&deviceIndex);

    if (deviceFound == AIOUSB_TRUE ) {
      if( opts.stderr ) 
        printf("DEVICE INDEX: %lu\n\n", deviceIndex);

      DeviceProperties devProps;
      unsigned long result = AIOUSB_GetDeviceProperties(deviceIndex,
                                                        &devProps);
      if (result == AIOUSB_SUCCESS && opts.stderr ) {
        AioUsbPrintDeviceProps(&devProps);
      }

      // const double CLOCK_SPEED = 500000;
      // const unsigned char NUM_OVERSAMPLES = 1;
      // const unsigned STREAMING_BLOCK_SIZE = 512 * 1000;
      // const unsigned NUM_SCANS = 12800;
      // const int clkspeed                  = (double)clkspeed;
      const double CLOCK_SPEED            = DEFAULT_VALUE( TestCaseSetup::envGetDouble("CLOCK_SPEED") , 500000 );
      const unsigned char NUM_OVERSAMPLES = DEFAULT_VALUE( TestCaseSetup::envGetInteger("NUM_OVERSAMPLES"), 1 );
      // const unsigned STREAMING_BLOCK_SIZE = (unsigned)DEFAULT_VALUE(TestCaseSetup::envGetInteger("STREAMING_BLOCK_SIZE"), (512 * 1000));
      const unsigned NUM_SCANS            = DEFAULT_VALUE( TestCaseSetup::envGetInteger("NUM_SCANS") , 12800);


      AIOUSB_Reset(deviceIndex);
      ADC_SetOversample(deviceIndex, NUM_OVERSAMPLES);
      ADC_SetScanLimits(deviceIndex, 0, devProps.ADCChannels - 1);
      //AIOUSB_SetStreamingBlockSize(deviceIndex, STREAMING_BLOCK_SIZE);

      // stop counter
      double clockHz = 0;
      CTR_StartOutputFreq(deviceIndex, 0, &clockHz);

      // set timer-triggered mode
      ADC_ADMode(deviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER,
                 AD_CAL_MODE_NORMAL);
      AIOUSB_SetMiscClock(deviceIndex, CLOCK_SPEED);

      // allocate data buffer
      const unsigned long BULK_BYTES = NUM_SCANS * devProps.ADCChannels
        * sizeof(unsigned short) * (1 + NUM_OVERSAMPLES);
      // const unsigned long BULK_BYTES = 100;
      unsigned short dataBuf[BULK_BYTES];

      // keep acquiring
      while ( not_done( &opts) ) {
        result = ADC_BulkAcquire(deviceIndex, BULK_BYTES, &dataBuf);
        if (result == AIOUSB_SUCCESS) {
          // printf("Started bulk acquire of %d bytes\n", BULK_BYTES);
          char *now_time = get_time();
          fprintf(stdout,"%s,START,", now_time);

          // poll while we don't receive everything
          unsigned long bytesRemaining = BULK_BYTES;
          int bulk_fail = 0;

          while (bytesRemaining > 0) {
            usleep(50000);
            result = ADC_BulkPoll(deviceIndex, &bytesRemaining);
            if (result == AIOUSB_SUCCESS) {
              if( opts.stderr ) 
                fprintf(stderr,"  %lu bytes remaining\n", bytesRemaining);
              if (bytesRemaining == 0)
                break;
            } else {
              if( opts.stderr ) 
                fprintf(stderr,"Error '%s' polling bulk acquire progress\n",
                        AIOUSB_GetResultCodeAsString(result));
              bulk_fail = 1;
              continue;
            }
          }

          if (result == AIOUSB_SUCCESS && bytesRemaining == 0 && bulk_fail == 0 ) {
            // printf("Successfully bulk acquired %d bytes\n", BULK_BYTES);
            fprintf(stdout,"PASS,%lu,%lu\n",BULK_BYTES, result);
          } else {
            // printf("Failed to bulk acquire %d bytes\n", BULK_BYTES);
            fprintf(stdout,"FAIL,%lu,%lu\n",BULK_BYTES, result);
          }
        } else {
          // printf(
          //        "Error '%s' attempting to start bulk acquire of %d bytes\n",
          //        AIOUSB_GetResultCodeAsString(result), BULK_BYTES);
          fprintf(stdout,"%lu,FAIL,,%lu,\n",time(NULL),result);
        }
      }
      fflush(stdout);
    }

    AIOUSB_Exit();
  }

  return result;
}

/** 
 * @desc Just parse the command line
 * @param argc 
 * @param argv 
 * 
 * @return struct options An options object for 
 */
struct options get_options(int argc, char **argv )
{
  int opt;
  struct options retoptions = {0,-1};
  while ((opt = getopt(argc, argv, "c:e")) != -1) {
    switch (opt) {
    case 'e':
      retoptions.stderr = 1;
      break;
    case 'c':
      retoptions.count = atoi(optarg);
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-e ] \n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  return retoptions;
}

int not_done( struct options *opts)
{
  if( opts->count >= 0 ) {
    opts->count --;
    return (opts->count) >= 0;
  } else {
    return 1;
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
// #ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
//   clock_serv_t cclock;
//   mach_timespec_t mts;
//   host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
//  clock_get_time(cclock, &mts);
//  mach_port_deallocate(mach_task_self(), cclock);
//  tv.tv_sec = mts.tv_sec;
//  tv.tv_nsec = mts.tv_nsec;
// #else
 clock_gettime(CLOCK_REALTIME, &tv);
// #endif
  // clock_gettime( CLOCK_REALTIME, &tv );
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


/**
 * Standard procedure for getting device index.
 * Returns AIOUSB_TRUE if device exists and its index has been found.
 * In that case it also sets passed argument to actual device index.
 * Else it returns AIOUSB_FALSE and leaves parameter intact.
 */
int  AioUsbFindDeviceIndex(unsigned long *deviceIndex) {
  unsigned long devId = 0;

  AIOUSB_BOOL deviceFound = AIOUSB_FALSE;
  DeviceProperties devProps;
  unsigned long deviceMask = GetDevices();
  while (deviceMask != 0) {
    if ((deviceMask & 1) != 0) {
      unsigned long result = AIOUSB_GetDeviceProperties(devId, &devProps);
      if (result == AIOUSB_SUCCESS) {
        if (devProps.ProductID == USB_AI16_64MA) {
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DA12_8A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DA12_8E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO_32 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO_48 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO_96 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DI16A_REV_A1 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DO16A_REV_A1 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DI16A_REV_A2 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO_16H ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DI16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DO16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IIRO_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_II_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_RO_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IIRO_8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_II_8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IIRO_4 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IDIO_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_II_16_OLD ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IDO_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IDIO_8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_II_8_OLD ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IDIO_4 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_CTR_15 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IIRO4_2SM ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_IIRO4_COM ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_DIO16RO8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_16E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_16E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_64MA ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_64ME ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64MA ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64M ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64ME ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_32A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_32E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_32A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_32 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_32E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_64A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_64E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_64E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_96A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_96E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_96A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_96 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_96E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_128A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI16_128E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_128A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_128 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AI12_128E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_12A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_12 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_8A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_4A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO16_4 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_12A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_12 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_8A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_8 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_4A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AO12_4 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_16E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_16A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_16 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_16E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_64MA ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_64ME ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64MA ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64M ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64ME ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_32A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_32E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_32A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_32 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_32E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_64A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_64E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_64E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_96A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_96E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_96A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_96 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_96E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_128A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO16_128E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_128A ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_128 ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
        else if( devProps.ProductID == USB_AIO12_128E ) { 
          deviceFound = AIOUSB_TRUE;
          break;
        }
      } else {
        printf("Error '%s' querying device at index %lu!\n",
               AIOUSB_GetResultCodeAsString(result), devId);
      }
    }
    devId++;
    deviceMask >>= 1;
  }

  if (deviceFound == AIOUSB_TRUE) {
    deviceIndex = &devId;
  }
  return deviceFound;
}

/**
 * Prints passed device properties object fields.
 */
void AioUsbPrintDeviceProps(DeviceProperties *devProps ) {
  fprintf(stderr, "---------- AIO USB Device Properties ----------\n");
  fprintf(stderr, "Name: %s\n", devProps->Name);
  fprintf(stderr, "ProductID: 0x%x\n", devProps->ProductID);
  fprintf(stderr, "SerialNumber: %d\n", (int)devProps->SerialNumber);
  fprintf(stderr, "ADCChannels: %u\n", devProps->ADCChannels);
  fprintf(stderr, "ADCChannelsPerGroup: %u\n", devProps->ADCChannelsPerGroup);
  fprintf(stderr, "ADCMUXChannels: %u\n", devProps->ADCMUXChannels);
  fprintf(stderr, "Counters: %u\n", devProps->Counters);
  fprintf(stderr, "DACChannels: %u\n", devProps->DACChannels);
  fprintf(stderr, "DIOPorts: %u\n", devProps->DIOPorts);
  fprintf(stderr, "RootClock: %lu\n", devProps->RootClock);
  fprintf(stderr, "Tristates: %u\n", devProps->Tristates);
  fprintf(stderr, "-----------------------------------------------\n");
}
