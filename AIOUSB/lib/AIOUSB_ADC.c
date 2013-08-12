/**
 * @file   aiousb.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  Configuration functions for ADC elements
 *
 */

#include "AIOUSB_Core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BAD_RESULT_AND_GOTO(x, y)  result = x; goto (y);


#ifdef __cplusplus
namespace AIOUSB {
#endif


enum {
    AD_MAX_CHANNELS           = 128,                // maximum number of channels supported by this driver
    AD_GAIN_CODE_MASK               = 7
};

static const struct ADRange {
    double minVolts;
    double range;
} adRanges[ AD_NUM_GAIN_CODES ] = {
    { 0, 10 },                                                    // AD_GAIN_CODE_0_10V
    { -10, 20 },                                                  // AD_GAIN_CODE_10V
    { 0, 5 },                                                     // AD_GAIN_CODE_0_5V
    { -5, 10 },                                                   // AD_GAIN_CODE_5V
    { 0, 2 },                                                     // AD_GAIN_CODE_0_2V
    { -2, 4 },                                                    // AD_GAIN_CODE_2V
    { 0, 1 },                                                     // AD_GAIN_CODE_0_1V
    { -1, 2 }                                                     // AD_GAIN_CODE_1V
};

// formerly public in the API
static unsigned long ADC_GetImmediate(
    unsigned long DeviceIndex,
    unsigned long Channel,
    unsigned short *pData);


/**
 * @desc
 * @param DeviceIndex
 * @param forceRead
 *
 * @return
 */
static unsigned long
ReadConfigBlock(unsigned long DeviceIndex,
                AIOUSB_BOOL forceRead
                )
{
    unsigned long result;
    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
    if(!deviceDesc || result != AIOUSB_SUCCESS)
        return result;

    if(forceRead || deviceDesc->cachedConfigBlock.size == 0) {
          libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
          if(deviceHandle != NULL) {
            /* request A/D configuration block from device */

                ADConfigBlock configBlock;
                configBlock.device = deviceDesc;
                configBlock.size = deviceDesc->ConfigBytes;
                const unsigned timeout = deviceDesc->commTimeout;

                AIOUSB_UnLock(); /* unlock while communicating with device */

                const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                     USB_READ_FROM_DEVICE,
                                                                     AUR_ADC_GET_CONFIG,
                                                                     0,
                                                                     0,
                                                                     configBlock.registers,
                                                                     configBlock.size,
                                                                     timeout
                                                                     );
                if(bytesTransferred == ( int )configBlock.size) {
                  /*
                   * check and correct settings read from device
                   */
                      AIOUSB_Lock();
                      unsigned channel;
                      for(channel = 0; channel < AD_NUM_GAIN_CODE_REGISTERS; channel++) {
                            if(
                                (
                                    configBlock.registers[ AD_CONFIG_GAIN_CODE + channel ]
                                    & ~( unsigned char )(AD_DIFFERENTIAL_MODE | AD_GAIN_CODE_MASK)
                                ) != 0
                                )
                                configBlock.registers[ AD_CONFIG_GAIN_CODE + channel ] = FIRST_ENUM(ADGainCode);
                        }

                      const unsigned char calMode = configBlock.registers[ AD_CONFIG_CAL_MODE ];
                      if(
                          calMode != AD_CAL_MODE_NORMAL &&
                          calMode != AD_CAL_MODE_GROUND &&
                          calMode != AD_CAL_MODE_REFERENCE
                          )
                          configBlock.registers[ AD_CONFIG_CAL_MODE ] = AD_CAL_MODE_NORMAL;

                      if((configBlock.registers[ AD_CONFIG_TRIG_COUNT ] & ~AD_TRIGGER_VALID_MASK) != 0)
                          configBlock.registers[ AD_CONFIG_TRIG_COUNT ] = 0;

                      const unsigned endChannel = AIOUSB_GetEndChannel(&configBlock);
                      if(
                          endChannel >= ( unsigned )deviceDesc->ADCMUXChannels ||
                          AIOUSB_GetStartChannel(&configBlock) > endChannel
                          )
                          AIOUSB_SetScanRange(&configBlock, 0, deviceDesc->ADCMUXChannels - 1);

                      deviceDesc->cachedConfigBlock = configBlock;
                      AIOUSB_UnLock();
                  }else
                    result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
            }else {
                result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                AIOUSB_UnLock();
            }
      }else
        AIOUSB_UnLock();
    return result;
}


/**
 *
 *
 * @param DeviceIndex
 *
 * @return
 */
static unsigned long
WriteConfigBlock(unsigned long DeviceIndex)
{
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->cachedConfigBlock.size > 0) {
          libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
          if(deviceHandle != NULL) {
                ADConfigBlock configBlock;
                configBlock = deviceDesc->cachedConfigBlock;
                const unsigned timeout = deviceDesc->commTimeout;
                AIOUSB_UnLock();                      // unlock while communicating with device
                assert(configBlock.size > 0 &&
                       configBlock.size <= AD_MAX_CONFIG_REGISTERS);
                const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                     USB_WRITE_TO_DEVICE, AUR_ADC_SET_CONFIG,
                                                                     0, 0, configBlock.registers, configBlock.size, timeout);
                if(bytesTransferred != ( int )configBlock.size)
                    result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
            } else {
                result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                AIOUSB_UnLock();
            }
      }else {
          result = AIOUSB_ERROR_INVALID_DATA;
          AIOUSB_UnLock();
      }

    return result;
}



ADConfigBlock *
AIOUSB_GetConfigBlock( unsigned long DeviceIndex )
{
     unsigned long result = ReadConfigBlock( DeviceIndex, AIOUSB_TRUE );
     DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
     ADConfigBlock *tmp;
     if(!deviceDesc || result != AIOUSB_SUCCESS)
          return (ADConfigBlock *)NULL;

     tmp = (ADConfigBlock *)malloc(sizeof(ADConfigBlock));
     *tmp = deviceDesc->cachedConfigBlock;
     AIOUSB_UnLock();
     return tmp;
}

unsigned long
AIOUSB_SetConfigBlock( unsigned long DeviceIndex , ADConfigBlock *entry )
{
     unsigned long result = AIOUSB_SUCCESS;
     DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( DeviceIndex, &result);
     if( !entry ) 
          return AIOUSB_ERROR_INVALID_DATA;
     if(!deviceDesc || result != AIOUSB_SUCCESS)
          return AIOUSB_ERROR_INVALID_DATA;

     deviceDesc->cachedConfigBlock = *entry;
     /* memcpy(&(deviceDesc->cachedConfigBlock), entry, sizeof( ADConfigBlock ) ); */

     AIOUSB_UnLock();
     result = WriteConfigBlock(  DeviceIndex );

     return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param counts
 *
 * @return
 */
PRIVATE unsigned long
AIOUSB_GetScan(
    unsigned long DeviceIndex,
    unsigned short counts[]
    )
{
    unsigned long result;
    
    if(counts == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;
    DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
    libusb_device_handle *deviceHandle;
    if(result != AIOUSB_SUCCESS)
        goto out_aiousb_getscan;

    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
        result = AIOUSB_ERROR_NOT_SUPPORTED;
        goto out_aiousb_getscan;
    }

    deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);

    if(deviceHandle != NULL) {
          AIOUSB_UnLock();                                // unlock while communicating with device
          result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
          AIOUSB_Lock();

          if(result == AIOUSB_SUCCESS) {
                const ADConfigBlock origConfigBlock = deviceDesc->cachedConfigBlock;         // restore when done
                AIOUSB_BOOL configChanged       = AIOUSB_FALSE;
                AIOUSB_BOOL discardFirstSample  = deviceDesc->discardFirstSample;
                unsigned startChannel           = AIOUSB_GetStartChannel(&deviceDesc->cachedConfigBlock);
                unsigned endChannel             = AIOUSB_GetEndChannel(&deviceDesc->cachedConfigBlock);
                unsigned overSample             = AIOUSB_GetOversample(&deviceDesc->cachedConfigBlock);
                assert(startChannel <= endChannel);
                int numChannels = endChannel - startChannel + 1;

                /**
                 * in theory, all the A/D functions, including
                 * AIOUSB_GetScan(), should work in all measurement
                 * modes, including calibration mode; in practice,
                 * however, the device will return only a single
                 * sample in calibration mode; therefore, users must
                 * be careful to select a single channel and set
                 * oversample to zero during calibration mode;
                 * attempting to read more than one channel or use an
                 * oversample setting of more than zero in calibration
                 * mode will result in a timeout error; as a
                 * convenience to the user we automatically impose
                 * this restriction here in AIOUSB_GetScan(); if the
                 * device is changed to permit normal use of the A/D
                 * functions in calibration mode, we will have to
                 * modify this function to somehow recognize which
                 * devices support that capability, or simply delete
                 * this restriction altogether and rely on the users'
                 * good judgment
                 */

                const unsigned calMode = AIOUSB_GetCalMode(&deviceDesc->cachedConfigBlock);
                if(calMode == AD_CAL_MODE_GROUND || calMode == AD_CAL_MODE_REFERENCE) {
                      if(numChannels > 1) {
                            AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, startChannel, endChannel = startChannel);
                            numChannels = 1;
                            configChanged = AIOUSB_TRUE;
                        }
                      if(overSample > 0) {
                            AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, overSample = 0);
                            configChanged = AIOUSB_TRUE;
                        }
                      discardFirstSample = AIOUSB_FALSE;           // this feature can't be used in calibration mode either
                  }

                /**
                 * turn scan on and turn timer and external trigger
                 * off
                 */
                const unsigned origTriggerMode = AIOUSB_GetTriggerMode(&deviceDesc->cachedConfigBlock);
                unsigned triggerMode = origTriggerMode;
                triggerMode |= AD_TRIGGER_SCAN;                                              // enable scan
                triggerMode &= ~(AD_TRIGGER_TIMER | AD_TRIGGER_EXTERNAL);         // disable timer and external trigger
                if(triggerMode != origTriggerMode) {
                      AIOUSB_SetTriggerMode(&deviceDesc->cachedConfigBlock, triggerMode);
                      configChanged = AIOUSB_TRUE;
                  }

                /**
                 * the oversample setting dictates how many samples to
                 * take _in addition_ to the primary sample; if
                 * oversample is zero, we take just one sample for
                 * each channel; if oversample is greater than zero
                 * then we average the primary sample and all of its
                 * over-samples; if the discardFirstSample setting is
                 * enabled, then we discard the primary sample,
                 * leaving just the over-samples; thus, if
                 * discardFirstSample is enabled, we must take at
                 * least one over-sample in order to have any data
                 * left; there's another complication: the device
                 * buffer is limited to a small number of samples, so
                 * we have to limit the number of over-samples to what
                 * the device buffer can accommodate, so the actual
                 * oversample setting depends on the number of
                 * channels being scanned; we also preserve and
                 * restore the original oversample setting specified
                 * by the user; since the user is expecting to average
                 * (1 + oversample) samples, then if
                 * discardFirstSample is enabled we simply always add
                 * one
                 */

                const unsigned origOverSample = overSample;
                int samplesPerChannel = 1 + origOverSample;
                if(discardFirstSample)
                    samplesPerChannel++;
                if(samplesPerChannel > 256)
                    samplesPerChannel = 256;              // constrained by maximum oversample of 255

                /**
                 * make sure device buffer can accommodate this number
                 * of samples
                 */
                const int DEVICE_SAMPLE_BUFFER_SIZE = 1024;   /* number of samples device can buffer */
                if((numChannels * samplesPerChannel) > DEVICE_SAMPLE_BUFFER_SIZE)
                    samplesPerChannel = DEVICE_SAMPLE_BUFFER_SIZE / numChannels;
                overSample = samplesPerChannel - 1;
                if(overSample != origOverSample) {
                      AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, overSample);
                      configChanged = AIOUSB_TRUE;
                  }

                if(configChanged) {
                      AIOUSB_UnLock();                // unlock while communicating with device
                      result = WriteConfigBlock(DeviceIndex);
                      AIOUSB_Lock();
                  }
                if(result == AIOUSB_SUCCESS) {
                      const int numSamples = numChannels * samplesPerChannel;
                      const unsigned short numSamplesHigh = ( unsigned short )(numSamples >> 16);
                      const unsigned short numSamplesLow = ( unsigned short )numSamples;
                      const int numBytes = numSamples * sizeof(unsigned short);
                      unsigned short *const sampleBuffer = ( unsigned short* )malloc(numBytes);
                      assert(sampleBuffer != 0);
                      if(sampleBuffer != 0) {
                            const unsigned timeout = deviceDesc->commTimeout;
                            AIOUSB_UnLock();             // unlock while communicating with device
                            int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                           USB_WRITE_TO_DEVICE, AUR_START_ACQUIRING_BLOCK,
                                                                           numSamplesHigh, numSamplesLow, 0, 0, timeout);
                            if(bytesTransferred == 0) {

                                /* request AUR_ADC_IMMEDIATE triggers the sampling of data */

                                  bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                             USB_READ_FROM_DEVICE, AUR_ADC_IMMEDIATE,
                                                                             0, 0, ( unsigned char* )sampleBuffer /* discarded */, sizeof(unsigned short),
                                                                             timeout);
                                  if(bytesTransferred == sizeof(unsigned short)) {
                                        const int libusbResult = AIOUSB_BulkTransfer(deviceHandle,
                                                                                     LIBUSB_ENDPOINT_IN | USB_BULK_READ_ENDPOINT,
                                                                                     ( unsigned char* )sampleBuffer, numBytes, &bytesTransferred,
                                                                                     timeout);
                                        if(libusbResult != LIBUSB_SUCCESS) {
                                              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                                          }else if(bytesTransferred != numBytes) {
                                              result = AIOUSB_ERROR_INVALID_DATA;
                                          }else {
                                             /**
                                              * Compute the average of all the samples taken for each channel, discarding
                                              * the first sample if that option is enabled; each byte in sampleBuffer[] is
                                              * 1 of 2 bytes for each sample, the first byte being the LSB and the second
                                              * byte the MSB, in other words, little-endian format; so for convenience we
                                              * simply declare sampleBuffer[] to be of type 'unsigned short' and the data
                                              * is already in the correct format; the device returns data only for the
                                              * channels requested, from startChannel to endChannel; AIOUSB_GetScan()
                                              * returns the averaged data readings in counts[], putting the reading for
                                              * startChannel in counts[0], and the reading for endChannel in counts[numChannels-1]
                                              */
                                              const int samplesToAverage
                                                  = discardFirstSample
                                                    ? samplesPerChannel - 1
                                                    : samplesPerChannel;
                                              int sampleIndex = 0;
                                              int channel;
                                              for(channel = 0; channel < numChannels; channel++) {
                                                    unsigned long sampleSum = 0;
                                                    if(discardFirstSample)
                                                        sampleIndex++;                          // skip over first sample
                                                    int sample;
                                                    for(sample = 0; sample < samplesToAverage; sample++)
                                                        sampleSum += sampleBuffer[ sampleIndex++ ];
                                                    counts[ channel ] = ( unsigned short )
                                                                        ((sampleSum + samplesToAverage / 2) / samplesToAverage);
                                                }
                                          }
                                    }else
                                      result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                              }else
                                result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                            free(sampleBuffer);
                        }else {
                            result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                            AIOUSB_UnLock();
                        }
                  }else
                    AIOUSB_UnLock();
                if(configChanged) {
                      AIOUSB_Lock();
                      deviceDesc->cachedConfigBlock = origConfigBlock;
                      AIOUSB_UnLock();                // unlock while communicating with device
                      WriteConfigBlock(DeviceIndex);
                  }
            }else
              AIOUSB_UnLock();
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }
out_aiousb_getscan:
    AIOUSB_UnLock();
    return result;
}



