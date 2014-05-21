/**
 * @file   AIOUSB_DIO.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief Core code to interface with Digital cards
 */

#include "AIOUSB_DIO.h"
#include "AIOUSB_Core.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
static unsigned short OctaveDacFromFreq(double *Hz) {
    assert(Hz != 0);
    unsigned short octaveOffset = 0;
    if(*Hz > 0) {
          if(*Hz > 40000000.0)
              *Hz = 40000000.0;
          int offset,
              octave = ( int )floor(3.322 * log10(*Hz / 1039));
          if(octave < 0) {
                octave = offset = 0;
            } else {
                offset = ( int )round(2048.0 - (ldexp(2078, 10 + octave) / *Hz));
                octaveOffset = (( unsigned short )octave << 12) | (( unsigned short )offset << 2);
                octaveOffset = htons(octaveOffset);         // oscillator wants the value in big-endian format
            }
          *Hz = (2078 << octave) / (2.0 - offset / 1024.0);
      }
    return octaveOffset;
}
/*----------------------------------------------------------------------------*/
unsigned long _check_init( unsigned long DeviceIndex ) {
    unsigned long result;
    if ( !AIOUSB_Lock() ) {
        return AIOUSB_ERROR_INVALID_MUTEX;
    }
    result = AIOUSB_Validate(&DeviceIndex);
    AIOUSB_UnLock();
    return result;
}
/*----------------------------------------------------------------------------*/
DeviceDescriptor *_check_dio( unsigned long DeviceIndex, unsigned long *result ) {
    DeviceDescriptor *deviceDesc = NULL;
    *result = _check_init( DeviceIndex );
    if ( *result != AIOUSB_SUCCESS ) {
        return deviceDesc;
    }
    deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->DIOBytes == 0) {
        AIOUSB_UnLock();
        *result = AIOUSB_ERROR_NOT_SUPPORTED;
    }
    return (DeviceDescriptor *)deviceDesc;
}
/*----------------------------------------------------------------------------*/
libusb_device_handle *_dio_get_device_handle( unsigned long DeviceIndex, DeviceDescriptor **deviceDesc,  unsigned long *result ) {
    libusb_device_handle *deviceHandle = NULL;
    *deviceDesc = _check_dio( DeviceIndex, result );

    if( *result != AIOUSB_SUCCESS ) {
        return deviceHandle;
    }
    if ( (*deviceDesc)->DIOBytes > 1000 ) { // Sanity check for decent values
        *result = AIOUSB_ERROR_INVALID_DATA;
        return deviceHandle;
    }

    deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);

    if( !deviceHandle ) {
        *result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
    }
    return deviceHandle;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_Configure(
                                          unsigned long DeviceIndex,
                                          unsigned char bTristate,
                                          AIOChannelMask *mask,
                                          DIOBuf *buf
                                          ) {
    libusb_device_handle *deviceHandle = NULL;
    DeviceDescriptor  *deviceDesc = NULL;
    unsigned long result;
    char *dest, *configBuffer;
    int maskBytes, bufferSize;
    int bytesTransferred;
    unsigned long retval;
    if ( !mask  || !buf  || ( bTristate != AIOUSB_FALSE && bTristate != AIOUSB_TRUE ) )
        return AIOUSB_ERROR_INVALID_PARAMETER;

    deviceDesc = _check_dio( DeviceIndex, &result );
    if ( !_dio_get_device_handle( DeviceIndex, &deviceDesc,  &result ) || result != AIOUSB_SUCCESS ) {
        goto out_DIO_Configure;
    }

    if ( deviceDesc->LastDIOData == 0) {
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        goto out_DIO_Configure;
    }
    deviceHandle = deviceDesc->deviceHandle;
    if( !deviceHandle ) { 
        result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
        goto out_DIO_Configure;
    }
    /* unsafe */
    memcpy(deviceDesc->LastDIOData, DIOBufToBinary(buf), DIOBufByteSize( buf ) );

    /* const int maskBytes = (deviceDesc->DIOBytes + BITS_PER_BYTE - 1) / BITS_PER_BYTE; */
    /* maskBytes = deviceDesc->DIOBytes; */
    /* maskBytes = AIOChannelMaskGetSize( mask ); */

    maskBytes = (deviceDesc->DIOBytes + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    bufferSize = deviceDesc->DIOBytes + maskBytes;
    configBuffer = ( char* )malloc( bufferSize );
    if ( !configBuffer ) {
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        goto out_DIO_Configure;
    }
    dest = configBuffer;
    /* memcpy(dest, pData, deviceDesc->DIOBytes); */
    memcpy( dest, DIOBufToBinary(buf), deviceDesc->DIOBytes );
    dest += deviceDesc->DIOBytes;
    /* memcpy(dest, pOutMask, maskBytes); */
    for( int i = 0; i < maskBytes ; i ++ ) {
        char tmpmask;
        if ( (retval = AIOChannelMaskGetMaskAtIndex( mask, &tmpmask, i ) != AIOUSB_SUCCESS ) ) { 
            return retval;
        }
        memcpy( dest, &tmpmask, 1 );
        /* memcpy( dest, tmpmask, 1 ); */
        dest += 1;
    }
    /* dest += maskBytes; */
    /* memset(dest, 0, maskBytes); */

    AIOUSB_UnLock();
    bytesTransferred = libusb_control_transfer(deviceHandle, 
                                               USB_WRITE_TO_DEVICE, 
                                               AUR_DIO_CONFIG,
                                               bTristate, 
                                               0, 
                                               (unsigned char *)configBuffer, 
                                               bufferSize, 
                                               deviceDesc->commTimeout
                                               );
    if (bytesTransferred != bufferSize )
        result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
    free(configBuffer);

out_DIO_Configure:
    AIOUSB_UnLock();
    return result;
}

PUBLIC_EXTERN unsigned long DIO_ConfigureRaw(
                                             unsigned long DeviceIndex,
                                             unsigned char bTristate,
                                             void *pOutMask,
                                             void *pData
                                             ) {
    if( !pOutMask  || !pData  || ( bTristate != AIOUSB_FALSE && bTristate != AIOUSB_TRUE ))
        return AIOUSB_ERROR_INVALID_PARAMETER;
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;
    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
        AIOUSB_UnLock();
        return result;
    }
    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->DIOBytes == 0) {
        AIOUSB_UnLock();
        return AIOUSB_ERROR_NOT_SUPPORTED;
    }
    if(deviceDesc->LastDIOData != 0) {
        assert(deviceDesc->DIOBytes <= 1000);       // arbitrary sanity check
        memcpy(deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes);
        libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
        if(deviceHandle != NULL) {
            const int maskBytes = (deviceDesc->DIOBytes + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
            const int bufferSize = deviceDesc->DIOBytes + 2 * maskBytes;
            unsigned char *const configBuffer = ( unsigned char* )malloc(bufferSize);
            assert(configBuffer != 0);
            if(configBuffer != 0) {
                unsigned char *dest = configBuffer;
                memcpy(dest, pData, deviceDesc->DIOBytes);
                dest += deviceDesc->DIOBytes;
                memcpy(dest, pOutMask, maskBytes);
                dest += maskBytes;
                memset(dest, 0, maskBytes);
                const unsigned timeout = deviceDesc->commTimeout;
                AIOUSB_UnLock();                                    // unlock while communicating with device
                const int bytesTransferred = libusb_control_transfer(deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_CONFIG,
                                                                     bTristate, 0, configBuffer, bufferSize, timeout);
                if(bytesTransferred != bufferSize)
                  result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                free(configBuffer);
            } else {
                result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                AIOUSB_UnLock();
            }
        } else {
            result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
            AIOUSB_UnLock();
        }
    } else {
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        AIOUSB_UnLock();
    }
    return result;
}


