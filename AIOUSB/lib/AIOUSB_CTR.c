/**
 * @file AIOUSB_CTR.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @copy
 * @brief Counter functionality
 */

#include "AIOUSB_CTR.h"


#include <math.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

#define RETURN_IF_INVALID_INPUT(d, r, f ) do { \
        if( !d )                                                        \
            return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_INDEX;            \
        if( ( r = f ) != AIOUSB_SUCCESS ) {                             \
            AIOUSB_UnLock();                                            \
            return r;                                                   \
        }                                                               \
    } while (0)
/*----------------------------------------------------------------------------*/
AIORET_TYPE _check_block_index(DeviceDescriptor *deviceDesc, unsigned long BlockIndex , unsigned long CounterIndex )
{
    if (BlockIndex == 0) {
        /* contiguous counter addressing*/
        BlockIndex = CounterIndex / COUNTERS_PER_BLOCK;
        CounterIndex = CounterIndex % COUNTERS_PER_BLOCK;
        if (BlockIndex >= deviceDesc->Counters) {
            AIOUSB_UnLock();
            return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
        }
    } else {
        if ( BlockIndex >= deviceDesc->Counters || CounterIndex >= COUNTERS_PER_BLOCK ) {
            AIOUSB_UnLock();
            return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
        }
    }
    return (AIORET_TYPE)AIOUSB_SUCCESS;
}
/*----------------------------------------------------------------------------*/
 AIORET_TYPE _check_valid_counters( DeviceDescriptor *deviceDesc ) {
     if ( deviceDesc->Counters == 0) {
         return (AIORET_TYPE)-AIOUSB_ERROR_NOT_SUPPORTED;
     }
     return AIOUSB_SUCCESS;
 }