/**
 * @desc
 * @param DeviceIndex
 * @param startChannel
 * @param numChannels
 * @param counts
 * @param volts
 * @return
 */
PRIVATE unsigned long AIOUSB_ArrayCountsToVolts(
    unsigned long DeviceIndex,
    int startChannel,
    int numChannels,
    const unsigned short counts[],
    double volts[]
     )
{
     unsigned long result;
     DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( DeviceIndex, &result);
     if( !deviceDesc || result != AIOUSB_SUCCESS ) {

     }
     
     assert(startChannel >= 0 &&
            numChannels >= 0 &&
            startChannel + numChannels <= ( int )deviceDesc->ADCMUXChannels &&
            counts != 0 &&
            volts != 0);
     if(
          startChannel < 0 ||
          numChannels < 0 ||
          startChannel + numChannels > ( int )deviceDesc->ADCMUXChannels ||
          counts == NULL ||
          volts == NULL
          ) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
     }

     AIOUSB_UnLock();                                        // unlock while communicating with device
     result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
     if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          int channel;
          for(channel = 0; channel < numChannels; channel++) {
               const int gainCode = AIOUSB_GetGainCode(&deviceDesc->cachedConfigBlock, startChannel + channel);
               assert(gainCode >= FIRST_ENUM(ADGainCode) && gainCode <= LAST_ENUM(ADGainCode));
               const struct ADRange *const range = &adRanges[ gainCode ];
               volts[ channel ] = ( (( double )counts[ channel ] / ( double )AI_16_MAX_COUNTS) * range->range ) + range->minVolts;
          }
          AIOUSB_UnLock();
     }

     return result;
}

/**
 *
 *
 * @param DeviceIndex
 * @param startChannel
 * @param numChannels
 * @param volts
 * @param counts
 *
 * @return
 */
PRIVATE unsigned long AIOUSB_ArrayVoltsToCounts(
    unsigned long DeviceIndex,
    int startChannel,
    int numChannels,
    const double volts[],
    unsigned short counts[]
    )
{
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    assert(startChannel >= 0 &&
           numChannels >= 0 &&
           startChannel + numChannels <= ( int )deviceDesc->ADCMUXChannels &&
           volts != 0 &&
           counts != 0);
    if(
        startChannel < 0 ||
        numChannels < 0 ||
        startChannel + numChannels > ( int )deviceDesc->ADCMUXChannels ||
        volts == NULL ||
        counts == NULL
        ) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
      }

    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          int channel;
          for(channel = 0; channel < numChannels; channel++) {
                const int gainCode = AIOUSB_GetGainCode(&deviceDesc->cachedConfigBlock, startChannel + channel);
                assert(gainCode >= FIRST_ENUM(ADGainCode) &&
                       gainCode <= LAST_ENUM(ADGainCode));
                const struct ADRange *const range = &adRanges[ gainCode ];
                int rawCounts = round(
                    ( double )AI_16_MAX_COUNTS
                    * (volts[ channel ] - range->minVolts)
                    / range->range
                    );
                if(rawCounts < 0)
                    rawCounts = 0;
                else if(rawCounts > AI_16_MAX_COUNTS)
                    rawCounts = AI_16_MAX_COUNTS;
                counts[ channel ] = ( unsigned short )rawCounts;
            }
          AIOUSB_UnLock();
      }

    return result;
}





/**
 *
 *
 * @param DeviceIndex
 * @param ChannelIndex
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_GetChannelV(
    unsigned long DeviceIndex,
    unsigned long ChannelIndex,
    double *pBuf
    )
{
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }
    if(
        pBuf == NULL ||
        ChannelIndex >= deviceDesc->ADCMUXChannels
        ) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
      }

    /**
     * there is no guarantee that ChannelIndex, passed by the user, is
     * within the current channel scan range; if it is not, then valid
     * data cannot be returned; in addition, since we're only returning
     * the data for a single channel, there's no need to scan all the
     * channels; the Windows implementation attempts to improve
     * performance by caching all the values read; but the technique is
     * riddled with problems; first of all, it can easily return extremely
     * stale data, without any indication to the user; secondly, it can
     * return data for channels that weren't even scanned, without any
     * indication to the user; thirdly, caching is unnecessary; if the
     * user wants to read a single channel they can call
     * ADC_GetChannelV(); if the user wants to improve performance by
     * reading multiple channels they can call ADC_GetScanV(); so to
     * address all these issues, we temporarily compress the scan range to
     * just ChannelIndex and then restore it when we're done; so in this
     * implementation all calls to ADC_GetChannelV() return "real-time"
     * data for the specified channel
     */
    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          const ADConfigBlock origConfigBlock = deviceDesc->cachedConfigBlock;       // restore when done
          AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, ChannelIndex, ChannelIndex);
          AIOUSB_UnLock();                              // unlock while communicating with device
          result = WriteConfigBlock(DeviceIndex);
          unsigned short counts;
          result = AIOUSB_GetScan(DeviceIndex, &counts);
          if(result == AIOUSB_SUCCESS) {
                double volts;
                result = AIOUSB_ArrayCountsToVolts(DeviceIndex, ChannelIndex, 1, &counts, &volts);
                if(result == AIOUSB_SUCCESS)
                    *pBuf = volts;
                else
                    *pBuf = 0.0;
            }
          AIOUSB_Lock();
          deviceDesc->cachedConfigBlock = origConfigBlock;
          AIOUSB_UnLock();                              // unlock while communicating with device
          WriteConfigBlock(DeviceIndex);
      }

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_GetScanV(
    unsigned long DeviceIndex,
    double *pBuf
    )
{
    if(pBuf == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result == AIOUSB_SUCCESS) {
          DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
          if(deviceDesc->bADCStream) {
               /**
                * get raw A/D counts
                */
                unsigned short *const counts = ( unsigned short* )malloc(deviceDesc->ADCMUXChannels * sizeof(unsigned short));
                assert(counts != 0);
                if(counts != 0) {
                      AIOUSB_UnLock();            // unlock while communicating with device
                      result = ADC_GetScan(DeviceIndex, counts);
                      AIOUSB_Lock();
                      if(result == AIOUSB_SUCCESS) {
                           /**
                            * Convert from A/D counts to volts; only
                            * the channels from startChannel to
                            * endChannel contain valid data, so we
                            * only convert those; pBuf[] is expected
                            * to contain entries for all the A/D
                            * channels; so for cleanliness, we zero
                            * out the channels in pBuf[] that aren't
                            * going to be filled in with real readings
                            */
                            const unsigned startChannel = AIOUSB_GetStartChannel(&deviceDesc->cachedConfigBlock),
                                           endChannel = AIOUSB_GetEndChannel(&deviceDesc->cachedConfigBlock);
                            assert(startChannel <= endChannel);

                            /**
                             * zero out unused channels
                             */
                            unsigned channel;
                            for(channel = 0; channel < deviceDesc->ADCMUXChannels; channel++) {
                                  if(
                                      channel < startChannel ||
                                      channel > endChannel
                                      )
                                      pBuf[ channel ] = 0.0;
                              }

                            /**
                             * convert remaining channels to volts
                             */
                            result = AIOUSB_ArrayCountsToVolts(DeviceIndex, startChannel, endChannel - startChannel + 1,
                                                               counts + startChannel, pBuf + startChannel);
                        }
                      free(counts);
                  }else
                    result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
            }else
              result = AIOUSB_ERROR_NOT_SUPPORTED;
      }

    AIOUSB_UnLock();
    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_GetScan(
    unsigned long DeviceIndex,
    unsigned short *pBuf
    )
{
    if(pBuf == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    /**
     * pBuf[] is expected to contain entries for all the A/D channels,
     * even though we may be reading only a few channels; so for
     * cleanliness, we zero out the channels in pBuf[] that aren't
     * going to be filled in with real readings
     */
    memset(pBuf, 0, deviceDesc->ADCMUXChannels * sizeof(unsigned short));
    const unsigned startChannel = AIOUSB_GetStartChannel(&deviceDesc->cachedConfigBlock);
    AIOUSB_UnLock();
    return AIOUSB_GetScan(DeviceIndex, pBuf + startChannel);
}



/**
 * @desc Copies the old Cached Config block registers into the pConfigBuf
 *       object.
 * @param DeviceIndex
 * @param pConfigBuf
 * @param ConfigBufSize
 *
 * @return
 */
unsigned long ADC_GetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize
    )
{
    if( pConfigBuf == NULL || ConfigBufSize == NULL ) 
         return AIOUSB_ERROR_INVALID_PARAMETER;
    
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->ConfigBytes == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }
    if(*ConfigBufSize < deviceDesc->ConfigBytes) {
          *ConfigBufSize = deviceDesc->ConfigBytes;
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
      }

    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_TRUE);
    if(result == AIOUSB_SUCCESS) {
          assert(deviceDesc->cachedConfigBlock.size > 0 &&
                 deviceDesc->cachedConfigBlock.size <= AD_MAX_CONFIG_REGISTERS);
          AIOUSB_Lock();
          memcpy(pConfigBuf, deviceDesc->cachedConfigBlock.registers, deviceDesc->cachedConfigBlock.size);
          *ConfigBufSize = deviceDesc->cachedConfigBlock.size;
          AIOUSB_UnLock();
      }

    AIOUSB_UnLock();
    return result;
}