/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_ConfigureEx( 
                                            unsigned long DeviceIndex, 
                                            void *pOutMask, 
                                            void *pData, 
                                            void *pTristateMask 
                                             ) { 
    if( !pOutMask  || !pData || ! pTristateMask )
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
        deviceDesc->DIOBytes == 0 ||
        deviceDesc->Tristates == 0
        ) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->LastDIOData != 0) {
          assert(deviceDesc->DIOBytes <= 1000);       // arbitrary sanity check
          memcpy(deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes);
          libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
          if(deviceHandle != NULL) {
                const int maskBytes = (deviceDesc->DIOBytes + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
                const int tristateBytes = (deviceDesc->Tristates + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
                const int bufferSize = deviceDesc->DIOBytes + maskBytes + tristateBytes;
                unsigned char *const configBuffer = ( unsigned char* )malloc(bufferSize);
                assert(configBuffer != 0);
                if(configBuffer != 0) {
                      unsigned char *dest = configBuffer;
                      memcpy(dest, pData, deviceDesc->DIOBytes);
                      dest += deviceDesc->DIOBytes;
                      memcpy(dest, pOutMask, maskBytes);
                      dest += maskBytes;
                      memcpy(dest, pTristateMask, tristateBytes);
                      const int dioBytes = deviceDesc->DIOBytes;
                      const unsigned timeout = deviceDesc->commTimeout;
                      AIOUSB_UnLock();                                    // unlock while communicating with device
                      const int bytesTransferred = libusb_control_transfer(deviceHandle, USB_WRITE_TO_DEVICE, AUR_DIO_CONFIG,
                                                                           0, dioBytes, configBuffer, bufferSize, timeout);
                      if(bytesTransferred != bufferSize)
                          result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                      free(configBuffer);
                  } else {
                      result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                      AIOUSB_UnLock();
                  }
            } else {
                result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                AIOUSB_UnLock();
            }
      } else {
          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_ConfigurationQuery(
                                                   unsigned long DeviceIndex,
                                                   void *pOutMask,
                                                   void *pTristateMask
                                                   ) {
    if( !pOutMask || pTristateMask == NULL )
        return AIOUSB_ERROR_INVALID_PARAMETER;

    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->Tristates == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          assert(deviceDesc->DIOBytes <= 1000);       // arbitrary sanity check
          const int maskBytes = (deviceDesc->DIOBytes + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
          const int tristateBytes = (deviceDesc->Tristates + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
          const int bufferSize = maskBytes + tristateBytes;
          unsigned char *const configBuffer = ( unsigned char* )malloc(bufferSize);
          assert(configBuffer != 0);
          if(configBuffer != 0) {
                const int dioBytes = deviceDesc->DIOBytes;
                const unsigned timeout = deviceDesc->commTimeout;
                AIOUSB_UnLock();                                            // unlock while communicating with device
                const int bytesTransferred = libusb_control_transfer(deviceHandle, USB_READ_FROM_DEVICE, AUR_DIO_CONFIG_QUERY,
                                                                     0, dioBytes, configBuffer, bufferSize, timeout);
                if(bytesTransferred == bufferSize) {
                      memcpy(pOutMask, configBuffer, maskBytes);
                      memcpy(pTristateMask, configBuffer + maskBytes, tristateBytes);
                  } else
                    result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                free(configBuffer);
            } else {
                result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                AIOUSB_UnLock();
            }
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_WriteAll(
                                         unsigned long DeviceIndex,
                                         void *pData
                                         /* DIOBuf *data */
                                         ) {
    if( !pData )
        return AIOUSB_ERROR_INVALID_PARAMETER;
    int tmp = AIOUSB_Lock();
    if( !tmp ) {
          AIOUSB_UnLock();
          AIOUSB_Lock();
      }
    /* return AIOUSB_ERROR_INVALID_MUTEX; */

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->DIOBytes == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->LastDIOData != 0) {
          assert(deviceDesc->DIOBytes <= 1000);       // arbitrary sanity check
          char foo[10] = {};
          memcpy(foo, pData, deviceDesc->DIOBytes);
          memcpy(deviceDesc->LastDIOData, pData, deviceDesc->DIOBytes);

          libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
          if(deviceHandle != NULL) {
                const int dioBytes = deviceDesc->DIOBytes;
                const unsigned timeout = deviceDesc->commTimeout;
                AIOUSB_UnLock();                                            // unlock while communicating with device
                const int bytesTransferred = libusb_control_transfer(deviceHandle, 
                                                                     USB_WRITE_TO_DEVICE, 
                                                                     AUR_DIO_WRITE,
                                                                     0, 
                                                                     0, 
                                                                     ( unsigned char* )pData, 
                                                                     dioBytes, 
                                                                     timeout
                                                                     );
                if(bytesTransferred != dioBytes)
                    result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
            } else {
                result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                AIOUSB_UnLock();
            }
      } else {
          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          AIOUSB_UnLock();
      }

    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_Write8(
                                       unsigned long DeviceIndex,
                                       unsigned long ByteIndex,
                                       unsigned char Data
                                       ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->DIOBytes == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(ByteIndex >= deviceDesc->DIOBytes) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_INVALID_PARAMETER;
      }

    if(deviceDesc->LastDIOData != 0) {
          assert(deviceDesc->DIOBytes <= 1000);       // arbitrary sanity check
          deviceDesc->LastDIOData[ ByteIndex ] = Data;
          libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
          if(deviceHandle != NULL) {
                const int dioBytes = deviceDesc->DIOBytes;
                unsigned char *const dataBuffer = ( unsigned char* )malloc(dioBytes);
                assert(dataBuffer != 0);
                if(dataBuffer != 0) {
                      memcpy(dataBuffer, deviceDesc->LastDIOData, dioBytes);
                      const unsigned timeout = deviceDesc->commTimeout;
                      AIOUSB_UnLock();                                    // unlock while communicating with device
                      const int bytesTransferred = libusb_control_transfer(deviceHandle, 
                                                                           USB_WRITE_TO_DEVICE, 
                                                                           AUR_DIO_WRITE,
                                                                           0, 
                                                                           0, 
                                                                           dataBuffer, 
                                                                           dioBytes, 
                                                                           timeout
                                                                           );
                      if(bytesTransferred != dioBytes)
                          result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
                      free(dataBuffer);
                  } else {
                      result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
                      AIOUSB_UnLock();
                  }
            } else {
                result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
                AIOUSB_UnLock();
            }
      } else {
          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          AIOUSB_UnLock();
      }

    return result;
}


/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_Write1(
                                       unsigned long DeviceIndex,
                                       unsigned long BitIndex,
                                       unsigned char bData
                                       ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->DIOBytes == 0) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    const unsigned byteIndex = BitIndex / BITS_PER_BYTE;
    if(( bData != AIOUSB_FALSE && bData != AIOUSB_TRUE ) ||
       byteIndex >= deviceDesc->DIOBytes
       ) {
        AIOUSB_UnLock();
        return AIOUSB_ERROR_INVALID_PARAMETER;
    }

    if(deviceDesc->LastDIOData != 0) {
          unsigned char value = deviceDesc->LastDIOData[ byteIndex ];
          const unsigned char bitMask = 1 << (BitIndex % BITS_PER_BYTE);
          if(bData == AIOUSB_FALSE)
              value &= ~bitMask;
          else
              value |= bitMask;

          AIOUSB_UnLock();                                                    // unlock while communicating with device
          result = DIO_Write8(DeviceIndex, byteIndex, value);
      } else {
          result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
          AIOUSB_UnLock();
      }

    return result;
}


/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_ReadAll(
                                        unsigned long DeviceIndex,
                                        DIOBuf *buf
                                        ) {
    unsigned long result = AIOUSB_SUCCESS;
    DeviceDescriptor *deviceDesc = NULL;
    libusb_device_handle *deviceHandle = NULL;
    int bytesTransferred;
    char *tmpbuf;
    deviceHandle = _dio_get_device_handle( DeviceIndex, &deviceDesc,  &result );
    if ( !deviceHandle )
        goto out_DIO_ReadAll;
  
    if ( !buf ) {
        result = AIOUSB_ERROR_INVALID_PARAMETER;
        goto out_DIO_ReadAll;
    }

    tmpbuf = (char*)malloc( sizeof(char)*deviceDesc->DIOBytes );
    if ( !tmpbuf ) {
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        goto out_DIO_ReadAll;
    }
    AIOUSB_UnLock(); /* unlock while communicating with device */
    bytesTransferred = libusb_control_transfer(deviceHandle, 
                                               USB_READ_FROM_DEVICE, 
                                               AUR_DIO_READ,
                                               0, 
                                               0, 
                                               (unsigned char *)tmpbuf,
                                               deviceDesc->DIOBytes,
                                               deviceDesc->commTimeout
                                               );
    if( bytesTransferred < 0 || bytesTransferred != (int)deviceDesc->DIOBytes ) {
        result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
        goto cleanup_DIO_ReadAll;
    }

    if ( DIOBufResize( buf, deviceDesc->DIOBytes ) == NULL ) {
        result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup_DIO_ReadAll;
    }
    
    DIOBufReplaceString( buf, tmpbuf, deviceDesc->DIOBytes ); /* Copy to the DIOBuf */
    cleanup_DIO_ReadAll:
    free(tmpbuf);
    out_DIO_ReadAll:
    AIOUSB_UnLock();
    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_ReadAllToCharStr(
                                                 unsigned long DeviceIndex,
                                                 char *buf,
                                                 unsigned size
                                                 ) {
    unsigned long result = AIOUSB_SUCCESS;
    DeviceDescriptor *deviceDesc = NULL;
    libusb_device_handle *deviceHandle = NULL;

    deviceHandle = _dio_get_device_handle( DeviceIndex, &deviceDesc,  &result );
    if ( !deviceHandle ) {
        return result;
    }
    int bytes_to_transfer = MIN( size, deviceDesc->DIOBytes );
    AIOUSB_UnLock();                                                    // unlock while communicating with device
    int bytesTransferred = libusb_control_transfer(deviceHandle, 
                                                   USB_READ_FROM_DEVICE, 
                                                   AUR_DIO_READ,
                                                   0, 
                                                   0, 
                                                   (unsigned char *)buf,
                                                   bytes_to_transfer,
                                                   deviceDesc->commTimeout
                                                   );
    if( bytesTransferred < 0 || bytesTransferred != (int)deviceDesc->DIOBytes )
        result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);

    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_Read8(
                                      unsigned long DeviceIndex,
                                      unsigned long ByteIndex,
                                      int *pdat
                                      ) {
    unsigned long result = AIOUSB_SUCCESS;
    DeviceDescriptor *deviceDesc = NULL;
    DIOBuf *readBuffer;
    if ( !_dio_get_device_handle( DeviceIndex, &deviceDesc,  &result ) || result != AIOUSB_SUCCESS ) {
        goto out_DIO_Read8;
    }

    AIOUSB_UnLock();         // unlock while communicating with device
    readBuffer = NewDIOBuf( deviceDesc->DIOBytes );
    if ( !readBuffer ) {
        result =  AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        goto out_DIO_Read8;
    }
    
    if ( (result = DIO_ReadAll(DeviceIndex, readBuffer)) == AIOUSB_SUCCESS ) {
        char *tmp = DIOBufToBinary( readBuffer ); 
        *pdat = (int)tmp[ByteIndex];
    }

    DeleteDIOBuf( readBuffer );

 out_DIO_Read8:
    AIOUSB_UnLock();
    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_Read1(
                                      unsigned long DeviceIndex,
                                      unsigned long BitIndex,
                                      int *bit
                                      ) {
    char result = AIOUSB_SUCCESS;
    int value = 0;
    if((result = DIO_Read8(DeviceIndex, BitIndex / BITS_PER_BYTE, &value )) >= AIOUSB_SUCCESS) {
        const unsigned char bitMask = 1 << (( int )BitIndex % BITS_PER_BYTE);
        if((value & bitMask) != 0)
          *bit = AIOUSB_TRUE;
        else
          *bit = AIOUSB_FALSE;
    }
    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_StreamOpen(
                                           unsigned long DeviceIndex,
                                           unsigned long bIsRead
                                           ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bDIOStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->bDIOOpen) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_OPEN_FAILED;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                                                    // unlock while communicating with device
          const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                               USB_WRITE_TO_DEVICE,
                                                               bIsRead
                                                               ? AUR_DIO_STREAM_OPEN_INPUT
                                                               : AUR_DIO_STREAM_OPEN_OUTPUT,
                                                               0, 0, 0, 0, timeout);
          if(bytesTransferred == 0) {
                AIOUSB_Lock();
                deviceDesc->bDIOOpen = AIOUSB_TRUE;
                deviceDesc->bDIORead
                    = bIsRead
                      ? AIOUSB_TRUE
                      : AIOUSB_FALSE;
                AIOUSB_UnLock();
            } else
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}


/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_StreamClose(
                                            unsigned long DeviceIndex
                                            ) {
    if(!AIOUSB_Lock())
        return AIOUSB_ERROR_INVALID_MUTEX;

    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if(result != AIOUSB_SUCCESS) {
          AIOUSB_UnLock();
          return result;
      }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
    if(deviceDesc->bDIOStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->bDIOOpen == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_FILE_NOT_FOUND;
      }

    deviceDesc->bDIOOpen = AIOUSB_FALSE;

    AIOUSB_UnLock();
    return result;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_StreamSetClocks(
                                                unsigned long DeviceIndex,
                                                double *ReadClockHz,
                                                double *WriteClockHz
                                                ) {
    if(
        *ReadClockHz < 0 ||
        *WriteClockHz < 0
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
    if(deviceDesc->bDIOStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
      /**
       * @note  
       * @verbatim
       * fill in data for the vendor request
       * byte 0 used enable/disable read and write clocks
       *   bit 0 is write clock
       *   bit 1 is read  clock
       *     1 = off/disable
       *     0 = enable (1000 Khz is default value whenever enabled)
       * bytes 1-2 = write clock value
       * bytes 3-4 = read clock value
       * @endverbatim
       */
          const unsigned timeout = deviceDesc->commTimeout;
          AIOUSB_UnLock();                                                    // unlock while communicating with device
          const int CONFIG_BLOCK_SIZE = 5;
          unsigned char configBlock[ CONFIG_BLOCK_SIZE ];
          configBlock[ 0 ] = 0x03;                                    // disable read and write clocks by default
          if(*WriteClockHz > 0)
              configBlock[ 0 ] &= ~0x01;                            // enable write clock
          if(*ReadClockHz > 0)
              configBlock[ 0 ] &= ~0x02;                            // enable read clock
          *( unsigned short* )&configBlock[ 1 ] = OctaveDacFromFreq(WriteClockHz);
          *( unsigned short* )&configBlock[ 3 ] = OctaveDacFromFreq(ReadClockHz);
          const int bytesTransferred = libusb_control_transfer(deviceHandle,
                                                               USB_WRITE_TO_DEVICE, AUR_DIO_SETCLOCKS,
                                                               0, 0, configBlock, CONFIG_BLOCK_SIZE, timeout);
          if(bytesTransferred != CONFIG_BLOCK_SIZE)
              result = LIBUSB_RESULT_TO_AIOUSB_RESULT(bytesTransferred);
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}

/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN unsigned long DIO_StreamFrame(
                                            unsigned long DeviceIndex,
                                            unsigned long FramePoints,
                                            unsigned short *pFrameData,
                                            unsigned long *BytesTransferred
                                            ) {
    if(
        FramePoints == 0 ||
        pFrameData == NULL ||
        BytesTransferred == NULL
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
    if(deviceDesc->bDIOStream == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_NOT_SUPPORTED;
      }

    if(deviceDesc->bDIOOpen == AIOUSB_FALSE) {
          AIOUSB_UnLock();
          return AIOUSB_ERROR_FILE_NOT_FOUND;
      }

    libusb_device_handle *const deviceHandle = AIOUSB_GetDeviceHandle(DeviceIndex);
    if(deviceHandle != NULL) {
          const unsigned timeout = deviceDesc->commTimeout;
          const unsigned char endpoint
              = deviceDesc->bDIORead
                ? (LIBUSB_ENDPOINT_IN | USB_BULK_READ_ENDPOINT)
                : (LIBUSB_ENDPOINT_OUT | USB_BULK_WRITE_ENDPOINT);
          const int streamingBlockSize = ( int )deviceDesc->StreamingBlockSize * sizeof(unsigned short);
          AIOUSB_UnLock();                                                    // unlock while communicating with device

          /**
           * @note convert parameter types to those that libusb likes
           */
          unsigned char *data = ( unsigned char* )pFrameData;
          int remaining = ( int )FramePoints * sizeof(unsigned short);
          int total = 0;
          while(remaining > 0) {
                int bytes;
                const int libusbResult = AIOUSB_BulkTransfer(deviceHandle, endpoint, data,
                                                             (remaining < streamingBlockSize)
                                                             ? remaining
                                                             : streamingBlockSize,
                                                             &bytes, timeout);
                if(libusbResult == LIBUSB_SUCCESS) {
                      if(bytes > 0) {
                            total += bytes;
                            data += bytes;
                            remaining -= bytes;
                        }
                  } else {
                      result = LIBUSB_RESULT_TO_AIOUSB_RESULT(libusbResult);
                      break;                                                      // from while()
                  }
            }         // while( remaining ...
          if(result == AIOUSB_SUCCESS)
              *BytesTransferred = total;
      } else {
          result = AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
          AIOUSB_UnLock();
      }

    return result;
}


#ifdef __cplusplus
} // namespace AIOUSB
#endif