/*----------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_counter_device(DeviceDescriptor *deviceDesc, 
                                        unsigned long BlockIndex,
                                        unsigned long CounterIndex
                                        )
{
    AIORET_TYPE temp;
    if( (temp = _check_valid_counters( deviceDesc )) != AIOUSB_SUCCESS )
        return temp;
    return _check_block_index( deviceDesc, BlockIndex, CounterIndex );
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_counter_device_with_mode(DeviceDescriptor *deviceDesc, 
                                                  unsigned long BlockIndex,
                                                  unsigned long CounterIndex,
                                                  unsigned long Mode
                                                  )
{
    if (Mode >= COUNTER_NUM_MODES)
        return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
    return _check_valid_counter_device( deviceDesc, BlockIndex, CounterIndex );
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_counter_device_for_read(DeviceDescriptor *deviceDesc, 
                                                 unsigned short *pData
                                                 )
{
    if ( !pData )
        return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
    return (AIORET_TYPE)AIOUSB_SUCCESS;
}
/*------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_counter_device_for_gate(DeviceDescriptor *deviceDesc, unsigned long GateIndex  ) 
{
    AIORET_TYPE retval;
    if( (retval = _check_valid_counters( deviceDesc )) != AIOUSB_SUCCESS ) 
        return retval;
    if ( deviceDesc->bGateSelectable == AIOUSB_FALSE ) {
        return (AIORET_TYPE)-AIOUSB_ERROR_NOT_SUPPORTED;
    }
    if (GateIndex >= deviceDesc->Counters * COUNTERS_PER_BLOCK) {
        return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
    }
    return AIOUSB_SUCCESS;
}
/*------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_counter_output_frequency( DeviceDescriptor *deviceDesc, unsigned long BlockIndex, double *pHz ) {
    AIORET_TYPE retval;
    if( (retval = _check_valid_counters( deviceDesc )) != AIOUSB_SUCCESS ) 
        return retval;
    if( BlockIndex >= deviceDesc->Counters || !pHz ) {
        return (AIORET_TYPE)-AIOUSB_ERROR_INVALID_PARAMETER;
    }
    return (AIORET_TYPE)AIOUSB_SUCCESS;
}
/*------------------------------------------------------------------------*/
AIORET_TYPE _check_valid_input_for_modeload( DeviceDescriptor *deviceDesc, 
                                             unsigned long BlockIndex, 
                                             unsigned long CounterIndex, 
                                             unsigned long Mode, 
                                             unsigned long LoadValue, 
                                             unsigned short *pReadValue
                                             )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if ( !pReadValue ) {
         return AIOUSB_ERROR_INVALID_PARAMETER;
    }
    if ( (retval = _check_valid_counters( deviceDesc )) != AIOUSB_SUCCESS ) 
        return retval;
    if ( (retval = _check_block_index( deviceDesc, BlockIndex, CounterIndex ) ) != AIOUSB_SUCCESS )
        return retval;

    return retval;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254Mode(
                           unsigned long DeviceIndex,
                           unsigned long BlockIndex,
                           unsigned long CounterIndex,
                           unsigned long Mode
                           ) {
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    if( !deviceDesc ) {
        return AIOUSB_ERROR_INVALID_INDEX;
    }
    if( (result = _check_valid_counter_device( deviceDesc, BlockIndex, CounterIndex )) != AIOUSB_SUCCESS ) {
        AIOUSB_UnLock();
        return result;
    }
    libusb_device_handle *usbHandle = AIOUSB_GetUSBHandle( deviceDesc );

    if (usbHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
          const unsigned short controlValue = (( unsigned short )CounterIndex << (6 + 8))  |
              (0x3u << (4 + 8))                                                            | 
              (( unsigned short )Mode << (1 + 8))                                          | 
              ( unsigned short )BlockIndex;
          const int bytesTransferred = libusb_control_transfer(usbHandle,
                                                               USB_WRITE_TO_DEVICE,
                                                               AUR_CTR_MODE,
                                                               controlValue,
                                                               0,
                                                               0,
                                                               0,
                                                               timeout
                                                               );
          if (bytesTransferred != 0)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
    } else {
        result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
        AIOUSB_UnLock();
    }
    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254Load(
                           unsigned long DeviceIndex,
                           unsigned long BlockIndex,
                           unsigned long CounterIndex,
                           unsigned short LoadValue
                           ) {
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    if( !deviceDesc ) {
        return AIOUSB_ERROR_INVALID_INDEX;
    }
    if( (result = _check_valid_counter_device( deviceDesc, BlockIndex, CounterIndex )) != AIOUSB_SUCCESS ) {
        AIOUSB_UnLock();
        return result;
    }
    libusb_device_handle *const usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
          const unsigned short controlValue
              = (( unsigned short )CounterIndex << (6 + 8))
                /* | ( 0x3u << ( 4 + 8 ) )*/
                /* | ( ( unsigned short ) Mode << ( 1 + 8 ) )*/
                | ( unsigned short )BlockIndex;
          const int bytesTransferred = libusb_control_transfer(usbHandle, 
                                                               USB_WRITE_TO_DEVICE, 
                                                               AUR_CTR_LOAD,
                                                               controlValue, 
                                                               LoadValue, 
                                                               0, 
                                                               0, 
                                                               timeout
                                                               );
          if (bytesTransferred != 0)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254ModeLoad(
                             unsigned long DeviceIndex,
                             unsigned long BlockIndex,
                             unsigned long CounterIndex,
                             unsigned long Mode,
                             unsigned short LoadValue
                             )
{
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_counter_device( deviceDesc, BlockIndex, CounterIndex ));
    libusb_device_handle *const usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
          const unsigned short controlValue
              = (( unsigned short )CounterIndex << (6 + 8))
                | (0x3u << (4 + 8))
                | (( unsigned short )Mode << (1 + 8))
                | ( unsigned short )BlockIndex;
          const int bytesTransferred = libusb_control_transfer( usbHandle, 
                                                                USB_WRITE_TO_DEVICE, 
                                                                AUR_CTR_MODELOAD,
                                                                controlValue, 
                                                                LoadValue, 
                                                                0, 
                                                                0, 
                                                                timeout
                                                                );
          if (bytesTransferred != 0)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
    } else {
        result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
        AIOUSB_UnLock();
    }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254ReadModeLoad(
                                   unsigned long DeviceIndex,
                                   unsigned long BlockIndex,
                                   unsigned long CounterIndex,
                                   unsigned long Mode,
                                   unsigned short LoadValue,
                                   unsigned short *pReadValue
                                   )
{
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_input_for_modeload( deviceDesc, BlockIndex, CounterIndex, Mode, LoadValue, pReadValue));
    libusb_device_handle *usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
   
    if (usbHandle != NULL) {
        const unsigned timeout = deviceDesc->commTimeout;
        AIOUSB_UnLock();
        unsigned short readValue;
        const unsigned short controlValue = (( unsigned short )CounterIndex << (6 + 8)) | 
            (0x3u << (4 + 8)) | 
            (( unsigned short )Mode << (1 + 8)) | 
            ( unsigned short )BlockIndex;

        const int bytesTransferred = libusb_control_transfer( usbHandle, 
                                                              USB_WRITE_TO_DEVICE, 
                                                              AUR_CTR_MODELOAD,
                                                              controlValue, 
                                                              LoadValue, 
                                                              ( unsigned char* )&readValue, 
                                                              sizeof(readValue), 
                                                              timeout
                                                              );
          if ( bytesTransferred == sizeof(readValue) ) {
              /* TODO: verify endian mode; original code had it reversed*/
                *pReadValue = readValue;
          } else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254Read(
                           unsigned long DeviceIndex,
                           unsigned long BlockIndex,
                           unsigned long CounterIndex,
                           unsigned short *pReadValue
                           )
{
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_counter_device( deviceDesc, BlockIndex, CounterIndex ) );

    libusb_device_handle *const usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
          unsigned short readValue;
          const unsigned short controlValue = (( unsigned short )CounterIndex << 8) | ( unsigned short )BlockIndex;
          const int bytesTransferred = libusb_control_transfer(usbHandle,
                                                               USB_READ_FROM_DEVICE,
                                                               AUR_CTR_READ,
                                                               controlValue,
                                                               0,
                                                               ( unsigned char* )&readValue,
                                                               sizeof(readValue),
                                                               timeout
                                                               );
          if (bytesTransferred == sizeof(readValue)) {
                                /* TODO: verify endian mode; original code had it reversed*/
                *pReadValue = readValue;
            } else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254ReadAll(
                              unsigned long DeviceIndex,
                              unsigned short *pData
                              ) {
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_counter_device_for_read( deviceDesc, pData ) );

    libusb_device_handle *usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
          const int READ_BYTES = deviceDesc->Counters * COUNTERS_PER_BLOCK * sizeof(unsigned short);
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
          const int bytesTransferred = libusb_control_transfer(usbHandle, 
                                                               USB_READ_FROM_DEVICE, 
                                                               AUR_CTR_READALL,
                                                               0, 
                                                               0, 
                                                               ( unsigned char* )pData, 
                                                               READ_BYTES, 
                                                               timeout
                                                               );
          if (bytesTransferred != READ_BYTES)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254ReadStatus(
                                 unsigned long DeviceIndex,
                                 unsigned long BlockIndex,
                                 unsigned long CounterIndex,
                                 unsigned short *pReadValue,
                                 unsigned char *pStatus
                                 ) {
    unsigned long result;
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_block_index( deviceDesc, BlockIndex, CounterIndex ) );

    libusb_device_handle *usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
           unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
           int READ_BYTES = 3;
          unsigned char readData[ READ_BYTES ];
           unsigned short controlValue = (( unsigned short )CounterIndex << 8) | ( unsigned short )BlockIndex;
           int bytesTransferred = libusb_control_transfer(usbHandle, 
                                                               USB_READ_FROM_DEVICE, 
                                                               AUR_CTR_READ,
                                                               controlValue, 
                                                               0, 
                                                               readData, 
                                                               READ_BYTES, 
                                                               timeout
                                                               );
          if (bytesTransferred == READ_BYTES) {
                *pReadValue = *( unsigned short* )readData;
                *pStatus = readData[ 2 ];
            } else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_StartOutputFreq(
                                  unsigned long DeviceIndex,
                                  unsigned long BlockIndex,
                                  double *pHz
                                  ) {
    unsigned long result;
    DeviceDescriptor * deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_counter_output_frequency( deviceDesc, BlockIndex, pHz ) );

    if (*pHz <= 0) {
                                /* turn off counters */
          AIOUSB_UnLock();
          result = CTR_8254Mode(DeviceIndex, BlockIndex, 1, 2);
          if (result != AIOUSB_SUCCESS)
              return result;
          result = CTR_8254Mode(DeviceIndex, BlockIndex, 2, 3);
          if (result != AIOUSB_SUCCESS)
              return result;
          *pHz = 0;                                                                   /* actual clock speed*/
      } else {
           long rootClock = deviceDesc->RootClock;
          AIOUSB_UnLock();
           long frequency = ( long )*pHz;
           long MIN_DIVISOR = 2;
           long MAX_DIVISOR = 65535;
          long bestHighDivisor = MIN_DIVISOR,
               bestLowDivisor = MIN_DIVISOR,
               minFreqError = 0;
          AIOUSB_BOOL minFreqErrorValid = AIOUSB_FALSE;
           long divisor = ( long )round(( double )rootClock / ( double )frequency);
#if defined(DEBUG_START_CLOCK)
          printf(
              "Calculating divisors (total divisor = %ld)\n"
              "  %8s  %8s  %8s\n",
              divisor, "High", "Low", "Error"
              );
#endif
          if (divisor > MIN_DIVISOR * MIN_DIVISOR) {
                long lowDivisor;
                for(lowDivisor = ( long )sqrt(divisor); lowDivisor >= MIN_DIVISOR; lowDivisor--) {
                      long highDivisor = divisor / lowDivisor;
                       long freqError = labs(frequency - rootClock / (highDivisor * lowDivisor));
#if defined(DEBUG_START_CLOCK)
                      printf("  %8ld  %8ld  %8ld\n", highDivisor, lowDivisor, freqError);
#endif
                      if (highDivisor > MAX_DIVISOR) {
                          /* this divisor would exceed the maximum; use best divisor calculated thus far*/
                            break;
                        } else if (freqError == 0) {
                          /* these divisors have no error; no need to continue searching for divisors*/
                            minFreqErrorValid = AIOUSB_TRUE;
                            minFreqError = freqError;
                            bestHighDivisor = highDivisor;
                            bestLowDivisor = lowDivisor;
                            break;
                        } else if (
                          !minFreqErrorValid ||
                          freqError < minFreqError
                          ) {
                            minFreqErrorValid = AIOUSB_TRUE;
                            minFreqError = freqError;
                            bestHighDivisor = highDivisor;
                            bestLowDivisor = lowDivisor;
                        }
                  }
            }
#if defined(DEBUG_START_CLOCK)
          printf("  %8ld  %8ld  %8ld (final)\n", bestHighDivisor, bestLowDivisor, minFreqError);
#endif
          result = CTR_8254ModeLoad(DeviceIndex, BlockIndex, 1, 2, ( unsigned short )bestHighDivisor);
          if (result != AIOUSB_SUCCESS)
              return result;
          result = CTR_8254ModeLoad(DeviceIndex, BlockIndex, 2, 3, ( unsigned short )bestLowDivisor);
          if (result != AIOUSB_SUCCESS)
              return result;
          *pHz = rootClock / (bestHighDivisor * bestLowDivisor);              /* actual clock speed*/
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254SelectGate(
    unsigned long DeviceIndex,
    unsigned long GateIndex
    )
{
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );
    RETURN_IF_INVALID_INPUT( deviceDesc, result, _check_valid_counter_device_for_gate(deviceDesc, GateIndex ) );

    libusb_device_handle * usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
           unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();
           int bytesTransferred = libusb_control_transfer(usbHandle, 
                                                               USB_WRITE_TO_DEVICE, 
                                                               AUR_CTR_SELGATE,
                                                               GateIndex, 
                                                               0, 
                                                               0, 
                                                               0 /* wLength */, 
                                                               timeout);
          if (bytesTransferred != 0)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE CTR_8254ReadLatched(
                                  unsigned long DeviceIndex,
                                  unsigned short *pData
                                  ) {
    unsigned long result;
    DeviceDescriptor *deviceDesc = DeviceTableAtIndex_Lock( DeviceIndex );

    libusb_device_handle * usbHandle = AIOUSB_GetUSBHandle( deviceDesc );
    if (usbHandle != NULL) {
        int READ_BYTES = deviceDesc->Counters * COUNTERS_PER_BLOCK * sizeof(unsigned short) + 1 ;/* for "old data" flag */
        unsigned timeout = deviceDesc->commTimeout;
        AIOUSB_UnLock();
        int bytesTransferred = libusb_control_transfer(usbHandle, 
                                                       USB_READ_FROM_DEVICE, 
                                                       AUR_CTR_READLATCHED,
                                                       0, 
                                                       0, 
                                                       ( unsigned char* )pData, 
                                                       READ_BYTES, 
                                                       timeout
                                                       );
        if (bytesTransferred != READ_BYTES)
            result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
    } else {
        result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
        AIOUSB_UnLock();
    }
    
    return result;
}



#ifdef __cplusplus
}
#endif