int adcblock_valid_trigger_settings(ADConfigBlock *config )
{
     return (config->registers[ AD_CONFIG_TRIG_COUNT ] & ~AD_TRIGGER_VALID_MASK ) == 0;
}

int adcblock_valid_channel_settings(ADConfigBlock *config , int ADCMUXChannels )
{
     int result = 1;

     for(int channel = 0; channel < AD_NUM_GAIN_CODE_REGISTERS; channel++) {
          if(( config->registers[ AD_CONFIG_GAIN_CODE + channel ] & ~( unsigned char )(AD_DIFFERENTIAL_MODE | AD_GAIN_CODE_MASK)) != 0 ) {
               return 0;
          }
     }

     unsigned endChannel = AIOUSB_GetEndChannel( config );
     if( endChannel >= ( unsigned )ADCMUXChannels || AIOUSB_GetStartChannel( config ) > endChannel ) {
          result = AIOUSB_ERROR_INVALID_PARAMETER;
     }

     return result;
}

unsigned long 
valid_config_block( ADConfigBlock *config )
{
     unsigned long result = 0;
     return result;
}

int 
adcblock_valid_size( ADConfigBlock *config )
{
     return config->size > 0 && config->size <= AD_MAX_CONFIG_REGISTERS;
}

/**
 *
 *
 * @param DeviceIndex
 * @param pConfigBuf
 * @param ConfigBufSize
 *
 * @return
 */
unsigned long ADC_SetConfig(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize
    )
{

     unsigned endChannel;
     unsigned long result;
     if( pConfigBuf == NULL || ConfigBufSize == NULL )
          return AIOUSB_ERROR_INVALID_PARAMETER;

     DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
     if(result != AIOUSB_SUCCESS || !deviceDesc)
          goto out_ADC_SetConfig;

     if(deviceDesc->ConfigBytes == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
     }

     /**
      * validate settings
      */
     if(*ConfigBufSize < deviceDesc->ConfigBytes) {
          *ConfigBufSize = deviceDesc->ConfigBytes;
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
     }

     ADConfigBlock configBlock;
     configBlock.device = deviceDesc;
     configBlock.size   = deviceDesc->ConfigBytes;

     memcpy(configBlock.registers, pConfigBuf, configBlock.size);
     
     if( ! adcblock_valid_size( &configBlock ) ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CHANNEL_SETTING;
          goto out_ADC_SetConfig;
     }

     if( !adcblock_valid_channel_settings( &configBlock , deviceDesc->ADCMUXChannels )  ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CHANNEL_SETTING;
          goto out_ADC_SetConfig;
     }

     if( !VALID_ENUM( ADCalMode , configBlock.registers[ AD_CONFIG_CAL_MODE ] ) ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CAL_SETTING;
          goto out_ADC_SetConfig;
     }

     if( !adcblock_valid_trigger_settings( &configBlock ) )  {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_SETTING;
          goto out_ADC_SetConfig;  
     }

     endChannel = AIOUSB_GetEndChannel( &configBlock );
     if( endChannel >= ( unsigned )deviceDesc->ADCMUXChannels || 
         AIOUSB_GetStartChannel(&configBlock) > endChannel ) {
          result = AIOUSB_ERROR_INVALID_PARAMETER;
          goto out_ADC_SetConfig;
     }


     deviceDesc->cachedConfigBlock = configBlock;


     AIOUSB_UnLock();                                        // unlock while communicating with device
     result = WriteConfigBlock(DeviceIndex);

     if(result == AIOUSB_SUCCESS)
          *ConfigBufSize = configBlock.size;

out_ADC_SetConfig:
     AIOUSB_UnLock();
     return result;
}


/** 
 * @desc Copies the given ADConfig object into the cachedConfigBlock
 * that is used to communicate with the USB device 
 * @param DeviceIndex
 * @param config
 * 
 * @return 
 */
unsigned long ADC_CopyConfig(
    unsigned long DeviceIndex,
    ADConfigBlock *config
    )
{

     unsigned long result;
     if( config== NULL )
          return AIOUSB_ERROR_INVALID_PARAMETER;

     DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
     if( !deviceDesc || result != AIOUSB_SUCCESS )
          goto out_ADC_CopyConfig;

     if(deviceDesc->ConfigBytes == 0) {
          result =  AIOUSB_ERROR_NOT_SUPPORTED;
          goto out_ADC_CopyConfig;
     }

     /**
      * validate settings
      */
     if( config->size < deviceDesc->ConfigBytes) {
          config->size = deviceDesc->ConfigBytes;
          result =  AIOUSB_ERROR_INVALID_PARAMETER;
          goto out_ADC_CopyConfig;
     }

     /* ADConfigBlock config; */
     /* config.device = deviceDesc; */
     /* config.size   = deviceDesc->ConfigBytes; */
     /* memcpy(config.registers, pConfigBuf, config.size); */
     
     if( ! adcblock_valid_size( config ) ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CHANNEL_SETTING;
          goto out_ADC_CopyConfig;
     }
     
     if( !adcblock_valid_channel_settings( config  , deviceDesc->ADCMUXChannels )  ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CHANNEL_SETTING;
          goto out_ADC_CopyConfig;
     }

     if( !VALID_ENUM( ADCalMode , config->registers[ AD_CONFIG_CAL_MODE ] ) ) {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_CAL_SETTING;
          goto out_ADC_CopyConfig;
     }

     if( !adcblock_valid_trigger_settings( config ) )  {
          result = AIOUSB_ERROR_INVALID_ADCONFIG_SETTING;
          goto out_ADC_CopyConfig;  
     }

     deviceDesc->cachedConfigBlock = *config;


     AIOUSB_UnLock();                                        // unlock while communicating with device
     result = WriteConfigBlock(DeviceIndex);

     if(result == AIOUSB_SUCCESS)
          deviceDesc->cachedConfigBlock.size = config->size;

out_ADC_CopyConfig:
     AIOUSB_UnLock();
     return result;
}





/**
 *
 *
 * @param DeviceIndex
 * @param pGainCodes
 * @param bSingleEnded
 *
 * @return
 */
unsigned long ADC_RangeAll(
    unsigned long DeviceIndex,
    unsigned char *pGainCodes,
    unsigned long bSingleEnded
    )
{
    if(
        pGainCodes == NULL ||
        (
            bSingleEnded != AIOUSB_FALSE &&
            bSingleEnded != AIOUSB_TRUE
        )
        )
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(
        deviceDesc->ADCChannels == 0 ||
        deviceDesc->bADCStream == AIOUSB_FALSE
        ) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

/*
 * validate gain codes; they should be just gain codes; single-ended or differential
 * mode is specified by bSingleEnded
 */
    unsigned channel;
    for(channel = 0; channel < deviceDesc->ADCChannels; channel++) {
          if((pGainCodes[ AD_CONFIG_GAIN_CODE + channel ] & ~AD_GAIN_CODE_MASK) != 0) {
                AIOUSB_UnLock();
                return AIOUSB_ERROR_INVALID_PARAMETER;
            }
      }

    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          for(channel = 0; channel < deviceDesc->ADCChannels; channel++) {
                AIOUSB_SetGainCode(&deviceDesc->cachedConfigBlock, channel, pGainCodes[ channel ]);
                AIOUSB_SetDifferentialMode(&deviceDesc->cachedConfigBlock, channel,
                                           (bSingleEnded == AIOUSB_FALSE) ? AIOUSB_TRUE : AIOUSB_FALSE);
            }
          AIOUSB_UnLock();                              // unlock while communicating with device
          result = WriteConfigBlock(DeviceIndex);
      }

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param ADChannel
 * @param GainCode
 * @param bSingleEnded
 *
 * @return
 */
unsigned long ADC_Range1(unsigned long DeviceIndex,
                         unsigned long ADChannel,
                         unsigned char GainCode,
                         unsigned long bSingleEnded
                         )
{
    unsigned long result;
    DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);

    if(result != AIOUSB_SUCCESS || !deviceDesc)
        goto out_adc_range1;


    result = AIOUSB_ERROR_NOT_SUPPORTED;
    if(deviceDesc->ADCMUXChannels == 0 || deviceDesc->bADCStream == AIOUSB_FALSE)
        goto out_adc_range1;

    result = AIOUSB_ERROR_INVALID_PARAMETER;
    if(
        (GainCode & ~AD_GAIN_CODE_MASK) != 0 ||
        (
            bSingleEnded != AIOUSB_FALSE &&
            bSingleEnded != AIOUSB_TRUE
        ) ||
        ADChannel >= deviceDesc->ADCMUXChannels
        )
        goto out_adc_range1;

    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          AIOUSB_SetGainCode(&deviceDesc->cachedConfigBlock, ADChannel, GainCode);
          AIOUSB_SetDifferentialMode(&deviceDesc->cachedConfigBlock, ADChannel,
                                     (bSingleEnded == AIOUSB_FALSE) ? AIOUSB_TRUE : AIOUSB_FALSE);
          AIOUSB_UnLock();                              // unlock while communicating with device
          result = WriteConfigBlock(DeviceIndex);
      }

    return result;
out_adc_range1:
    AIOUSB_UnLock();
    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param TriggerMode
 * @param CalMode
 *
 * @return
 */
unsigned long ADC_ADMode(
    unsigned long DeviceIndex,
    unsigned char TriggerMode,
    unsigned char CalMode
    )
{
    if(
        (TriggerMode & ~AD_TRIGGER_VALID_MASK) != 0 ||
        (
            CalMode != AD_CAL_MODE_NORMAL &&
            CalMode != AD_CAL_MODE_GROUND &&
            CalMode != AD_CAL_MODE_REFERENCE
        )
        )
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    AIOUSB_UnLock();                                        // unlock while communicating with device
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if(result == AIOUSB_SUCCESS) {
          AIOUSB_Lock();
          AIOUSB_SetCalMode(&deviceDesc->cachedConfigBlock, CalMode);
          AIOUSB_SetTriggerMode(&deviceDesc->cachedConfigBlock, TriggerMode);
          AIOUSB_UnLock();                              // unlock while communicating with device
          result = WriteConfigBlock(DeviceIndex);
      }

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param Oversample
 *
 * @return
 */
unsigned long ADC_SetOversample(
    unsigned long DeviceIndex,
    unsigned char Oversample
    )
{
    unsigned long result;
    DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);

    if(result != AIOUSB_SUCCESS || !deviceDesc )
        goto out_ADC_SetOversample;
    else if( deviceDesc->bADCStream == AIOUSB_FALSE) {
        result = AIOUSB_ERROR_NOT_SUPPORTED;
        goto out_ADC_SetOversample;
    }

    AIOUSB_UnLock(); /* unlock while communicating with the device */
    result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
    if( result != AIOUSB_SUCCESS) 
        goto out_ADC_SetOversample;
    AIOUSB_Lock();
    AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, Oversample);
    AIOUSB_UnLock();                              /*unlock while communicating with device*/
    result = WriteConfigBlock(DeviceIndex);

out_ADC_SetOversample:
    AIOUSB_UnLock();
    return result;
}

/** 
 * 
 * @param DeviceIndex 
 * 
 * @return 
 */
unsigned
ADC_GetOversample(
    unsigned long DeviceIndex
    )
{
    unsigned long result;
    DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);

    if(!deviceDesc || result != AIOUSB_SUCCESS )
         goto out_adc_setoversample;
    
    ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);

    result = (unsigned long)AIOUSB_GetOversample(&deviceDesc->cachedConfigBlock );

out_adc_setoversample:
   AIOUSB_UnLock();
   return result;

}

/**
 * @desc
 * @param DeviceIndex
 * @param StartChannel
 * @param EndChannel
 *
 * @return
 */
unsigned long ADC_SetScanLimits(
    unsigned long DeviceIndex,
    unsigned long StartChannel,
    unsigned long EndChannel
    )
{
    unsigned long result;
    DeviceDescriptor *const deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
    if(!deviceDesc || deviceDesc->bADCStream == AIOUSB_FALSE) {
        result =  AIOUSB_ERROR_NOT_SUPPORTED;
        goto out_ADC_SetScanLimits;
    }

    if( EndChannel > deviceDesc->ADCMUXChannels || StartChannel > EndChannel ) {
        result = AIOUSB_ERROR_INVALID_PARAMETER;
        goto out_ADC_SetScanLimits;
    }

    AIOUSB_UnLock();                                        // unlock while communicating with device
    if( (result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE)) == AIOUSB_SUCCESS ) {
          AIOUSB_Lock();
          AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, StartChannel, EndChannel);
          AIOUSB_UnLock();                              // unlock while communicating with device
          result = WriteConfigBlock(DeviceIndex);
    }
 out_ADC_SetScanLimits:
    AIOUSB_UnLock();
    return result;
}

/**
 * @desc 
 * @param DeviceIndex
 * @param CalFileName
 *
 * @return
 */
unsigned long ADC_SetCal(
    unsigned long DeviceIndex,
    const char *CalFileName
    )
{
    if(CalFileName == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    assert(strlen(CalFileName) > 0);
    unsigned long result;
    if(strcmp(CalFileName, ":AUTO:") == 0)
        result = AIOUSB_ADC_InternalCal(DeviceIndex, AIOUSB_TRUE, 0, 0);
    else if(
        strcmp(CalFileName, ":NONE:") == 0 ||
        strcmp(CalFileName, ":1TO1:") == 0
        )
        result = AIOUSB_ADC_InternalCal(DeviceIndex, AIOUSB_FALSE, 0, 0);
    else
        result = AIOUSB_ADC_LoadCalTable(DeviceIndex, CalFileName);

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 *
 * @return
 */
unsigned long ADC_QueryCal(
    unsigned long DeviceIndex
    )
{
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                              // unlock while communicating with device
          unsigned char calSupported = 0xff;       // so we can detect if it changes
          const int bytesTransferred = libusb_control_transfer(deviceHandle, USB_READ_FROM_DEVICE, AUR_PROBE_CALFEATURE,
                                                               0, 0, &calSupported, sizeof(calSupported), timeout);
          if(bytesTransferred == sizeof(calSupported)) {
                if(calSupported != 0xBB)              // 0xBB == AUR_LOAD_BULK_CALIBRATION_BLOCK
                    result = AIOUSB_ERROR_NOT_SUPPORTED;
            }else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param pConfigBuf
 * @param ConfigBufSize
 * @param CalFileName
 *
 * @return
 */
unsigned long ADC_Initialize(
    unsigned long DeviceIndex,
    unsigned char *pConfigBuf,
    unsigned long *ConfigBufSize,
    const char *CalFileName
    )
{
    unsigned long result = AIOUSB_SUCCESS;

    if(
        pConfigBuf != NULL &&
        ConfigBufSize != NULL
        )
        result = ADC_SetConfig(DeviceIndex, pConfigBuf, ConfigBufSize);

    if(
        result == AIOUSB_SUCCESS &&
        CalFileName != NULL
        )
        result = ADC_SetCal(DeviceIndex, CalFileName);

    return result;
}




static void *BulkAcquireWorker(void *params);

/**
 *
 *
 * @param DeviceIndex
 * @param BufSize
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_BulkAcquire(
    unsigned long DeviceIndex,
    unsigned long BufSize,
    void *pBuf
    )
{
    if(pBuf == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->workerBusy) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_OPEN_FAILED;
      }

    AIOUSB_UnLock();
    struct BulkAcquireWorkerParams *const acquireParams
        = ( struct BulkAcquireWorkerParams* )malloc(sizeof(struct BulkAcquireWorkerParams));
    assert(acquireParams != 0);
    if(acquireParams != 0) {
      /**
       * we initialize the worker thread status here in case the thread doesn't start for some reason,
       * such as an improperly locked mutex; this pre-initialization is necessary so that the thread
       * status doesn't make it appear as though the worker thread has completed successfully
       */
          AIOUSB_Lock();
          deviceDesc->workerStatus = BufSize;       // deviceDesc->workerStatus == bytes remaining to receive
          deviceDesc->workerResult = AIOUSB_ERROR_INVALID_DATA;
          deviceDesc->workerBusy = AIOUSB_TRUE;
          AIOUSB_UnLock();
          acquireParams->DeviceIndex = DeviceIndex;
          acquireParams->BufSize = BufSize;
          acquireParams->pBuf = pBuf;
          const int maxPriority = sched_get_priority_max(SCHED_FIFO);
          struct sched_param schedParam = { maxPriority };
          pthread_attr_t workerThreadAttr;
          pthread_t workerThreadID;
          pthread_attr_init(&workerThreadAttr);
          pthread_attr_setschedpolicy(&workerThreadAttr, SCHED_FIFO);
          pthread_attr_setschedparam(&workerThreadAttr, &schedParam);
          const int threadResult = pthread_create(&workerThreadID, &workerThreadAttr, BulkAcquireWorker, acquireParams);
          if(threadResult == 0) {
                sched_yield();
            }else {
/*
 * failed to create worker thread, clean up
 */
                AIOUSB_Lock();
                deviceDesc->workerStatus = 0;
                deviceDesc->workerResult = AIOUSB_SUCCESS;
                deviceDesc->workerBusy = AIOUSB_FALSE;
                AIOUSB_UnLock();
                free(acquireParams);
                result = AIOUSB_ERROR_INVALID_THREAD;
            }
          pthread_attr_destroy(&workerThreadAttr);
          pthread_detach(workerThreadID);
      }else
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;

    return result;
}





/**
 * @desc we assume the parameters passed to BulkAcquireWorker() have
 * been validated by ADC_BulkAcquire()
 * @param params
 * @return
 */
static void *BulkAcquireWorker(void *params)
{

    assert(params != 0);
    unsigned long result = AIOUSB_SUCCESS;
    struct BulkAcquireWorkerParams *const acquireParams = ( struct BulkAcquireWorkerParams* )params;
    AIOUSB_Lock();
    DeviceDescriptor *const deviceDesc = &deviceTable[ acquireParams->DeviceIndex ];
    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(acquireParams->DeviceIndex);
    if(deviceHandle != NULL) {
          unsigned long bytesRemaining = acquireParams->BufSize;
          deviceDesc->workerStatus = bytesRemaining;       // deviceDesc->workerStatus == bytes remaining to receive
          deviceDesc->workerResult = AIOUSB_SUCCESS;
          deviceDesc->workerBusy = AIOUSB_TRUE;
          double clockHz = deviceDesc->miscClockHz;
          const unsigned long streamingBlockSize = deviceDesc->StreamingBlockSize * sizeof(unsigned short);       // bytes
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                              // unlock while communicating with device
          const unsigned short numSamplesHigh = ( unsigned short )(acquireParams->BufSize >> 17);       // acquireParams->BufSize is bytes
          const unsigned short numSamplesLow = ( unsigned short )(acquireParams->BufSize >> 1);
          unsigned char *data = ( unsigned char* )acquireParams->pBuf;
          assert(data != 0);

          int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                         USB_WRITE_TO_DEVICE, AUR_START_ACQUIRING_BLOCK,
                                                         numSamplesHigh, numSamplesLow, 0, 0, timeout);
          if(bytesTransferred == 0) {
                CTR_StartOutputFreq(acquireParams->DeviceIndex, 0, &clockHz);
                while(bytesRemaining > 0) {
                      unsigned long bytesToTransfer
                          = (bytesRemaining < streamingBlockSize)
                            ? bytesRemaining
                            : streamingBlockSize;
                      const int libusbResult = AIOUSB_BulkTransfer(deviceHandle,
                                                                   LIBUSB_ENDPOINT_IN | USB_BULK_READ_ENDPOINT,
                                                                   data, ( int )bytesToTransfer, &bytesTransferred,
                                                                   timeout);
                      if(libusbResult != LIBUSB_SUCCESS) {
                            result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                            break;                            // from while()
                        }else if(bytesTransferred != ( int )bytesToTransfer) {
                            result = AIOUSB_ERROR_INVALID_DATA;
                            break;                            // from while()
                        }else {
                            data += bytesTransferred;
                            bytesRemaining -= bytesTransferred;
                            AIOUSB_Lock();
                            deviceDesc->workerStatus = bytesRemaining;
                            AIOUSB_UnLock();
                        }
                  }
                clockHz = 0;
                CTR_StartOutputFreq(acquireParams->DeviceIndex, 0, &clockHz);
            }else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    AIOUSB_Lock();
    deviceDesc->workerStatus = 0;
    deviceDesc->workerResult = result;
    deviceDesc->workerBusy = AIOUSB_FALSE;
    AIOUSB_UnLock();
    free(params);
    return 0;
}


AIOBuf *
NewBuffer( unsigned int bufsize )
{
     AIOBuf *tmp = (AIOBuf*)malloc( sizeof( AIOBuf) );
     if( !tmp ) {
          aio_errno = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          goto out_ret;
     }

     tmp->bytes_remaining = 0;
     tmp->bufsize = bufsize;
  
     /* printf("allocating space %d bytes\n",(int)tmp->bufsize ); */
     tmp->buffer = (unsigned short *)malloc( tmp->bufsize );
     if( !tmp->buffer ) {
          aio_errno = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          free(tmp);
          tmp = NULL;
     }
out_ret:
     return tmp;
}

void
DeleteBuffer( AIOBuf *buf )
{
  if( !buf ) 
    return;

  /* printf("Freeing buffer of size  %d bytes\n",(int)buf->bufsize); */

  if( buf->buffer ) 
    free(buf->buffer );
  free(buf);
}

/** 
 * @desc After setting up your oversamples and such, creates a new
 * AIOBuf object that can be used for BulkAcquiring.
 * @param DeviceIndex 
 * @return AIOBuf * new Buffer object for BulkAcquire methods
 * @todo Replace 16 with correct channels returned by probing the device
 */
AIOBuf *
CreateSmartBuffer( unsigned long DeviceIndex )
{
  unsigned long result;
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);
  if(!deviceDesc || result != AIOUSB_SUCCESS) {
       aio_errno = -result;
       return (AIOBuf*)NULL;
  }
  
  long size = ((1 + ADC_GetOversample( DeviceIndex ) ) * 16 * sizeof( unsigned short ) * AIOUSB_GetStreamingBlockSize(DeviceIndex)) ;
  AIOBuf *tmp = NewBuffer( size );
  
  return tmp;
}

AIORET_TYPE  
BulkAcquire(
            unsigned long DeviceIndex,
            AIOBuf *aiobuf,
            int size
            )
{
  AIORET_TYPE result = 0;
  /* printf("Using internal member with size %d\n", (int)aiobuf->bufsize ); */
  result = ADC_BulkAcquire( DeviceIndex, aiobuf->bufsize , aiobuf->buffer );
  aiobuf->bytes_remaining = aiobuf->bufsize;
  /* printf("Setting remaining value ot  %d\n",(int)aiobuf->bytes_remaining  ); */
  return result;

}


AIORET_TYPE  
BulkPoll(
         unsigned long DeviceIndex,
         AIOBuf *aiobuf
         )
{
  AIORET_TYPE result= AIOUSB_SUCCESS;
  unsigned long retval = ADC_BulkPoll( DeviceIndex, &(aiobuf->bytes_remaining) );
  if( retval != AIOUSB_SUCCESS ) 
    result = - retval;

  return result;
}





/**
 *
 *
 * @param DeviceIndex
 * @param BytesLeft
 *
 * @return
 */
unsigned long ADC_BulkPoll(
    unsigned long DeviceIndex,
    unsigned long *BytesLeft
    )
{
    if(BytesLeft == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    *BytesLeft = deviceDesc->workerStatus;
    result = deviceDesc->workerResult;
    AIOUSB_UnLock();

    return result;
}



/**
 * @desc
 * o this function is erroneously documented in the API specification, but it should
 *   not be made public; it is useful internally, however, for such things as
 *   calibrating the A/D converter
 *
 * o the specification does not include a Channel parameter, but this implementation
 *   does because the Pascal code does and because it's used by ADC_SetCal()
 *
 * o in a departure from the Pascal code, this function supports two categories of
 *   "immediate" A/Ds: the older products which have a single immediate A/D channel,
 *   and the newer products which have multiple immediate A/D channels; fortunately,
 *   this function accepts a pData parameter, which permits us to return any amount of
 *   data; the caller simply has to make sure that his pData buffer is large enough for
 *   the particular device; that's not an unreasonable demand since this function is
 *   used internally and not intended to be public
 */
static unsigned long ADC_GetImmediate(
    unsigned long DeviceIndex,
    unsigned long Channel,
    unsigned short *pData
    )
{
    if(pData == NULL)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->ImmADCs == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          const int numBytes = sizeof(unsigned short) * deviceDesc->ImmADCs;
          AIOUSB_UnLock();                              // unlock while communicating with device
          const int bytesTransferred = libusb_control_transfer(deviceHandle, USB_READ_FROM_DEVICE, AUR_ADC_IMMEDIATE,
                                                               0, Channel, ( unsigned char* )pData, numBytes, timeout);
          if(bytesTransferred != numBytes)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}


/**
 * @desc Creates FastIT Config Blocks
 *
 * @param DeviceIndex
 * @param size
 *
 * @return
 */
unsigned long
ADC_CreateFastITConfig(unsigned long DeviceIndex,
                       int size
                       )
{
    DeviceDescriptor *deviceDesc = &deviceTable[ DeviceIndex ];

    if(!deviceDesc->FastITConfig || deviceDesc->FastITConfig->size <= 0) {
          deviceDesc->FastITConfig = (ADConfigBlock*)malloc(sizeof(ADConfigBlock));
          deviceDesc->FastITBakConfig = (ADConfigBlock*)malloc(sizeof(ADConfigBlock));
          deviceDesc->FastITConfig->size = size;
          deviceDesc->FastITBakConfig->size = size;
      }
    return AIOUSB_SUCCESS;
}


unsigned char *
ADC_GetADConfigBlock_Registers(ADConfigBlock *config)
{
    return &(config->registers[0]);
}



/**
 * @desc Frees memory associated with the FastConfig Config blocks. Use
 *       this call after you are done using the ADC_FastIT* Functions
 * @param DeviceIndex
 */
void
ADC_ClearFastITConfig(unsigned long DeviceIndex)
{
    DeviceDescriptor *deviceDesc = &deviceTable[ DeviceIndex ];

    if(deviceDesc->FastITConfig->size) {
          free(deviceDesc->FastITConfig);
          free(deviceDesc->FastITBakConfig);
          deviceDesc->FastITConfig = NULL;
          deviceDesc->FastITBakConfig = NULL;
      }
}



unsigned long
ADC_CreateADBuf(DeviceDescriptor *const deviceDesc,
                int size
                )
{
    deviceDesc->ADBuf = (unsigned char*)malloc(sizeof(unsigned char*) * size);
    if(!deviceDesc->ADBuf) {
          return 0;
      }
    deviceDesc->ADBuf_size = size;
    return AIOUSB_SUCCESS;
}

void ADC_ClearADBuf(DeviceDescriptor *deviceDesc)
{
    if(deviceDesc->ADBuf_size) {
          free(deviceDesc->ADBuf);
          deviceDesc->ADBuf_size = 0;
      }
}

/**
 * @param DeviceIndex
 *
 * @return
 */

unsigned long ADC_InitFastITScanV(
    unsigned long DeviceIndex
    )
{
    int Dat;
    unsigned long result;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

/* ADConfigBlock configBlock; */


    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    result = AIOUSB_Validate(&DeviceIndex);

    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_ADC_InitFastITScanV;
      }

    result = AIOUSB_EnsureOpen(DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_ADC_InitFastITScanV;
      }

    if((result = ADC_CreateFastITConfig(DeviceIndex, 21)) != AIOUSB_SUCCESS)
        goto CLEAR_CONFIG_ADC_InitFastITScanV;


    if(deviceDesc->FastITConfig->size < 20) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto RETURN_ADC_InitFastITScanV;
      }

    result = ADC_GetConfig(DeviceIndex, &deviceDesc->FastITBakConfig->registers[0], &deviceDesc->FastITBakConfig->size);


    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          goto RETURN_ADC_InitFastITScanV;
      }

    /* Copy old config */
    memcpy(&deviceDesc->FastITConfig->registers[0], &deviceDesc->FastITBakConfig->registers[0], deviceDesc->FastITBakConfig->size);

    /* Make changes to the config block */
    Dat = AIOUSB_GetRegister(deviceDesc->FastITBakConfig, 0x10);
    AIOUSB_SetRegister(deviceDesc->FastITConfig, 0x11, (0x05 | (Dat & 0x10)));
    Dat = (3 > AIOUSB_GetRegister(deviceDesc->FastITBakConfig, 0x13) ? 3 : AIOUSB_GetRegister(deviceDesc->FastITBakConfig, 0x13));

    AIOUSB_SetRegister(deviceDesc->FastITConfig, 0x13, Dat);
    Dat = 64 > deviceDesc->ADCMUXChannels ?  deviceDesc->ADCMUXChannels - 1  : 63;

    AIOUSB_SetRegister(deviceDesc->FastITConfig, 0x12, Dat << 4);
    AIOUSB_SetRegister(deviceDesc->FastITConfig, 0x14, Dat & 0xF0);


    result = ADC_SetConfig(DeviceIndex, &deviceDesc->FastITConfig->registers[0], &deviceDesc->FastITConfig->size);

    if(result != AIOUSB_SUCCESS) {
          ADC_SetConfig(DeviceIndex, ADC_GetADConfigBlock_Registers(deviceDesc->FastITBakConfig), &deviceDesc->FastITConfig_size);
          return result;
      }

    result = 0;

CLEAR_CONFIG_ADC_InitFastITScanV:
    if(result != AIOUSB_SUCCESS) {
          if(deviceDesc->FastITConfig_size)
              ADC_ClearFastITConfig(DeviceIndex);
      }

RETURN_ADC_InitFastITScanV:
    AIOUSB_UnLock();
    return result;
}

/**
 *
 *
 * @param DeviceIndex
 *
 * @return
 */
unsigned long ADC_ResetFastITScanV(
    unsigned long DeviceIndex
    )
{
    unsigned long result = 0;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

/* unsigned long Dat; */
    if(!deviceDesc->bADCStream || deviceDesc->ConfigBytes < 20) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto RETURN_ADC_ResetFastITScanV;
      }
    result = ADC_SetConfig(DeviceIndex, ADC_GetADConfigBlock_Registers(deviceDesc->FastITBakConfig), &deviceDesc->ConfigBytes);
    if(result != AIOUSB_SUCCESS)
        goto RETURN_ADC_ResetFastITScanV;

/* Dat = 0x0; */
/* result = GenericVendorWrite( DeviceIndex, 0xD4, 0x1E, 0, sizeof(Dat), &Dat ); */
    ADC_ClearFastITConfig(DeviceIndex);

RETURN_ADC_ResetFastITScanV:
    return result;
}


/**
 *
 *
 * @param DeviceIndex
 *
 * @return
 */
unsigned long ADC_SetFastITScanVChannels(
    unsigned long DeviceIndex,
    unsigned long NewChannels
    )
{
    unsigned long result = 0;
    ADConfigBlock configBlock;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    configBlock.device = deviceDesc;
    configBlock.size = deviceDesc->ConfigBytes;

    result = AIOUSB_EnsureOpen(DeviceIndex);
    if(result != AIOUSB_SUCCESS)
        goto RETURN_ADC_SetFastITScanVChannels;

    if(!deviceDesc->bADCStream) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto RETURN_ADC_SetFastITScanVChannels;
      }

    if(configBlock.size < 20) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto RETURN_ADC_SetFastITScanVChannels;
      }

    result = ADC_SetConfig(DeviceIndex, ADC_GetADConfigBlock_Registers(deviceDesc->FastITConfig), &deviceDesc->ConfigBytes);

RETURN_ADC_SetFastITScanVChannels:
    return result;
}

/**
 * @desc Just a debugging function for listing all attributes of a 
 *       config object
 **/
void
ADC_Debug_Register_Settings(ADConfigBlock *config)
{
    int i;

    for(i = 0; i <= 15; i++) {
          printf("Channel %d:\t", i);
          switch(config->registers[i]) {
            case FIRST_ENUM(ADGainCode):
                printf("0-10V\n");
                break;

            case AD_GAIN_CODE_10V:
                printf("+/-10V\n");
                break;

            case AD_GAIN_CODE_0_5V:
                printf("0-5V\n");
                break;

            case AD_GAIN_CODE_5V:
                printf("+/-5V\n");
                break;

            case AD_GAIN_CODE_0_2V:
                printf("0-2V\n");
                break;

            case AD_GAIN_CODE_2V:
                printf("+/-2V\n");
                break;

            case AD_GAIN_CODE_0_1V:
                printf("0-1V\n");
                break;

            case AD_GAIN_CODE_1V:
                printf("+/-1V\n");
                break;

            default:
                printf("Unknown\n");
            }
      }

    printf("Calibration Mode:\t");

    switch(config->registers[AD_REGISTER_CAL_MODE]) {
      case AD_CAL_MODE_NORMAL:
          printf("Normal\n");
          break;

      case AD_CAL_MODE_GROUND:
          printf("Ground\n");
          break;

      case AD_CAL_MODE_REFERENCE:
          printf("Reference\n");
          break;

      case AD_CAL_MODE_BIP_GROUND:
          printf("BIP Reference\n");
          break;

      default:
          printf("Unknown\n");
      }

    printf("Trig/Counter clk\t");
    if (config->registers[AD_REGISTER_TRIG_COUNT] & AD_TRIGGER_CTR0_EXT) {
          printf("(counter)              ");
      }else {
          printf("(externally triggered) ");
      }

    if (config->registers[AD_REGISTER_TRIG_COUNT] & AD_TRIGGER_FALLING_EDGE) {
          printf("(triggered by falling edge) ");
      }

    if (config->registers[AD_REGISTER_TRIG_COUNT] & AD_TRIGGER_SCAN) {
          printf("(scan all channels) ");
      }else {
          printf("(single channel) ");
      }

    if (config->registers[AD_REGISTER_TRIG_COUNT] & AD_TRIGGER_EXTERNAL) {
          printf("(triggered by external ) ");
      }else if (config->registers[AD_REGISTER_TRIG_COUNT] & AD_TRIGGER_TIMER) {
          printf("(triggered by counter 2) ");
      }else
        printf("(triggered by sw       ) ");

    printf("\n");

    /* Display the Start end channels */

    printf("Channels:\tstart=%d, end=%d\n", config->registers[AD_CONFIG_START_END] & 0xF, config->registers[AD_CONFIG_START_END] >> 4);
}

/**
 * @param DeviceIndex
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_GetFastITScanV(unsigned long DeviceIndex, double *pData)
{
    unsigned long result = 0;

    int StartChannel;
    int EndChannel;
    int Channels;
    unsigned long BytesLeft;
    unsigned short *thisDataBuf;

    int bufsize;
    double clockHz = 0;
    double *pBuf;
    int numsleep = 100;
    double CLOCK_SPEED = 100000;
    int i, ch;

    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock(DeviceIndex, &result);

    if( !deviceDesc || result != AIOUSB_SUCCESS )
        goto out_ADC_GetFastITScanV;

    if(!deviceDesc->bADCStream) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto out_ADC_GetFastITScanV;
      }

    if(deviceDesc->ConfigBytes < 20) {
          result = AIOUSB_ERROR_BAD_TOKEN_TYPE;
          goto out_ADC_GetFastITScanV;
      }

    StartChannel  = AIOUSB_GetRegister(deviceDesc->FastITConfig, 0x12) & 0x0F;
    EndChannel    = AIOUSB_GetRegister(deviceDesc->FastITConfig, 0x12) >> 4;

    if( deviceDesc->ConfigBytes >= 21 ) {
        StartChannel = StartChannel  | ((AIOUSB_GetRegister(deviceDesc->FastITConfig, 20 ) & 0xF ) << 4);
        EndChannel   = EndChannel    | ( AIOUSB_GetRegister( deviceDesc->FastITConfig, 20 ) & 0xF0 );
    }


    Channels      = EndChannel - StartChannel + 1;

    /*  Result := ADC_BulkAcquire(DeviceIndex, Length(ADBuf) * SizeOf(ADBuf[0]), @ADBuf[0]); */

    CTR_8254Mode(DeviceIndex, 0, 2, 0);
    CTR_8254Mode(DeviceIndex, 0, 2, 1);
    bufsize = Channels * sizeof(unsigned short) * (AIOUSB_GetRegister(deviceDesc->FastITConfig, 0x13) + 1); /* 1 sample + 3 oversamples */;

    /* CLOCK_SPEED = 100000; */
    /* AIOUSB_SetStreamingBlockSize( DeviceIndex, 100000 ); */
    clockHz = 0;

    ADC_SetScanLimits(DeviceIndex, 0, Channels - 1);

    CLOCK_SPEED = 100000;       // Hz
    AIOUSB_SetStreamingBlockSize(DeviceIndex, 100000);
    /* dataBuf = ( unsigned short * ) malloc( bufsize ); */
    thisDataBuf = ( unsigned short* )malloc(bufsize + 100);
    memset(thisDataBuf, 0, bufsize + 100);

    clockHz = 0;
    CTR_StartOutputFreq(DeviceIndex, 0, &clockHz);
    ADC_ADMode(DeviceIndex, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER, AD_CAL_MODE_NORMAL);
    AIOUSB_SetMiscClock(DeviceIndex, CLOCK_SPEED);


    result = ADC_BulkAcquire(DeviceIndex, bufsize, thisDataBuf);

    if(result != AIOUSB_SUCCESS)
        goto CLEANUP_ADC_GetFastITScanV;

    BytesLeft = bufsize;
    numsleep = 0;
    usleep(0);
    while(BytesLeft) {
          result = ADC_BulkPoll(DeviceIndex, &BytesLeft);
          if(result != AIOUSB_SUCCESS) {
            /* #ifdef DEBUG */
            /*         printf( "Error '%s' polling bulk acquire progress\n", AIOUSB_GetResultCodeAsString( result ) ); */
            /* #endif */
                break;
            }else {
            /* #ifdef DEBUG */
            /*         printf( "  %lu bytes remaining\n", BytesLeft ); */
            /* #endif */
                numsleep++;
                usleep(10);
                if(numsleep > 100) {
                      result = AIOUSB_ERROR_TIMEOUT;
                      break;
                  }
            }
      }

    if(result != AIOUSB_SUCCESS)
        goto CLEANUP_ADC_GetFastITScanV;

    pBuf = pData;

    for(i = 0, ch = StartChannel; ch <= EndChannel; i++, ch++) {
          int RangeCode = AIOUSB_GetRegister(deviceDesc->FastITConfig, ch >> deviceDesc->RangeShift);
          int Tot = 0, Wt = 0;
          float V;
          int j;
          int numsamples = AIOUSB_GetRegister(deviceDesc->FastITConfig, 0x13) + 1;

          for(j = 1; j < numsamples; j++) {
                Tot += thisDataBuf[i * (numsamples) + j];
                Wt++;
          }
          V = Tot / Wt / (float)65536;
          if((RangeCode & 1) != 0)
              V = V * 2 - 1;
          if((RangeCode & 2) == 0)
              V = V * 2;
          if((RangeCode & 4) == 0)
              V = V * 5;

          *pBuf = (double)V;
          pBuf++;

          fflush(stdout);
      }

CLEANUP_ADC_GetFastITScanV:
    free(thisDataBuf);

out_ADC_GetFastITScanV:
    return result;
}

/**
 * @desc
 * @param DeviceIndex
 * @param pBuf
 *
 * @return
 */
unsigned long ADC_GetITScanV(unsigned long DeviceIndex,
                             double *pBuf
                             )
{
    unsigned result = ADC_InitFastITScanV(DeviceIndex);

    result = ADC_GetFastITScanV(DeviceIndex, pBuf);
    if(result != AIOUSB_SUCCESS)
        result = ADC_ResetFastITScanV(DeviceIndex);

    return result;
}


/**
 *
 *
 * @param DeviceIndex
 *
 * @return
 */
AIOUSB_BOOL AIOUSB_IsDiscardFirstSample(
    unsigned long DeviceIndex
    )
{
    AIOUSB_BOOL discard = AIOUSB_FALSE;

    if(!AIOUSB_Lock())
        return discard;

    if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS)
        discard = deviceTable[ DeviceIndex ].discardFirstSample;

    AIOUSB_UnLock();
    return discard;
}



/**
 *
 *
 * @param DeviceIndex
 * @param discard
 *
 * @return
 */
unsigned long AIOUSB_SetDiscardFirstSample(
    unsigned long DeviceIndex,
    AIOUSB_BOOL discard
    )
{
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result == AIOUSB_SUCCESS)
        deviceTable[ DeviceIndex ].discardFirstSample = discard;

    AIOUSB_UnLock();
    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param channel
 * @param counts
 *
 * @return
 */
double AIOUSB_CountsToVolts(
    unsigned long DeviceIndex,
    unsigned channel,
    unsigned short counts
    )
{
    double volts;

    if(AIOUSB_ArrayCountsToVolts(DeviceIndex, channel, 1, &counts, &volts) != AIOUSB_SUCCESS)
        volts = 0.0;
    return volts;
}



/**
 *
 *
 * @param DeviceIndex
 * @param startChannel
 * @param endChannel
 * @param counts
 *
 * @return
 */
unsigned long AIOUSB_MultipleCountsToVolts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const unsigned short counts[],     /* deviceDesc->ADCMUXChannels */
    double volts[]     /* deviceDesc->ADCMUXChannels */
    )
{
    return AIOUSB_ArrayCountsToVolts(DeviceIndex, startChannel, endChannel - startChannel + 1,
                                     counts + startChannel, volts + startChannel);
}



/**
 *
 *
 * @param DeviceIndex
 * @param channel
 * @param volts
 *
 * @return
 */
unsigned short AIOUSB_VoltsToCounts(
    unsigned long DeviceIndex,
    unsigned channel,
    double volts
    )
{
    unsigned short counts;

    if(AIOUSB_ArrayVoltsToCounts(DeviceIndex, channel, 1, &volts, &counts) != AIOUSB_SUCCESS)
        counts = 0;
    return counts;
}



/**
 *
 *
 * @param DeviceIndex
 * @param startChannel
 * @param endChannel
 * @param volts
 *
 * @return
 */
unsigned long AIOUSB_MultipleVoltsToCounts(
    unsigned long DeviceIndex,
    unsigned startChannel,
    unsigned endChannel,
    const double volts[],     /* deviceDesc->ADCMUXChannels */
    unsigned short counts[]     /* deviceDesc->ADCMUXChannels */
    )
{
    return AIOUSB_ArrayVoltsToCounts(DeviceIndex, startChannel, endChannel - startChannel + 1,
                                     volts + startChannel, counts + startChannel);
}



/**
 *
 *
 * @param DeviceIndex
 * @param fileName
 *
 * @return
 */
unsigned long AIOUSB_ADC_LoadCalTable(
    unsigned long DeviceIndex,
    const char *fileName
    )
{
    if(fileName == 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    AIOUSB_UnLock();
    unsigned short *const calTable = ( unsigned short* )malloc(CAL_TABLE_WORDS * sizeof(unsigned short));
    assert(calTable != 0);
    if(calTable != 0) {
          struct stat fileInfo;
          if(stat(fileName, &fileInfo) == 0) {
                if(fileInfo.st_size == CAL_TABLE_WORDS * sizeof(unsigned short)) {
                      FILE *const calFile = fopen(fileName, "r");
                      if(calFile != NULL) {
                            const size_t wordsRead = fread(calTable, sizeof(unsigned short), CAL_TABLE_WORDS, calFile);
                            fclose(calFile);
                            if(wordsRead == ( size_t )CAL_TABLE_WORDS)
                                result = AIOUSB_ADC_SetCalTable(DeviceIndex, calTable);
                            else
                                result = AIOUSB_ERROR_FILE_NOT_FOUND;
                        }else
                          result = AIOUSB_ERROR_FILE_NOT_FOUND;
                  }else
                    result = AIOUSB_ERROR_INVALID_DATA;              // file size incorrect
            }else
              result = AIOUSB_ERROR_FILE_NOT_FOUND;
          free(calTable);
      }else
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;

    return result;
}



/**
 *
 *
 * @param DeviceIndex
 * @param calTable
 *
 * @return
 */
unsigned long AIOUSB_ADC_SetCalTable(
    unsigned long DeviceIndex,
    const unsigned short calTable[]
    )
{
    if(calTable == 0)
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
/*
 * send calibration table to SRAM one block at a time; according to the documentation,
 * the proper procedure is to bulk transfer a block of calibration data to "endpoint 2"
 * and then send a control message to load it into the SRAM
 */
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                              // unlock while communicating with device
          const int SRAM_BLOCK_WORDS = 1024;       // can send 1024 words at a time to SRAM
          int sramAddress = 0;
          int wordsRemaining = CAL_TABLE_WORDS;
          while(wordsRemaining > 0) {
                const int wordsWritten
                    = (wordsRemaining < SRAM_BLOCK_WORDS)
                      ? wordsRemaining
                      : SRAM_BLOCK_WORDS;
                int bytesTransferred;
                const int libusbResult = AIOUSB_BulkTransfer(deviceHandle,
                                                             LIBUSB_ENDPOINT_OUT | USB_BULK_WRITE_ENDPOINT,
                                                             ( unsigned char* )(calTable + sramAddress), wordsWritten * sizeof(unsigned short),
                                                             &bytesTransferred, timeout);
                if(libusbResult != LIBUSB_SUCCESS) {
                      result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                      break;                                  // from while()
                  }else if(bytesTransferred != ( int )(wordsWritten * sizeof(unsigned short))) {
                      result = AIOUSB_ERROR_INVALID_DATA;
                      break;                                  // from while()
                  }else {
                      bytesTransferred = libusb_control_transfer(deviceHandle,
                                                                 USB_WRITE_TO_DEVICE, AUR_LOAD_BULK_CALIBRATION_BLOCK,
                                                                 sramAddress, wordsWritten, 0, 0, timeout);
                      if(bytesTransferred != 0) {
                            result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                            break;                            // from while()
                        }
                  }
                wordsRemaining -= wordsWritten;
                sramAddress += wordsWritten;
            }
      }else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}


/**
 *
 *
 * @param deviceIndex
 *
 * @return
 */
double GetHiRef(unsigned long deviceIndex)
{
    const double HiRefRef = 65130.249;          // == 9.938239V on 0-10V range (9.938239V / 10.0V * 65535 = 65130.249)
    unsigned short RefData = 0xFFFF;
    unsigned long DataSize = sizeof(RefData);
    unsigned long Status = GenericVendorRead(deviceIndex, 0xA2, 0x1DF2, 0, &DataSize, &RefData);

    if(Status != AIOUSB_SUCCESS)
        return HiRefRef;
    if(DataSize != sizeof(RefData))
        return HiRefRef;
    if((RefData == 0xFFFF) || (RefData == 0x0000))
        return HiRefRef;
    return RefData;
}

void
AIOUSB_Copy_Config_Block(ADConfigBlock *to, ADConfigBlock *from)
{
    to->device = from->device;
    to->size = from->size;
    memcpy(&to->registers[0], &from->registers[0], to->size);
}



unsigned long
AIOUSB_Validate_ADC_Device(unsigned long DeviceIndex)
{
    unsigned long result;

    if( (result = AIOUSB_Validate_Device( DeviceIndex ) ) != AIOUSB_SUCCESS )
        goto RETURN_AIOUSB_Validate_ADC_Device;
    
    result = ADC_QueryCal(DeviceIndex);

RETURN_AIOUSB_Validate_ADC_Device:
    AIOUSB_UnLock();
    return result;
}

/**
 * @desc Performs a number of ADC_GetImmediate calls and then averages out the values
 *       to determine adequate values for the Ground and Reference values
 * @param DeviceIndex
 * @param grounCounts
 * @param referenceCounts
 */
void
AIOUSB_Acquire_Reference_Counts(
    unsigned long DeviceIndex,
    double *groundCounts,
    double *referenceCounts
    )
{
    int reading;
    unsigned long result;
    double averageCounts;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    for(reading = 0; reading <= 1; reading++) {
          AIOUSB_Lock();
          AIOUSB_SetCalMode(&deviceDesc->cachedConfigBlock, (reading == 0) ? AD_CAL_MODE_GROUND : AD_CAL_MODE_REFERENCE);

          AIOUSB_UnLock(); /* unlock while communicating with device */
          result = WriteConfigBlock(DeviceIndex);

          if(result == AIOUSB_SUCCESS) {
            /*
             * average a bunch of readings to get a nice, stable
             * reading
             */
                const int AVERAGE_SAMPLES = 256;
                const unsigned MAX_GROUND = 0x00ff, MIN_REFERENCE = 0xf000;
                long countsSum = 0;
                int sample;
                unsigned short counts[ MAX_IMM_ADCS ];
                for(sample = 0; sample < AVERAGE_SAMPLES; sample++) {
                      result = ADC_GetImmediate(DeviceIndex, 0, counts);
                      if(result == AIOUSB_SUCCESS)
                          countsSum += counts[ 0 ];
                      else
                          goto RETURN_AIOUSB_GetBulkAcquire;
                  }

                averageCounts = countsSum / ( double )AVERAGE_SAMPLES;

                if(reading == 0) {
                      if(averageCounts <= MAX_GROUND)
                          *groundCounts = averageCounts;
                      else{
                            result = AIOUSB_ERROR_INVALID_DATA;
                            goto RETURN_AIOUSB_GetBulkAcquire;
                        }
                  }else {
                      if(
                          averageCounts >= MIN_REFERENCE &&
                          averageCounts <= AI_16_MAX_COUNTS
                          )
                          *referenceCounts = averageCounts;
                      else{
                            result = AIOUSB_ERROR_INVALID_DATA;
                            goto RETURN_AIOUSB_GetBulkAcquire;
                        }
                  }
            }
      }
RETURN_AIOUSB_GetBulkAcquire:
    return;
}




/**
 * @desc Loads the Cal table for Automatic internal calibration
 * @param calTable
 * @param DeviceIndex
 * @param groundCounts
 * @param referenceCounts
 * @return
 */
void DoLoadCalTable(
    unsigned short *const calTable,
    unsigned long DeviceIndex,
    double groundCounts,
    double referenceCounts
    )
{
    const double TARGET_GROUND_COUNTS = 0;      // == 0.0V on 0-10V range (0.0V / 10.0V * 65535 = 0.0)
    const double TARGET_REFERENCE_COUNTS = GetHiRef(DeviceIndex);
    const double slope
        = (TARGET_REFERENCE_COUNTS - TARGET_GROUND_COUNTS)
          / (referenceCounts - groundCounts);
    const double offset = TARGET_GROUND_COUNTS - slope * groundCounts;
    int index;

    for(index = 0; index < CAL_TABLE_WORDS; index++) {
          long value = ( long )round(slope * index + offset);
          if(value < 0)
              value = 0;
          else if(value > AI_16_MAX_COUNTS)
              value = AI_16_MAX_COUNTS;
          calTable[ index ] = ( unsigned short )value;
      }
}

/**
 * @desc 
 * @param config
 * @param channel
 * @param gainCode
 */
void AIOUSB_SetRangeSingle(ADConfigBlock *config, unsigned long channel, unsigned char gainCode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        gainCode >= FIRST_ENUM(ADGainCode) &&
        gainCode <= LAST_ENUM(ADGainCode) &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(
              channel < AD_MAX_CHANNELS &&
              channel < deviceDesc->ADCMUXChannels
              ) {
                assert(deviceDesc->ADCChannelsPerGroup != 0);
                const int reg = AD_CONFIG_GAIN_CODE + channel / deviceDesc->ADCChannelsPerGroup;
                assert(reg < AD_NUM_GAIN_CODE_REGISTERS);

                config->registers[ reg ] = gainCode;
            }
          AIOUSB_UnLock();
      }
}


/**
 * @desc Performs automatic calibration of the ADC
 * @param DeviceIndex
 * @param autoCal
 * @param returnCalTable
 * @param saveFileName
 * @return
 */
unsigned long AIOUSB_ADC_InternalCal(
    unsigned long DeviceIndex,
    AIOUSB_BOOL autoCal,
    unsigned short returnCalTable[],
    const char *saveFileName
    )
{
    int k;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bADCStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if((result = ADC_QueryCal(DeviceIndex)) != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    AIOUSB_UnLock();
    unsigned short *const calTable = ( unsigned short* )malloc(CAL_TABLE_WORDS * sizeof(unsigned short));
    assert(calTable != 0);
    if(!calTable) {
          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          goto INTERNAL_CAL_ERRORS;
      }

    if(autoCal) {
      /*
       * create calibrated calibration table
       */
          result = ReadConfigBlock(DeviceIndex, AIOUSB_FALSE);
          if(result == AIOUSB_SUCCESS) {
                AIOUSB_Lock();
                const ADConfigBlock origConfigBlock = deviceDesc->cachedConfigBlock;         // restore when done

                AIOUSB_SetAllGainCodeAndDiffMode(&deviceDesc->cachedConfigBlock, AD_GAIN_CODE_0_10V, AIOUSB_FALSE);
                AIOUSB_SetTriggerMode(&deviceDesc->cachedConfigBlock, 0);
                AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, 0, 0);
                AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, 0);
                /* ADC_Range1( DeviceIndex , 0x00 , 0x01, AIOUSB_FALSE ); */
                int rangeChannel = 0x00;
                int rangeValue = DAC_RANGE_10V;
                /* See page 21 of the USB manual */
                AIOUSB_SetCalMode(&deviceDesc->cachedConfigBlock, AD_CAL_MODE_BIP_GROUND);         // select bip low, to select
                AIOUSB_SetRangeSingle(&deviceDesc->cachedConfigBlock, rangeChannel, rangeValue);         // Select ??10 range for channel 0
                /* WriteConfigBlock( DeviceIndex ); */

                AIOUSB_UnLock();
                double groundCounts = 0, referenceCounts = 0;
                double averageCounts;
                int reading;

                for(reading = 0; reading <= 1; reading++) {
                      AIOUSB_Lock();
                      AIOUSB_SetCalMode(&deviceDesc->cachedConfigBlock, (reading == 0) ? AD_CAL_MODE_GROUND : AD_CAL_MODE_REFERENCE);

                      AIOUSB_UnLock();           // unlock while communicating with device
                      result = WriteConfigBlock(DeviceIndex);


                      if(result == AIOUSB_SUCCESS) {
                        /*
                         * average a bunch of readings to get a nice, stable reading
                         */
                            const int AVERAGE_SAMPLES = 256;
                            const unsigned MAX_GROUND = 0x00ff,
                                           MIN_REFERENCE = 0xf000;
                            long countsSum = 0;
                            int sample;
                            unsigned short counts[ MAX_IMM_ADCS ];
                            for(sample = 0; sample < AVERAGE_SAMPLES; sample++) {
                                  result = ADC_GetImmediate(DeviceIndex, 0, counts);
                                  if(result == AIOUSB_SUCCESS)
                                      countsSum += counts[ 0 ];
                                  else
                                      goto abort;
                              }
                            averageCounts = countsSum / ( double )AVERAGE_SAMPLES;
                            if(reading == 0) {
                                  if(averageCounts <= MAX_GROUND)
                                      groundCounts = averageCounts;
                                  else{
                                        result = AIOUSB_ERROR_INVALID_DATA;
                                        goto abort;
                                    }             /* if( averageCounts ...*/
                              }else {
                                  if(
                                      averageCounts >= MIN_REFERENCE &&
                                      averageCounts <= AI_16_MAX_COUNTS
                                      )
                                      referenceCounts = averageCounts;
                                  else{
                                        result = AIOUSB_ERROR_INVALID_DATA;
                                        goto abort;
                                    }
                              }
                      } else
                          goto abort;
                  }

abort:
                AIOUSB_Lock();
                deviceDesc->cachedConfigBlock = origConfigBlock;
                AIOUSB_UnLock();                    // unlock while communicating with device

                for(k = 0; k <= 1; k++) {
                      WriteConfigBlock(DeviceIndex);
                      /*result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );*/

                      AIOUSB_SetTriggerMode(&deviceDesc->cachedConfigBlock, AD_TRIGGER_SCAN); /* scan software start */
                      AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, 0, 0);              /* Select one channel */
                      AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, 0xff);             /* +255 oversample channel */

                      if(k == 0)
                          AIOUSB_SetCalMode(&deviceDesc->cachedConfigBlock, AD_CAL_MODE_BIP_GROUND);  /* select bip low, to select*/

                      WriteConfigBlock(DeviceIndex);


                      if(result != AIOUSB_SUCCESS)
                          goto INTERNAL_CAL_ERRORS;
                      /*
                       * we have good ground and reference readings;
                       * calculate table that makes ground reading
                       * equal to 0.0V and reference reading equal to
                       * 9.933939V; in order to compensate for an
                       * approximate 4.3 mV voltage drop across the
                       * primary MUX, we increase our target reference
                       * value by the same amount, yielding a new
                       * target of 9.933939V + 4.3 mV = 9.938239V =
                       * 65130.249 counts in unipolar mode
                       */

                      DoLoadCalTable(calTable, DeviceIndex, groundCounts, referenceCounts);

                      AIOUSB_Lock();
                      AIOUSB_SetTriggerMode(&deviceDesc->cachedConfigBlock, 0);
                      AIOUSB_SetAllGainCodeAndDiffMode(&deviceDesc->cachedConfigBlock, AD_GAIN_CODE_0_10V, AIOUSB_FALSE);
                      AIOUSB_SetScanRange(&deviceDesc->cachedConfigBlock, 0, 1);
                      AIOUSB_SetOversample(&deviceDesc->cachedConfigBlock, 0);

                      rangeValue = 0;
                      AIOUSB_SetRangeSingle(&deviceDesc->cachedConfigBlock, rangeChannel, rangeValue);


                      AIOUSB_UnLock();
                  }
            }
      }else {
      /*
       * create default (1:1) calibration table; that is, each output
       * word equals the input word
       */
          int index;
          for(index = 0; index < CAL_TABLE_WORDS; index++)
              calTable[ index ] = ( unsigned short )index;
      }

    if(result == AIOUSB_SUCCESS) {
      /*
       * optionally return calibration table to caller
       */
          if(returnCalTable != 0)
              memcpy(returnCalTable, calTable, CAL_TABLE_WORDS * sizeof(unsigned short));

          /*
           * optionally save calibration table to a file
           */
          if(saveFileName != 0) {
                FILE *const calFile = fopen(saveFileName, "w");
                if(calFile != NULL) {
                      const size_t wordsWritten = fwrite(calTable, sizeof(unsigned short), CAL_TABLE_WORDS, calFile);
                      fclose(calFile);
                      if(wordsWritten != ( size_t )CAL_TABLE_WORDS) {
                            remove(saveFileName);             // file is likely corrupt or incomplete
                            result = AIOUSB_ERROR_FILE_NOT_FOUND;
                        }
                  }else
                    result = AIOUSB_ERROR_FILE_NOT_FOUND;
            }

          /*
           * finally, send calibration table to device
           */
          result = AIOUSB_ADC_SetCalTable(DeviceIndex, calTable);
      }

    free(calTable);

INTERNAL_CAL_ERRORS:

    return result;
}

void
AIOUSB_SetRegister(ADConfigBlock *cb, unsigned int Register, unsigned char value)
{
    AIOUSB_Lock();
    cb->registers[Register] = value;
    AIOUSB_UnLock();
}


unsigned char
AIOUSB_GetRegister(ADConfigBlock *cb, unsigned int Register)
{
    unsigned char tmpval;

    AIOUSB_Lock();
    tmpval = cb->registers[Register];
    AIOUSB_UnLock();
    return tmpval;
}


/*
 * we have to lock some of these functions because they access the device table; we don't
 * have to lock functions that don't access the device table
 */

/**
 *
 *
 * @param config
 * @param DeviceIndex
 * @param defaults
 */
void AIOUSB_InitConfigBlock(ADConfigBlock *config, unsigned long DeviceIndex, AIOUSB_BOOL defaults)
{
    assert(config != 0);
    if(config != 0) {
/*
 * mark as uninitialized unless this function succeeds
 */
          config->device = 0;
          config->size = 0;
          if(AIOUSB_Lock()) {
                if(AIOUSB_Validate(&DeviceIndex) == AIOUSB_SUCCESS) {
                      const DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
                      config->device = deviceDesc;
                      config->size = deviceDesc->ConfigBytes;
                      assert(config->size == AD_CONFIG_REGISTERS ||
                             config->size == AD_MUX_CONFIG_REGISTERS);
                      if(defaults) {
                            AIOUSB_SetAllGainCodeAndDiffMode(config, AD_GAIN_CODE_0_10V, AIOUSB_FALSE);
                            AIOUSB_SetCalMode(config, AD_CAL_MODE_NORMAL);
                            AIOUSB_SetTriggerMode(config, 0);
                            AIOUSB_SetScanRange(config, 0, deviceDesc->ADCMUXChannels - 1);
                            AIOUSB_SetOversample(config, 0);
                        }
                  }
                AIOUSB_UnLock();
            }
      }
}


/**
 *
 *
 * @param config
 * @param gainCode
 * @param differentialMode
 */
void AIOUSB_SetAllGainCodeAndDiffMode(ADConfigBlock *config, unsigned gainCode, AIOUSB_BOOL differentialMode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        gainCode >= FIRST_ENUM(ADGainCode) &&
        gainCode <= LAST_ENUM(ADGainCode)
        ) {
          if(differentialMode)
              gainCode |= AD_DIFFERENTIAL_MODE;
          unsigned channel;
          for(channel = 0; channel < AD_NUM_GAIN_CODE_REGISTERS; channel++)
              config->registers[ AD_CONFIG_GAIN_CODE + channel ] = gainCode;
      }
}


/**
 * @desc
 * @param config
 * @param channel
 * @return
 */
unsigned AIOUSB_GetGainCode(const ADConfigBlock *config, unsigned channel)
{
    assert(config != 0);
    unsigned gainCode = FIRST_ENUM(ADGainCode);             // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(channel < AD_MAX_CHANNELS && channel < deviceDesc->ADCMUXChannels) {
                assert(deviceDesc->ADCChannelsPerGroup != 0);
                gainCode = (config->registers[ AD_CONFIG_GAIN_CODE + channel / deviceDesc->ADCChannelsPerGroup ]
                            & ( unsigned char )AD_GAIN_CODE_MASK
                            );
            }
          AIOUSB_UnLock();
      }
    return gainCode;
}


/**
 * @desc
 * @param config
 * @param channel
 * @param gainCode
 */
void AIOUSB_SetGainCode(ADConfigBlock *config, unsigned channel, unsigned gainCode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        VALID_ENUM(ADGainCode, gainCode) &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(channel < AD_MAX_CHANNELS && channel < deviceDesc->ADCMUXChannels) {
                assert(deviceDesc->ADCChannelsPerGroup != 0);
                const int reg = AD_CONFIG_GAIN_CODE + channel / deviceDesc->ADCChannelsPerGroup;
                assert(reg < AD_NUM_GAIN_CODE_REGISTERS);
                config->registers[ reg ]
                    = (config->registers[ reg ] & ~( unsigned char )AD_GAIN_CODE_MASK)
                      | ( unsigned char )(gainCode & AD_GAIN_CODE_MASK);
            }
          AIOUSB_UnLock();
      }
}


/**
 *
 * @param config
 * @param channel
 * @return
 */
AIOUSB_BOOL AIOUSB_IsDifferentialMode(const ADConfigBlock *config, unsigned channel)
{
    assert(config != 0);
    AIOUSB_BOOL differentialMode = AIOUSB_FALSE;
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(
              channel < AD_MAX_CHANNELS &&
              channel < deviceDesc->ADCMUXChannels
              ) {
                assert(deviceDesc->ADCChannelsPerGroup != 0);
                differentialMode
                    = (
                    (
                        config->registers[ AD_CONFIG_GAIN_CODE + channel / deviceDesc->ADCChannelsPerGroup ]
                        & ( unsigned char )AD_DIFFERENTIAL_MODE
                    ) != 0
                    )
                      ? AIOUSB_TRUE
                      : AIOUSB_FALSE;
            }
          AIOUSB_UnLock();
      }
    return differentialMode;
}


/**
 *
 *
 * @param config
 * @param channel
 * @param differentialMode
 */
void AIOUSB_SetDifferentialMode(ADConfigBlock *config, unsigned channel, AIOUSB_BOOL differentialMode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(
              channel < AD_MAX_CHANNELS &&
              channel < deviceDesc->ADCMUXChannels
              ) {
                assert(deviceDesc->ADCChannelsPerGroup != 0);
                const int reg = AD_CONFIG_GAIN_CODE + channel / deviceDesc->ADCChannelsPerGroup;
                assert(reg < AD_NUM_GAIN_CODE_REGISTERS);
                if(differentialMode)
                    config->registers[ reg ] |= ( unsigned char )AD_DIFFERENTIAL_MODE;
                else
                    config->registers[ reg ] &= ~( unsigned char )AD_DIFFERENTIAL_MODE;
            }
          AIOUSB_UnLock();
      }
}


/**
 *
 *
 * @param config
 *
 * @return
 */
unsigned AIOUSB_GetCalMode(const ADConfigBlock *config)
{
    assert(config != 0);
    unsigned calMode = AD_CAL_MODE_NORMAL;            // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        (
            config->registers[ AD_CONFIG_CAL_MODE ] == AD_CAL_MODE_NORMAL ||
            config->registers[ AD_CONFIG_CAL_MODE ] == AD_CAL_MODE_GROUND ||
            config->registers[ AD_CONFIG_CAL_MODE ] == AD_CAL_MODE_REFERENCE
        )
        ) {
          calMode = config->registers[ AD_CONFIG_CAL_MODE ];
      }
    return calMode;
}


/**
 *
 *
 * @param config
 * @param calMode
 */
void AIOUSB_SetCalMode(ADConfigBlock *config, unsigned calMode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        (
            calMode == AD_CAL_MODE_NORMAL ||
            calMode == AD_CAL_MODE_GROUND ||
            calMode == AD_CAL_MODE_REFERENCE
        )
        ) {
          config->registers[ AD_CONFIG_CAL_MODE ] = ( unsigned char )calMode;
      }
}


/**
 *
 *
 * @param config
 *
 * @return
 */
unsigned AIOUSB_GetTriggerMode(const ADConfigBlock *config)
{
    assert(config != 0);
    unsigned triggerMode = 0;                               // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0
        ) {
          triggerMode = config->registers[ AD_CONFIG_TRIG_COUNT ] & ( unsigned char )AD_TRIGGER_VALID_MASK;
      }
    return triggerMode;
}


/**
 *
 *
 * @param config
 * @param triggerMode
 */
void AIOUSB_SetTriggerMode(ADConfigBlock *config, unsigned triggerMode)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        (triggerMode & ~AD_TRIGGER_VALID_MASK) == 0
        ) {
          config->registers[ AD_CONFIG_TRIG_COUNT ] = ( unsigned char )triggerMode;
      }
}


/**
 *
 *
 * @param config
 *
 * @return
 */
unsigned AIOUSB_GetStartChannel(const ADConfigBlock *config)
{
    assert(config != 0);
    unsigned startChannel = 0;                              // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0
        ) {
          if(config->size == AD_MUX_CONFIG_REGISTERS)
              startChannel
                  = ((config->registers[ AD_CONFIG_MUX_START_END ] & 0x0f) << 4)
                    | (config->registers[ AD_CONFIG_START_END ] & 0xf);
          else
              startChannel = (config->registers[ AD_CONFIG_START_END ] & 0xf);
      }
    return startChannel;
}


/**
 *
 *
 * @param config
 *
 * @return
 */
unsigned AIOUSB_GetEndChannel(const ADConfigBlock *config)
{
    assert(config != 0);
    unsigned endChannel = 0;                                // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0
        ) {
          if(config->size == AD_MUX_CONFIG_REGISTERS)
              endChannel
                  = (config->registers[ AD_CONFIG_MUX_START_END ] & 0xf0)
                    | (config->registers[ AD_CONFIG_START_END ] >> 4);
          else
              endChannel = (config->registers[ AD_CONFIG_START_END ] >> 4);
      }
    return endChannel;
}


/**
 *
 *
 * @param config
 * @param startChannel
 * @param endChannel
 */
void AIOUSB_SetScanRange(ADConfigBlock *config, unsigned startChannel, unsigned endChannel)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        AIOUSB_Lock()
        ) {
          const DeviceDescriptor *const deviceDesc = ( DeviceDescriptor* )config->device;
          if(
              endChannel < AD_MAX_CHANNELS &&
              endChannel < deviceDesc->ADCMUXChannels &&
              startChannel <= endChannel
              ) {
                if(config->size == AD_MUX_CONFIG_REGISTERS) {
                  /*
                   * this board has a MUX, so support more channels
                   */
                      config->registers[ AD_CONFIG_START_END ] = ( unsigned char )((endChannel << 4) | (startChannel & 0x0f));
                      config->registers[ AD_CONFIG_MUX_START_END ] = ( unsigned char )((endChannel & 0xf0) | ((startChannel >> 4) & 0x0f));
                  } else {
                  /*
                   * this board doesn't have a MUX, so support base
                   * number of channels
                   */
                      config->registers[ AD_CONFIG_START_END ] = ( unsigned char )((endChannel << 4) | startChannel);
                  }
            }
          AIOUSB_UnLock();
      }
}


/**
 *
 *
 * @param config
 *
 * @return
 */
unsigned AIOUSB_GetOversample(const ADConfigBlock *config)
{
    assert(config != 0);
    unsigned overSample = 0;                                // return reasonable value on error
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0
        ) {
          overSample = config->registers[ AD_CONFIG_OVERSAMPLE ];
      }
    return overSample;
}



/**
 *
 *
 * @param config
 * @param overSample
 */
void AIOUSB_SetOversample(ADConfigBlock *config, unsigned overSample)
{
    assert(config != 0);
    if(
        config != 0 &&
        config->device != 0 &&
        config->size != 0 &&
        overSample <= 255
        ) {
          config->registers[ AD_CONFIG_OVERSAMPLE ] = ( unsigned char )overSample;
      }
}






#ifdef __cplusplus
}  // namespace AIOUSB
#endif


