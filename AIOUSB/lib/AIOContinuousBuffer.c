/**
 * @file   AIOContinuousBuffer.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief This file contains the required structures for performing the continuous streaming
 *        buffers that talk to ACCES USB-AI* cards. The functionality in this file was wrapped
 *        up to provide a more unified interface for continuous streaming of acquisition data and 
 *        to provide the user with a simplified system of reads for actually getting the streaming
 *        data. The role of the continuous mode is to just create a thread in the background that
 *        handles the low level USB transactions for collecting samples. This thread will fill up 
 *        a data structure known as the AIOContinuousBuf that is implemented as a fifo.
 *        
 * @todo Make the number of channels in the ContinuousBuffer match the number of channels in the
 *       config object
 */

#include "AIOUSB_Log.h"
#include "AIOContinuousBuffer.h"
#include "AIOBuf.h"
#include "ADCConfigBlock.h"
#include "AIOChannelMask.h"
#include "AIOUSB_Core.h"
#include "AIODeviceTable.h"
#include "AIOFifo.h"
#include "AIOCountsConverter.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

void *ConvertCountsToVoltsFunction( void *object );
void *RawCountsWorkFunction( void *object );
AIORET_TYPE _AIOContinuousBufResizeFifo( AIOContinuousBuf *buf );

/*-----------------------------  Constructors  -----------------------------*/
AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels )
{
    AIO_ASSERT_RET(NULL, num_channels > 0 );
    AIOContinuousBuf *tmp = NewAIOContinuousBuf(DeviceIndex, num_channels, 0, scancounts );
    AIO_ERROR_VALID_DATA( NULL, tmp );

    tmp->fifo  = (AIOFifoTYPE *)NewAIOFifoCounts( num_channels * scancounts );
    AIOContinuousBufSetDeviceIndex( tmp, DeviceIndex );

    AIOContinuousBufSetCallback( tmp, RawCountsWorkFunction );

    tmp->type = AIO_CONT_BUF_TYPE_COUNTS;
    tmp->PushN = AIOContinuousBufPushN;
    tmp->PopN  = AIOContinuousBufPopN;
    return tmp;
}

/*----------------------------------------------------------------------------*/
/**
 * @param deviceIndex 
 * @param num_channels 
 * @param num_oversamples 
 * @param base_size 
 * @brief Simplest constructor for the continuous mode buffer. It will by default use counts ( uint16_t ) as 
 *  the fundamental size/type (AIO_CONT_BUF_TYPE_COUNTS). 
 * @return 
 */
AIOContinuousBuf *NewAIOContinuousBuf( unsigned long deviceIndex, unsigned num_channels, unsigned num_oversamples, unsigned base_size )
{
    AIOContinuousBuf *tmp = (AIOContinuousBuf *)calloc(1,sizeof(AIOContinuousBuf));
    if ( tmp ) { 
        tmp->DeviceIndex      = deviceIndex;
        tmp->data_size        = 64*1024;
        tmp->hz               = 10000;
        tmp->timeout          = 1000;
        tmp->num_scans        = base_size;
        tmp->num_oversamples  = num_oversamples;
        tmp->base_size        = base_size;
        tmp->num_channels     = num_channels;
        tmp->unit_size        = sizeof(uint16_t);
#ifdef HAS_PTHREAD
        tmp->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif
        tmp->fifo = (AIOFifoTYPE *)NewAIOFifoCounts( tmp->num_channels *(tmp->num_oversamples+1)*tmp->base_size  );

        tmp->PushN = AIOContinuousBufPushN;
        tmp->PopN  = AIOContinuousBufPopN;
        tmp->type = AIO_CONT_BUF_TYPE_COUNTS;

#if 0
    if ( num_channels > 32 ) {
        char *bitstr = (char *)malloc( num_channels +1 );
        memset(bitstr, 49, num_channels ); /* Set all to 1s */
        bitstr[num_channels] = '\0';
        AIOChannelMaskSetMaskFromStr( tmp->mask, bitstr );
        free(bitstr);
    } else {
        AIOChannelMaskSetMaskFromInt( tmp->mask, (unsigned)-1 >> (BIT_LENGTH(unsigned)-num_channels) ); /**< Use all bits for each channel */
    }
#else
    tmp->mask = NewAIOChannelMask( num_channels );
#endif


    }
    return tmp;
}


/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetNumberOfChannels( AIOContinuousBuf * buf)
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->num_channels;
       
}

/*----------------------------------------------------------------------------*/
/**
 * @brief will set the number of channels that this AIOcontinuousbuf watches and if the number isn't
 *        divisibly into the total size of the fifo, the fifo gets resized
 */
 AIORET_TYPE AIOContinuousBufSetNumberOfChannels( AIOContinuousBuf * buf, unsigned num_channels )
{
    
    AIO_ASSERT_AIOCONTBUF( buf );
    buf->num_channels = num_channels;
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if ( (AIOFifoGetSizeNumElements( buf->fifo ) % num_channels) != 0 ) { 
        retval = _AIOContinuousBufResizeFifo( buf );
    }

    return retval;
}

/*----------------------------------------------------------------------------*/
 AIORET_TYPE AIOContinuousBufSetBaseSize( AIOContinuousBuf *buf , size_t newbase )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval  = AIOUSB_SUCCESS;
    if ( buf->base_size != newbase ) {
        buf->base_size = newbase;
        retval = _AIOContinuousBufResizeFifo( buf );
    }
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetBaseSize( AIOContinuousBuf *buf  )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->base_size;
}

/*----------------------------------------------------------------------------*/
AIOContinuousBuf *NewAIOContinuousBufForVolts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels, unsigned num_oversamples )
{
    AIO_ASSERT_RET(NULL, num_channels > 0 );

    AIOContinuousBuf *tmp = NewAIOContinuousBuf( DeviceIndex, num_channels,num_oversamples, scancounts );
    AIOContinuousBufSetUnitSize( tmp, sizeof(double));
    
    if ( tmp ) {
        AIOContinuousBufSetCallback( tmp, ConvertCountsToVoltsFunction );
        tmp->type = AIO_CONT_BUF_TYPE_VOLTS;
        tmp->PushN = AIOContinuousBufPushN;
        tmp->PopN  = AIOContinuousBufPopN;
        AIOFifoVoltsInitialize( (AIOFifoVolts*)tmp->fifo );
    }
    return tmp;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_InitConfiguration(  AIOContinuousBuf *buf ) 
{
    return AIOContinuousBufInitConfiguration( buf );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufPushN(AIOContinuousBuf *buf , void *frombuf, unsigned int N )
{
    return buf->fifo->PushN( buf->fifo, frombuf, N );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufPopN(AIOContinuousBuf *buf , void *frombuf, unsigned int N )
{
    return buf->fifo->PopN( buf->fifo, frombuf, N );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufInitADCConfigBlock( AIOContinuousBuf *buf, unsigned size, ADGainCode gainCode, AIOUSB_BOOL diffMode, unsigned char os, AIOUSB_BOOL dfs )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    AIOUSBDeviceSetTesting( AIODeviceTableGetDeviceAtIndex(  AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval ) , AIOContinuousBufGetTesting( buf ) );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );


    retval = ADCConfigBlockSetTesting( 
                                      AIOUSBDeviceGetADCConfigBlock( AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ) , (AIORESULT*)&retval ) ),
                                      AIOContinuousBufGetTesting( buf )
                                       );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    

    retval = ADCConfigBlockSetAIOUSBDevice( 
                                           AIOUSBDeviceGetADCConfigBlock( AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ) , (AIORESULT*)&retval ) ),
                                           AIODeviceTableGetDeviceAtIndex( 0, (AIORESULT*)&retval )
                                            );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );


    ADCConfigBlockSetSize( AIOUSBDeviceGetADCConfigBlock( AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ) , (AIORESULT*)&retval ) ),
                           size 
                           );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    AIOContinuousBufInitConfiguration( buf ); /* Needed to enforce Testing mode */
    AIOContinuousBufSetAllGainCodeAndDiffMode( buf, gainCode , diffMode );
    AIOContinuousBufSetOversample( buf, os );
    AIOContinuousBufSetDiscardFirstSample( buf, dfs );
    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Sets up an AIOContinuousBuffer to perform Internal , counter based 
 *        scanning. 
 * 
 * @param buf Our AIOContinuousBuffer
 * @return AIOUSB_SUCCESS if successful,  value < 0 if not.
 *
 */
AIORET_TYPE AIOContinuousBufInitConfiguration(  AIOContinuousBuf *buf ) 
{
    ADCConfigBlock config = {0};
    unsigned long tmp;
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    ADCConfigBlockInitForCounterScan( &config, deviceDesc );

    AIOContinuousBufSendPreConfig( buf );

    tmp = ADC_SetConfig( AIOContinuousBufGetDeviceIndex( buf ), config.registers, &config.size );
    AIO_ERROR_VALID_AIORET_TYPE( tmp, tmp == AIOUSB_SUCCESS );

        
    tmp = ADCConfigBlockCopy( AIOUSBDeviceGetADCConfigBlock( deviceDesc ), &config );
    AIO_ERROR_VALID_AIORET_TYPE( tmp, tmp == AIOUSB_SUCCESS );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SendPreConfig( AIOContinuousBuf *buf ) {
    return AIOContinuousBufSendPreConfig( buf );
}
AIORET_TYPE AIOContinuousBufSendPreConfig( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    unsigned wLength = 0x1, wIndex = 0x0, wValue = 0x0, bRequest = AUR_PROBE_CALFEATURE;
    int usbresult = 0;
    unsigned char data[1];
    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    if (  !buf->testing ) {
        usbresult = usb->usb_control_transfer( usb,
                                               USB_READ_FROM_DEVICE,
                                               bRequest,
                                               wValue,
                                               wIndex,
                                               data,
                                               wLength,
                                               buf->timeout
                                               );
        AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT( usbresult ), usbresult >= 0 );
    }

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Destructor for AIOContinuousBuf object
 */
AIORET_TYPE DeleteAIOContinuousBuf( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    if ( buf->mask )
        DeleteAIOChannelMask( buf->mask );
    if ( buf->buffer )
        free( buf->buffer );
    if ( buf->fifo  )
        DeleteAIOFifoCounts( (AIOFifoCounts *)buf->fifo );
    free( buf );
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetCountsBuffer( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetVoltsBuffer( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;
    
}


/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetStreamingBlockSize( AIOContinuousBuf *buf, unsigned blksize)
{
    if (!buf )
        return -AIOUSB_ERROR_INVALID_AIOCONTINUOUS_BUFFER;
    if ( blksize < 512 || blksize > 1024*64 ) { 
        buf->data_size = 512;
    } else {
        buf->data_size = ( blksize / 512 ) * 512;
    }
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetStreamingBlockSize( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->data_size;
}


/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) ) { return AIOContinuousBufSetCallback( buf, work );}
AIORET_TYPE AIOContinuousBufSetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT( work );
    AIOContinuousBufLock( buf );
    buf->callback = work;
    AIOContinuousBufUnlock( buf );
 
   return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetNumberScansToRead( AIOContinuousBuf *buf , unsigned num_scans )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    buf->num_scans = num_scans;
    return  AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetNumberScansToRead( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->num_scans;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_BufSizeForCounts( AIOContinuousBuf * buf) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOFifoGetSize( buf->fifo ) + AIOFifoGetRefSize(buf->fifo);
}

/*----------------------------------------------------------------------------*/
/**
 * @internal This is an internal function, don't use it externally as it 
 * will confuse you.
 *
 */
static unsigned write_size( AIOContinuousBuf *buf ) 
{
    unsigned retval = 0;
    unsigned read, write;
    read = (unsigned )AIOFifoReadPosition(buf->fifo);
    write = (unsigned)AIOFifoWritePosition(buf->fifo);
    if (  read > write ) {
        retval =  read - write;
    } else {
        return AIOFifoGetSize(buf->fifo) - (AIOFifoWritePosition (buf) - AIOFifoReadPosition (buf));
    }
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetRemainingWriteSize( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return write_size(buf);
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetUnitSize( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->unit_size;
}

 AIORET_TYPE AIOContinuousBufSetUnitSize( AIOContinuousBuf *buf , uint16_t new_unit_size)
{
    AIO_ASSERT_AIOCONTBUF( buf );
    buf->unit_size = new_unit_size;
    AIORET_TYPE retval = _AIOContinuousBufResizeFifo( buf );
    return retval;
}

AIORET_TYPE AIOContinuousBufReset( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIOContinuousBufLock( buf );
    AIOFifoReset( buf->fifo );
    AIOContinuousBufUnlock( buf );
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static unsigned write_size_num_scan_counts( AIOContinuousBuf *buf ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    float tmp = write_size(buf) / AIOContinuousBufNumberChannels(buf);
    if (  tmp > (int)tmp ) {
        tmp = (int)tmp;
    } else {
        tmp = ( tmp - 1 < 0 ? 0 : tmp -1 );
    }
    return (unsigned)tmp;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_NumberWriteScansInCounts( AIOContinuousBuf *buf ) { return AIOContinuousBufNumberWriteScansInCounts( buf ); }
AIORET_TYPE AIOContinuousBufNumberWriteScansInCounts( AIOContinuousBuf *buf ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE num_channels = AIOContinuousBufNumberChannels(buf);
    if ( num_channels < AIOUSB_SUCCESS )
        return num_channels;
    return num_channels*write_size_num_scan_counts( buf ) ;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->fifo->read_pos;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->fifo->write_pos;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetRemainingSize( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->fifo->delta( (AIOFifo*)buf->fifo );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetSize( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOFifoGetSize( buf->fifo );
}

AIORET_TYPE AIOContinuousBufGetSizeNumElements( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOFifoGetSizeNumElements( buf->fifo );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetStatus( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return (AIORET_TYPE)buf->status;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetExitCode( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->exitcode;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief returns the number of Scans accross all channels that still 
 *       remain in the buffer
 */
AIORET_TYPE AIOContinuousBufCountScansAvailable(AIOContinuousBuf *buf) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    retval = (AIORET_TYPE)buf->fifo->rdelta( (AIOFifo*)buf->fifo ) / ( buf->fifo->refsize * AIOContinuousBufNumberChannels(buf) );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufCountsAvailable(AIOContinuousBuf *buf) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    retval = (AIORET_TYPE)buf->fifo->rdelta( (AIOFifo*)buf->fifo ) / ( buf->fifo->refsize * AIOContinuousBufNumberChannels(buf) );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetDataAvailable( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief will read in an integer number of scan counts if there is room.
 * @param buf 
 * @param tmp 
 * @param size The size of the tmp buffer
 * @return 
 */
AIORET_TYPE AIOContinuousBufReadIntegerScanCounts( AIOContinuousBuf *buf, 
                                                   unsigned short *read_buf , 
                                                   unsigned tmpbuffer_size, 
                                                   unsigned size
                                                   )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int num_scans;
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT( read_buf );
    AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_NOT_ENOUGH_MEMORY, size >= (unsigned)AIOContinuousBufNumberChannels(buf) );

    AIOContinuousBufLock( buf );    
    num_scans = AIOContinuousBufCountScansAvailable( buf );

    retval += buf->fifo->PopN( buf->fifo, read_buf, num_scans*AIOContinuousBufNumberChannels(buf) );
    retval /= AIOContinuousBufNumberChannels(buf);
    retval /= buf->fifo->refsize;

    AIOContinuousBufUnlock( buf );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufReadIntegerGetNumberOfScans( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->num_scans;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufReadIntegerSetNumberOfScans( AIOContinuousBuf *buf, unsigned num_scans )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    buf->num_scans = num_scans;
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief will read in an integer number of scan counts if there is room.
 * @param buf 
 * @param tmp 
 * @param size The size of the tmp buffer
 * @return 
 */
AIORET_TYPE AIOContinuousBufReadIntegerNumberOfScans( AIOContinuousBuf *buf, 
                                                      unsigned short *read_buf , 
                                                      unsigned tmpbuffer_size, 
                                                      size_t num_scans
                                                      )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int debug = 0;
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_NOT_ENOUGH_MEMORY, tmpbuffer_size >= (unsigned)( AIOContinuousBufNumberChannels(buf)*num_scans ) );

    for ( int i = 0, pos = 0;  i < (int)num_scans && (int)( pos + AIOContinuousBufNumberChannels(buf)-1 ) < (int)tmpbuffer_size ; i++ , pos += AIOContinuousBufNumberChannels(buf) ) {
        if (  i == 0 )
            retval = AIOUSB_SUCCESS;
        if (  debug ) { 
            printf("Using i=%d\n",i );
        }
        retval += AIOContinuousBufRead( buf, (AIOBufferType *)&read_buf[pos] , tmpbuffer_size - pos, AIOContinuousBufNumberChannels(buf) );
        retval /= AIOContinuousBufNumberChannels(buf);
    }

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufReadSingle( AIOContinuousBuf *buf, AIOBuf *tobuf, size_t  size_to_read )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    return retval;

}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufReadCompleteScanCounts( AIOContinuousBuf *buf, 
                                                    unsigned short *read_buf, 
                                                    unsigned read_buf_size
                                                    )
{

    AIORET_TYPE retval = AIOContinuousBufReadIntegerScanCounts( buf, 
                                                                read_buf, 
                                                                read_buf_size, 
                                                                MIN((int)read_buf_size, (int)AIOContinuousBufCountScansAvailable(buf)*AIOContinuousBufNumberChannels(buf) )
                                                                );
    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Returns 
 * @param buf 
 * @return Pointer to our work function
 */
AIOUSB_WorkFn AIOContinuousBufGetCallback( AIOContinuousBuf *buf )
{
    AIO_ASSERT_RET( NULL, buf );
    return buf->callback;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    buf->hz = MIN( (unsigned)hz, (unsigned)(ROOTCLOCK / ( (AIOContinuousBufGetOversample(buf)+1) * AIOContinuousBufNumberChannels(buf))) );

    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Starts the thread that acquires data from USB bus.  
 * @param buf 
 * @param work 
 * @return status code of start.
 */
AIORET_TYPE AIOContinuousBufStart( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
#ifdef HAS_PTHREAD
    buf->status = RUNNING;
#ifdef HIGH_PRIORITY            /* Must run as root if you use this */
    int fifo_max_prio;
    struct sched_param fifo_param;
    pthread_attr_t custom_sched_attr;
    pthread_attr_init( &custom_sched_attr ) ;
    pthread_attr_setinheritsched(&custom_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&custom_sched_attr, SCHED_RR);
    fifo_max_prio = sched_get_priority_max(SCHED_RR);
    fifo_param.sched_priority = fifo_max_prio;
    pthread_attr_setschedparam( &custom_sched_attr, &fifo_param);
    retval = pthread_create( &(buf->worker), &custom_sched_attr, buf->callback, (void *)buf );
#else
    retval = pthread_create( &(buf->worker), NULL, buf->callback, (void *)buf );
#endif
    if (  retval != 0 ) {
        buf->status = TERMINATED;
        AIOUSB_ERROR("Unable to create thread for Continuous acquisition");
        return -1;
    }
#endif  

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Calculates the register values for buf->divisora, and buf->divisorb to create
 * an output clock that matches the value stored in buf->hz
 * @param buf AIOContinuousBuf object that we will be reading data into
 * @return Success(0) or failure( < 0 ) if we can't set the clocks
 */
AIORET_TYPE CalculateClocks( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    int  hz = (int)buf->hz;
    float l;

    int divisora, divisorb, divisorab;
    int min_err, err;

    if (  hz == 0 ) {
        return -AIOUSB_ERROR_INVALID_PARAMETER;
    }
    if (   hz * 4 >= ROOTCLOCK ) {
        divisora = 2;
        divisorb = 2;
    } else { 
        divisorab = ROOTCLOCK / hz;
        l = sqrt( divisorab );
        if ( l > 0xffff ) { 
            divisora = 0xffff;
            divisorb = 0xffff;
            min_err  = abs((int)(round(((ROOTCLOCK / hz) - (int)(divisora * l)))));
        } else  { 
            divisora  = round( divisorab / l );
            l         = round(sqrt( divisorab ));
            divisorb  = l;

            min_err = abs(((ROOTCLOCK / hz) - (int)(divisora * l)));
      
            for ( unsigned lv = l ; lv >= 2 ; lv -- ) {
                unsigned olddivisora = (int)round((double)divisorab / lv);
                if (  olddivisora > 0xffff ) { 
                    AIOUSB_DEVEL( "Found value > 0xff..resetting" );
                    break;
                } else { 
                    divisora = olddivisora;
                }

                err = abs((int)((ROOTCLOCK / hz) - (divisora * lv)));
                if (  err <= 0  ) {
                    min_err = 0;
                    AIOUSB_DEVEL("Found zero error: %d\n", lv );
                    divisorb = lv;
                    break;
                } 
                if (  err < min_err  ) {
                    AIOUSB_DEVEL( "Found new error: using lv=%d\n", (int)lv);
                    divisorb = lv;
                    min_err = err;
                }
                divisora = (int)round(divisorab / divisorb);
            }
        }
    }
    buf->divisora = divisora;
    buf->divisorb = divisorb;
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = pthread_create( &(buf->worker), NULL , callback, (void *)buf  );

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Sets the channel mask
 * @param buf 
 * @param mask 
 * @return 
 */
AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask ) { return AIOContinuousBufSetChannelMask( buf, mask ); }
AIORET_TYPE AIOContinuousBufSetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT(mask);
    buf->mask   = mask;
    buf->extra  = 0;
    return 0;
}

AIORET_TYPE AIOContinuousBuf_NumberSignals( AIOContinuousBuf *buf ) { return AIOContinuousBufNumberSignals( buf ); }
AIORET_TYPE AIOContinuousBufNumberSignals( AIOContinuousBuf *buf )
{

    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOChannelMaskNumberSignals(buf->mask );
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_NumberChannels( AIOContinuousBuf *buf ) { return AIOContinuousBufNumberChannels(buf); }
AIORET_TYPE AIOContinuousBufNumberChannels( AIOContinuousBuf *buf ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOChannelMaskNumberSignals(buf->mask );
}

/*----------------------------------------------------------------------------*/
/**
 * @brief A simple copy of one ushort buffer to one of AIOBufferType and converts
 *       counts to voltages
 * @param buf 
 * @param channel 
 * @param data 
 * @param count 
 * @param tobuf 
 * @param pos 
 * @return retval the number of data elements that were written to the tobuf
 */
AIORET_TYPE AIOContinuousBuf_SmartCountsToVolts( AIOContinuousBuf *buf,  
                                                 unsigned *channel,
                                                 unsigned short *data, 
                                                 unsigned count,  
                                                 AIOBufferType *tobuf, 
                                                 unsigned *pos )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT( channel );
    AIO_ASSERT( data );
    AIO_ASSERT( tobuf );
    AIO_ASSERT( pos );

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    int number_channels = AIOContinuousBufNumberChannels(buf);


    for (unsigned ch = 0; ch < count;  ch ++ , *channel = ((*channel+1)% number_channels ) , *pos += 1 ) {
        int gain = ADCConfigBlockGetGainCode( &deviceDesc->cachedConfigBlock, *channel );
        AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_INVALID_GAINCODE, gain >= AIOUSB_SUCCESS );
        struct ADRange *range = &adRanges[ gain ];
        tobuf[ *pos ] = ( (( double )data[ ch ] / ( double )AI_16_MAX_COUNTS) * range->range ) + range->minVolts;
        retval += 1;
    }

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief only write the number of elements. size is calculated
 * automatically based on the underlying type
 */
AIORET_TYPE _AIOContinuousBufWrite( AIOContinuousBuf *buf , void *input, size_t size )
{
    AIO_ASSERT( buf );
    AIO_ASSERT( input );

    buf->fifo->PushN( buf->fifo, input, size );

    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE _AIOContinuousBufRead( AIOContinuousBuf *buf , void *tobuf, size_t size )
{
    AIO_ASSERT( buf );
    AIO_ASSERT( tobuf );
    AIORET_TYPE retval;

    retval = buf->fifo->PopN( buf->fifo, tobuf, (int)size );

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Allows one to write into the AIOContinuousBuf buffer a given amount (size) of data.
 * @param buf 
 * @param writebuf 
 * @param size 
 * @param flag
 * @return Status of whether the write was successful , if so returning the number of bytes written
 *         or if there was insufficient space, it returns negative error code. If the number 
 *         is >= 0, then this corresponds to the number of bytes that were written into the buffer.
 */
AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, 
                                   AIOBufferType *writebuf, 
                                   unsigned wrbufsize, 
                                   unsigned size, 
                                   AIOContinuousBufMode flag )
{
    AIORET_TYPE retval;
    ERR_UNLESS_VALID_ENUM( AIOContinuousBufMode ,  flag );

    AIOContinuousBufLock( buf );
    int N = size / (buf->fifo->refsize );

    retval = buf->fifo->PushN( buf->fifo, writebuf, N );
    retval = ( retval == 0 ? -AIOUSB_ERROR_NOT_ENOUGH_MEMORY : retval );

    AIOContinuousBufUnlock( buf );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufWriteCounts( AIOContinuousBuf *buf, unsigned short *data, unsigned datasize, unsigned size , AIOContinuousBufMode flag )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOContinuousBufLock( buf );
    retval += buf->fifo->PushN( buf->fifo, data, size / sizeof(unsigned short));
    AIOContinuousBufUnlock( buf );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE aiocontbuf_get_data( AIOContinuousBuf *buf, 
                                 USBDevice *usb, 
                                 unsigned char endpoint, 
                                 unsigned char *data,
                                 int datasize,
                                 int *bytes,
                                 unsigned timeout 
                                 )
{
    AIORET_TYPE usbresult;

    usbresult = usb->usb_bulk_transfer( usb,
                                        0x86,
                                        data,
                                        datasize,
                                        bytes,
                                        timeout
                                        );

    return usbresult;
}


/*----------------------------------------------------------------------------*/
void *RawCountsWorkFunction( void *object )
{
    static AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
    AIO_ASSERT_RET( NULL, object );
    int usbfail = 0, usbfail_count = 5;
    unsigned count = 0;
    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_DATA( &retval, retval == AIOUSB_SUCCESS );


    unsigned char *data  = (unsigned char *)malloc( buf->data_size );

    while ( buf->status == RUNNING  ) {
        int bytes;
        int usbresult = aiocontbuf_get_data( buf, usb, 0x86, data, buf->data_size, &bytes, 3000 );

        AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , (int)bytes);

        if (  bytes ) {
            bytes = ( AIOContinuousBuf_BufSizeForCounts(buf) - buf->fifo->refsize - count < buf->data_size ? AIOContinuousBuf_BufSizeForCounts(buf) - buf->fifo->refsize - count : bytes );

            int tmp = buf->fifo->PushN( buf->fifo, (uint16_t*)data, bytes / sizeof(unsigned short));

            AIOUSB_DEVEL("Pushed %d, size: %d\n", bytes / 2 , buf->fifo->size );

            if (  tmp >= 0 ) {
                count += tmp;
            }

            AIOUSB_DEVEL("Tmpcount=%d,count=%d,Bytes=%d, Write=%d,Read=%d,max=%d\n", tmp,count,bytes,AIOFifoWritePosition(buf) , AIOFifoReadPosition(buf), AIOFifoGetSize(buf));

            /**
             * Modification, allow the count to keep going... stop 
             * if 
             * 1. count >= number we are supposed to read
             * 2. we don't have enough space
             */
            if (  count >= (unsigned)AIOContinuousBuf_BufSizeForCounts(buf) - AIOContinuousBufNumberChannels(buf) ) {

                AIOContinuousBufLock(buf);
                buf->status = TERMINATED;
                AIOContinuousBufUnlock(buf);
            }
        } else if (  usbresult < 0  && usbfail < usbfail_count ) {
            AIOUSB_ERROR("Error with usb: %d\n", (int)usbresult );
            usbfail ++;
        } else {
            if (  usbfail >= usbfail_count  ) {
                AIOUSB_ERROR("Erroring out. too many usb failures: %d\n", usbfail_count );
                retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
                AIOContinuousBufLock(buf);
                buf->status = TERMINATED;
                AIOContinuousBufUnlock(buf);
                buf->exitcode = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
            } 
        }
    }

    AIOContinuousBufLock(buf);
    buf->status = TERMINATED;
    AIOContinuousBufUnlock(buf);
    AIOUSB_DEVEL("Stopping\n");
    AIOContinuousBufCleanup( buf );
    free(data);
    pthread_exit((void*)&retval);
  
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Main work function for collecting data. Also performs copies from 
 *       the raw acquiring buffer into the AIOContinuousBuf
 * @param object 
 * @return 
 * @todo Ensure that copying matches the actual size of the data
 */
void *ConvertCountsToVoltsFunction( void *object )
{
    static AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIO_ERROR_VALID_DATA_W_CODE( &retval, retval = AIOUSB_ERROR_INVALID_PARAMETER, object );

    AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
    AIOGainRange *ranges;
    int usbfail = 0, usbfail_count = 5;
    unsigned count = 0;
    int num_channels = AIOContinuousBufNumberChannels(buf);
    int num_oversamples = AIOContinuousBufGetOversample(buf);
    int num_scans = AIOContinuousBufGetNumberScansToRead(buf);
    AIOFifoCounts *infifo = NewAIOFifoCounts( (unsigned)num_channels*(num_oversamples+1)*num_scans );
    AIO_ERROR_VALID_DATA_W_CODE( &retval, retval = AIOUSB_ERROR_INVALID_AIOFIFO, infifo );
    AIOFifoVolts *outfifo = (AIOFifoVolts*)buf->fifo;

    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_DATA( &retval, retval == AIOUSB_SUCCESS );


    AIOCountsConverter *cc;
    AIOUSBDevice *dev = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_DATA( &retval, retval == AIOUSB_SUCCESS );

    ranges = NewAIOGainRangeFromADCConfigBlock( AIOUSBDeviceGetADCConfigBlock( dev ) );
    AIO_ERROR_VALID_DATA_W_CODE( &retval, retval = AIOUSB_ERROR_INVALID_GAINCODE, ranges );

    unsigned char *data   = (unsigned char *)malloc( buf->data_size );
    AIO_ERROR_VALID_DATA_W_CODE( &retval, retval = AIOUSB_ERROR_NOT_ENOUGH_MEMORY, data );

    cc = NewAIOCountsConverterWithScanLimiter( (unsigned short*)data, num_scans, num_channels, ranges, num_oversamples , sizeof(unsigned short)  );
    AIO_ERROR_VALID_DATA_W_CODE( &retval, free(data); retval = AIOUSB_ERROR_INVALID_COUNTS_CONVERTER, cc );


    /**
     * @brief create temporary buffer and then Load the fifo with values
     */
   
    while ( buf->status == RUNNING  ) {
        int bytes;
        int usbresult = aiocontbuf_get_data( buf, usb, 0x86, data, buf->data_size, &bytes, 3000 );
        AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , (int)bytes);

        AIOUSB_DEVEL("Using counts=%d\n",bytes / 2 );

        bytes = MIN( (int)(buf->num_channels * (buf->num_oversamples+1)*buf->num_scans * sizeof(uint16_t) - count*sizeof(uint16_t)), bytes ); 
        retval = infifo->PushN( infifo, (uint16_t*)data, bytes / 2 );

        if ( bytes ) {

            retval = cc->ConvertFifo( cc, outfifo, infifo , bytes / sizeof(uint16_t) );

            if (  retval >= 0 ) {
                count += retval;
            }

            AIOUSB_DEVEL("Pushed %d, size: %d\n", bytes / 2 , buf->fifo->size );
            AIOUSB_DEVEL("Tmpcount=%d,count=%d,Bytes=%d, Write=%d,Read=%d,max=%d\n", retval,count,bytes,AIOFifoWritePosition(buf) , AIOFifoReadPosition(buf), AIOFifoGetSize(buf->fifo));

            /**
             * Modification, allow the count to keep going... stop 
             * if 
             * 1. count >= number we are supposed to read
             * 2. we don't have enough space
             */
            if ( count >= buf->num_scans*buf->num_channels ) {
                AIOContinuousBufLock(buf);
                buf->status = TERMINATED;
                AIOContinuousBufUnlock(buf);
            }
        } else if (  usbresult < 0  && usbfail < usbfail_count ) {
            AIOUSB_ERROR("Error with usb: %d\n", (int)usbresult );
            usbfail ++;
        } else {
            if (  usbfail >= usbfail_count  ){
                AIOUSB_ERROR("Erroring out. too many usb failures: %d\n", usbfail_count );
                retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
                AIOContinuousBufLock(buf);
                buf->status = TERMINATED;
                AIOContinuousBufUnlock(buf);
                buf->exitcode = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
            } 
        }
    }

    DeleteAIOFifoCounts(infifo);
    DeleteAIOCountsConverter( cc );
    free(data);
    AIOContinuousBufLock(buf);
    buf->status = TERMINATED;
    AIOContinuousBufUnlock(buf);
    AIOUSB_DEVEL("Stopping\n");
    AIOContinuousBufCleanup( buf );

    AIOUSB_ClearFIFO( AIOContinuousBufGetDeviceIndex(buf) ,   CLEAR_FIFO_METHOD_NOW );

    pthread_exit((void*)&retval);
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE StartStreaming( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    unsigned wValue = 0;
    unsigned wLength = 4;
    unsigned wIndex = 0;
    unsigned char data[] = {0x07, 0x0, 0x0, 0x1 } ;
    int usbval = usb->usb_control_transfer(usb, 
                                           USB_WRITE_TO_DEVICE, 
                                           AUR_START_ACQUIRING_BLOCK,
                                           wValue,
                                           wIndex,
                                           data,
                                           wLength,
                                           buf->timeout
                                           );

    AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval ), usbval >= 0 );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE SetConfig( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    USBDevice *usb = AIOUSBDeviceGetUSBHandle( deviceDesc );
    AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_INVALID_USBDEVICE, usb );


    ADCConfigBlock *config = AIOUSBDeviceGetADCConfigBlock( deviceDesc );

    usb->usb_put_config( usb, config );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE ResetCounters( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    unsigned wValue = 0x7400;
    unsigned wLength = 0;
    unsigned wIndex = 0;
    unsigned char data[0];
    int usbval;

    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_USBDEVICE_NOT_FOUND, usb );

    usbval = usb->usb_control_transfer(usb, 
                                       USB_WRITE_TO_DEVICE, 
                                       AUR_CTR_MODE,
                                       wValue,
                                       wIndex,
                                       data,
                                       wLength,
                                       buf->timeout
                                       );
    AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval), usbval == 0 );

    wValue = 0xb600;
    usbval = usb->usb_control_transfer(usb,
                                       USB_WRITE_TO_DEVICE, 
                                       AUR_CTR_MODE,
                                       wValue,
                                       wIndex,
                                       data,
                                       wLength,
                                       buf->timeout
                                       );

    AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval), usbval == 0 );
    
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufLoadCounters( AIOContinuousBuf *buf, unsigned countera, unsigned counterb )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    unsigned wValue = 0x7400;
    unsigned wLength = 0;
    unsigned char data[0];
    unsigned timeout = 3000;

    int usbval = usb->usb_control_transfer(usb,
                                           USB_WRITE_TO_DEVICE, 
                                           AUR_CTR_MODELOAD,
                                           wValue,
                                           countera,
                                           data,
                                           wLength,
                                           timeout
                                           );
    AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval), usbval == 0 );

    wValue = 0xb600;
    usbval = usb->usb_control_transfer(usb,
                                       USB_WRITE_TO_DEVICE, 
                                       AUR_CTR_MODELOAD,
                                       wValue,
                                       counterb,
                                       data,
                                       wLength,
                                       timeout
                                       );
    AIO_ERROR_VALID_AIORET_TYPE( LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval), usbval == 0 );

    return retval;
}

/*----------------------------------------------------------------------------*/
int continuous_end( USBDevice *usb , unsigned char *data, unsigned length )
{
    int retval = 0;
    unsigned bmRequestType, wValue = 0x0, wIndex = 0x0, bRequest = 0xba, wLength = 0x01;

    /* 40 BC 00 00 00 00 04 00 */
    bmRequestType = 0x40;
    bRequest = 0xbc;
    wLength = 4;
    data[0] = 0x2;
    data[1] = 0;
    data[2] = 0x2;
    data[3] = 0;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );

    /* C0 BC 00 00 00 00 04 00 */
    bmRequestType = 0xc0;
    bRequest = 0xbc;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );


    /* 40 21 00 74 00 00 00 00 */
    bmRequestType = 0x40;
    bRequest = 0x21;
    wValue = 0x7400;
    wLength = 0;
    wIndex = 0;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );
    
    wValue = 0xb600;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    unsigned char data[4] = {0};

    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    
    retval = (AIORET_TYPE)continuous_end( usb, data, 4 );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufPreSetup( AIOContinuousBuf * buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    int usbval;

    unsigned char data[0];
    unsigned wLength = 0;
    int wValue  = 0x7400, wIndex = 0;
    unsigned timeout = 7000;
    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_DATA_RETVAL( retval, retval == AIOUSB_SUCCESS );

    /* Write 02 00 02 00 */
    /* 40 bc 00 00 00 00 04 00 */
    usbval = usb->usb_control_transfer( usb, 
                                        USB_WRITE_TO_DEVICE,
                                        AUR_CTR_MODE,
                                        wValue,
                                        wIndex,
                                        data,
                                        wLength,
                                        timeout
                                        );
    AIO_ERROR_VALID_AIORET_TYPE( usbval, usbval >= AIOUSB_SUCCESS );

    wValue = 0xb600;

    /* Read c0 bc 00 00 00 00 04 00 */ 
    usbval = usb->usb_control_transfer( usb,
                                        USB_WRITE_TO_DEVICE,
                                        AUR_CTR_MODE,
                                        wValue,
                                        wIndex,
                                        data,
                                        wLength,
                                        timeout
                                      );
    AIO_ERROR_VALID_AIORET_TYPE( usbval, usbval >= 0 );


    return retval;
}

/*----------------------------------------------------------------------------*/
int continuous_setup( USBDevice *usb , unsigned char *data, unsigned length )
{
    unsigned bmRequestType, wValue = 0x0, wIndex = 0x0, bRequest = 0xba, wLength = 0x01;
    unsigned tmp[] = {0xC0, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
    memcpy(data,tmp, 8);
    int usbval = usb->usb_control_transfer( usb,
                                            0xC0,
                                            bRequest,
                                            wValue,
                                            wIndex,
                                            &data[0],
                                            wLength,
                                            1000
                                            );
    wValue = 0;
    wIndex = 0;
    wLength = 0x14;
    memset(data,(unsigned char)1,16);
    data[16] = 0;
    data[17] = 0x15;
    data[18] = 0xf0;
    data[19] = 0;
    /* 40 21 00 74 00 00 00 00 */
    bmRequestType = 0x40;
    bRequest = 0x21;
    wValue = 0x7400;
    wIndex =0;
    wLength = 0;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );
    /* 40 21 00 B6 00 00 00 00 */
    wValue = 0xB600;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );
    /*Config */


    /* 40 23 00 74 25 00 00 00 */
    wValue = 0x7400;
    bRequest = 0x23;
    wIndex = 0x64;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );


    /* 40 23 00 B6 64 00 00 00 */
    wValue = 0xb600;
    bRequest = 0x23;
    wIndex = 0x64;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );



    /* 40 BC 00 00 00 00 04 00 */
    data[0] = 0x07;
    data[1] = 0x0;
    data[2] = 0x0;
    data[3] = 0x01;
    wValue = 0x0;
    wIndex = 0x0;
    wLength = 4;
    bRequest = 0xBC;
    usb->usb_control_transfer( usb,
                               bmRequestType,
                               bRequest,
                               wValue,
                               wIndex,
                               &data[0],
                               wLength,
                               1000
                               );
    return usbval;
}

typedef  enum {
    AIO_PER_OVERSAMPLE = 1,
    AIO_PER_CHANNEL,
    AIO_PER_SCANS
} AIO_SCAN_TYPE;

/*----------------------------------------------------------------------------*/
/**
 * @brief Sets up a smart continuos mode acquisition allowing the user
 * to specify a callback function that is called based on the arguments constructed
 * in AIOCmd *cmd.  The user can specify that the callback is called after each 
 * oversample,  full chanell, full scan, or N number of scans.  
 *
 * @param buf 
 * @param cmd 
 * @param callback 
 * @return >= 0 if successful, < 0 if failure
 */
AIORET_TYPE AIOContinuousBufCallbackStartCallbackAcquisition( AIOContinuousBuf *buf, AIOCmd *cmd, AIORET_TYPE (*callback)( AIOBuf *buf) )
{
    AIOBuf **bufs = (AIOBuf **)calloc(5,sizeof(AIOBuf*));
    int i;
    int size = 1000;
    int num_bufs = 5;
    size_t data_to_read;
    int tmp_remaining;
    int data_read;
    int total;
    AIOBuf *tobuf;

    for ( i = 0; i < num_bufs; i ++ ) {
        bufs[i] = NewAIOBuf( AIO_COUNTS_BUF, size );
        if ( !bufs[i] ) {
            for ( i = i-1; i > 0 ; i --  ) {
                DeleteAIOBuf( bufs[i] );
            }
            return -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    int pos = 0;

    while ( buf->status == RUNNING ) {

        switch ( cmd->channel ) {
        case AIO_PER_OVERSAMPLE:
            data_to_read = 1 * sizeof(short);
            break;
        case AIO_PER_CHANNEL:
            data_to_read = buf->num_oversamples * sizeof(short);
            break;
        case AIO_PER_SCANS:
            data_to_read = buf->num_oversamples * sizeof(short)* buf->num_channels;
            break;
        default:
            break;
        } 

        if ( (tmp_remaining = AIOContinuousBufGetDataAvailable(buf) ) > 0 ) { 
            tobuf = bufs[pos];
            data_to_read =  tmp_remaining / data_to_read;
            data_read = AIOContinuousBufReadSingle( buf, tobuf, data_to_read );
            total += data_read;
            pos = ( pos + 1 )% num_bufs;
        }
    }

    for ( i = 0; i < num_bufs; i ++ ) {
        DeleteAIOBuf( bufs[i] );
    }

    return total;
}


/*----------------------------------------------------------------------------*/
/**
 * @brief Setups the Automated runs for continuous mode runs
 * @param buf 
 * @return 
 */
AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval;
    /**
     * @note Setup counters
     * see reference in [USB AIO documentation](http://accesio.com/MANUALS/USB-AIO%20Series.PDF)
     **/
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT_AIORET_TYPE(AIOUSB_ERROR_INVALID_DEVICE, AIOContinuousBufGetDeviceIndex(buf) >= 0 );

    /* Start the clocks, and need to get going capturing data */
    if ( (retval = ResetCounters(buf)) != AIOUSB_SUCCESS )
        goto out_AIOContinuousBufCallbackStart;

    if ( (retval = SetConfig(buf)) != AIOUSB_SUCCESS )
        goto out_AIOContinuousBufCallbackStart;
    if ( (retval = CalculateClocks( buf ) ) != AIOUSB_SUCCESS )
        goto out_AIOContinuousBufCallbackStart;
    /* Try a switch */
    if ( (retval = StartStreaming(buf)) != AIOUSB_SUCCESS )
        goto out_AIOContinuousBufCallbackStart;

    /**
     * @note BufStart ( or bulk read ) must occur before loading the counters
     */ 
    retval = AIOContinuousBufStart( buf ); /* Startup the thread that handles the data acquisition */

    if ( ( retval = AIOContinuousBufLoadCounters( buf, buf->divisora, buf->divisorb )) != AIOUSB_SUCCESS)
        goto out_AIOContinuousBufCallbackStart;



    if (  retval != AIOUSB_SUCCESS )
        goto cleanup_AIOContinuousBufCallbackStart;
    /**
     * Allow the other command to be run
     */
 out_AIOContinuousBufCallbackStart:
    return retval;
 cleanup_AIOContinuousBufCallbackStart:
    AIOContinuousBufCleanup( buf );
    return retval;
}

AIORET_TYPE AIOContinuousBuf_ResetDevice(AIOContinuousBuf *buf ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    return AIOContinuousBufResetDevice( buf );
}

AIORET_TYPE AIOContinuousBufResetDevice( AIOContinuousBuf *buf) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    unsigned char data[2] = {0x01};
    int usbval;
    USBDevice *usb = AIODeviceTableGetUSBDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS);

    usbval = usb->usb_control_transfer(usb, 0x40, 0xA0, 0xE600, 0 , data, 1, buf->timeout );
    data[0] = 0;

    usbval = usb->usb_control_transfer(usb, 0x40, 0xA0, 0xE600, 0 , data, 1, buf->timeout );
    retval = (AIORET_TYPE )usbval;

    return retval;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Reads the current available amount of data from buf, into 
 *       the readbuf datastructure
 * @param buf 
 * @param readbuf 
 * @return If number is positive, it is the number of bytes that have been read.
 */
AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned readbufsize, unsigned size)
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval;

    AIOContinuousBufLock( buf );
    int N = MIN(size ,readbufsize ) / buf->fifo->refsize;

    retval = buf->fifo->PopN( buf->fifo, readbuf, N );

    retval = ( retval == 0 ? -AIOUSB_ERROR_NOT_ENOUGH_MEMORY : retval );

    AIOContinuousBufUnlock( buf );
    return retval;
}


/*----------------------------------------------------------------------------*/
/**
 * @brief 
 * @param buf 
 * @return 
 */
AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = 0;
#ifdef HAS_PTHREAD
    retval = pthread_mutex_lock( &buf->lock );
    if ( retval != 0 ) {
        retval = -retval;
    }
#endif
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf )
{
    int retval = 0;
#ifdef HAS_PTHREAD
    retval = pthread_mutex_unlock( &buf->lock );
    if ( retval !=  0 ) {
        retval = -retval; 
        AIOUSB_ERROR("Unable to unlock mutex");
    }
#endif
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode )
{
    AIO_ASSERT_AIOCONTBUF( buf );

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    ADCConfigBlock configBlock = {0};
    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf) , (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    ADCConfigBlockInit( &configBlock, deviceDesc, AIOUSB_FALSE );
    ADCConfigBlockSetAllGainCodeAndDiffMode( &configBlock, gainCode, AIOUSB_FALSE );
    ADCConfigBlockSetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /* 0x05 */
    ADCConfigBlockSetScanRange( &configBlock, 0, 15 ); /* All 16 channels */

    ADC_QueryCal( AIOContinuousBufGetDeviceIndex(buf) );
    retval = ADC_SetConfig( AIOContinuousBufGetDeviceIndex(buf), configBlock.registers, &configBlock.size );
    AIO_ERROR_VALID_AIORET_TYPE( retval , retval == AIOUSB_SUCCESS );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf )
{ 
    AIO_ASSERT_AIOCONTBUF( buf );
    void *ptr;
    AIORET_TYPE ret;
    AIOContinuousBufLock( buf );

    AIOUSB_DEVEL("Locking and finishing thread\n");

    buf->status = TERMINATED;
    AIOUSB_DEVEL("\tWaiting for thread to terminate\n");
    AIOUSB_DEVEL("Set flag to FINISH\n");
    AIOContinuousBufUnlock( buf );


#ifdef HAS_PTHREAD
    ret = pthread_join( buf->worker , &ptr );
#endif
    if ( ret != 0 ) {
        AIOUSB_ERROR("Error joining threads");
    }
    buf->status = JOINED;
    return ret;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing ) {return AIOContinuousBufSetTesting( buf, testing );}
AIORET_TYPE AIOContinuousBufSetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing )
{
    AIO_ASSERT_AIOCONTBUF( buf );

    AIOContinuousBufLock( buf );

    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *device = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    retval = AIOUSBDeviceSetTesting( device, testing );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    retval = ADCConfigBlockSetTesting( AIOUSBDeviceGetADCConfigBlock( device ), testing );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    buf->testing = testing;

    AIOContinuousBufUnlock( buf );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetTesting( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );

    return buf->testing;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetDebug( AIOContinuousBuf *buf, AIOUSB_BOOL debug )
{
    AIO_ASSERT_AIOCONTBUF( buf );

    AIOContinuousBufLock( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *device = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval,retval == AIOUSB_SUCCESS );

    retval = ADCConfigBlockSetDebug( AIOUSBDeviceGetADCConfigBlock( device ), debug );

    buf->debug = debug;

    AIOContinuousBufUnlock( buf );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufGetDebug( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );

    return buf->debug;
}



/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex ) { return AIOContinuousBufSetDeviceIndex( buf, DeviceIndex ); }
AIORET_TYPE AIOContinuousBufSetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIOContinuousBufLock( buf );
    buf->DeviceIndex = DeviceIndex; 
    AIOContinuousBufUnlock( buf );
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SaveConfig( AIOContinuousBuf *buf ) { return AIOContinuousBufSaveConfig(buf); }
AIORET_TYPE AIOContinuousBufSaveConfig( AIOContinuousBuf *buf ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;

    SetConfig( buf );

    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetStartAndEndChannel( AIOContinuousBuf *buf, 
                                                    unsigned startChannel, 
                                                    unsigned endChannel ) {
    return AIOContinuousBufSetStartAndEndChannel( buf, startChannel, endChannel );
}
AIORET_TYPE AIOContinuousBufSetStartAndEndChannel( AIOContinuousBuf *buf, unsigned startChannel, unsigned endChannel )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE result = AIOUSB_SUCCESS;
    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&result );
    AIO_ERROR_VALID_AIORET_TYPE( result, result == AIOUSB_SUCCESS );

    if ( AIOContinuousBufNumberChannels( buf ) > 16 ) {
        deviceDesc->cachedConfigBlock.size = AD_MUX_CONFIG_REGISTERS;
    }
    result = ADCConfigBlockSetScanRange( AIOUSBDeviceGetADCConfigBlock( deviceDesc ) , startChannel, endChannel );

    return result;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetChannelRangeGain( AIOContinuousBuf *buf, 
                                                  unsigned startChannel, 
                                                  unsigned endChannel , 
                                                  unsigned gainCode ) { return AIOContinuousBufSetChannelRange(buf,startChannel,endChannel, gainCode ); }

AIORET_TYPE AIOContinuousBuf_SetChannelRange( AIOContinuousBuf *buf, 
                                              unsigned startChannel, 
                                              unsigned endChannel , 
                                              unsigned gainCode ) { return AIOContinuousBufSetChannelRange(buf,startChannel,endChannel, gainCode ); }
AIORET_TYPE AIOContinuousBufSetChannelRange( AIOContinuousBuf *buf, 
                                             unsigned startChannel, 
                                             unsigned endChannel , 
                                             unsigned gainCode )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE result = AIOUSB_SUCCESS;
    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf) , (AIORESULT*)&result );
    AIO_ERROR_VALID_AIORET_TYPE( result , result == AIOUSB_SUCCESS );


    for ( unsigned i = startChannel; i <= endChannel ; i ++ ) {
#ifdef __cplusplus
        ADCConfigBlockSetGainCode( AIOUSBDeviceGetADCConfigBlock( deviceDesc ), i, static_cast<ADGainCode>(gainCode));
#else
        ADCConfigBlockSetGainCode( AIOUSBDeviceGetADCConfigBlock( deviceDesc ), i, gainCode);
#endif
    }
    return result;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBufSetTimeout( AIOContinuousBuf *buf, unsigned timeout )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIOContinuousBufLock( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *dev = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( AIOUSB_ERROR_INVALID_DEVICE_SETTING, retval == AIOUSB_SUCCESS );

    retval = AIOUSBDeviceSetTimeout( dev, timeout );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    buf->timeout = timeout;

    AIOContinuousBufUnlock( buf );
    return retval;
}

AIORET_TYPE AIOContinuousBufGetTimeout( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    
    return buf->timeout;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE _AIOContinuousBufResizeFifo( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT_AIOCONTBUF( buf );

    int tmpval = buf->num_channels * (1 + buf->num_oversamples ) * buf->base_size * buf->unit_size / sizeof(uint16_t);

    AIORET_TYPE retval = AIOFifoResize( (AIOFifo*)buf->fifo, tmpval );
    return retval;

}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetOversample( AIOContinuousBuf *buf, unsigned os ) { return AIOContinuousBufSetOversample(buf,os);}
AIORET_TYPE AIOContinuousBufSetOversample( AIOContinuousBuf *buf, unsigned os )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *device = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    AIOContinuousBufLock( buf );
    
    if ( buf->num_oversamples != os ) {
        buf->num_oversamples = ( os > 255 ? 255 : os );
        retval = _AIOContinuousBufResizeFifo( buf );
    }
    ADCConfigBlockSetOversample( AIOUSBDeviceGetADCConfigBlock( device ), os );

    AIOContinuousBufUnlock( buf );
    return retval;
}


AIORET_TYPE AIOContinuousBufSetOverSample( AIOContinuousBuf *buf, size_t os ) { return AIOContinuousBufSetOversample(buf, os); } 


/*------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_GetOverSample( AIOContinuousBuf *buf ) { return AIOContinuousBufGetOversample( buf ); }
AIORET_TYPE AIOContinuousBufGetOversample( AIOContinuousBuf *buf ) {
    AIO_ASSERT_AIOCONTBUF( buf );
    return buf->num_oversamples;
}


/*------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff ) {
    return AIOContinuousBufSetAllGainCodeAndDiffMode( buf, gain, diff );
}
AIORET_TYPE AIOContinuousBufSetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIOContinuousBufLock( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *dev = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex( buf ), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );

    retval = ADCConfigBlockSetAllGainCodeAndDiffMode( AIOUSBDeviceGetADCConfigBlock( dev ), gain, diff );

    AIOContinuousBufUnlock( buf );
    return retval;
}

AIORET_TYPE AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard ){ return AIOContinuousBufSetDiscardFirstSample( buf, discard ); }
AIORET_TYPE AIOContinuousBufSetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard ) 
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIOContinuousBufLock( buf );
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    AIOUSBDevice *dev = AIODeviceTableGetDeviceAtIndex( AIOContinuousBufGetDeviceIndex(buf), (AIORESULT*)&retval );
    AIO_ERROR_VALID_AIORET_TYPE( retval, retval == AIOUSB_SUCCESS );
    
    dev->discardFirstSample = discard;

    AIOContinuousBufUnlock( buf );
    return retval;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf ) {return AIOContinuousBufGetDeviceIndex( buf ); }
AIORET_TYPE AIOContinuousBufGetDeviceIndex( AIOContinuousBuf *buf )
{
    AIO_ASSERT_AIOCONTBUF( buf );
    AIO_ASSERT_RET( AIOUSB_ERROR_DEVICE_NOT_FOUND, buf->DeviceIndex >= 0 );
    return (AIORET_TYPE)buf->DeviceIndex;
}


#ifdef __cplusplus
}
#endif

/*****************************************************************************
 * Self-test 
 * @note This section is for stress testing the Continuous buffer in place
 * without using the USB features
 *
 ****************************************************************************/ 

#ifdef SELF_TEST



#include "AIOUSBDevice.h"
#include "gtest/gtest.h"
#include "tap.h"
#include <iostream>
using namespace AIOUSB;



#ifndef TAP_TEST
#define LOG(...) do {                           \
    pthread_mutex_lock( &message_lock );        \
    printf( __VA_ARGS__ );                      \
    pthread_mutex_unlock(&message_lock);        \
  } while ( 0 );
#else
#define LOG(...) do { } while (0); 
#endif


void fill_buffer( AIOBufferType *buffer, unsigned size )
{
  for ( int i = 0 ; i < size; i ++ ) { 
    buffer[i] = rand() % 1000;
  }
}

void *newdoit(void *object )
{
  int counter = 0;
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  while ( counter < 10 ) {
    AIOUSB_DEBUG("Waiting in thread counter=%d\n", counter );
    sleep(1);
    counter++;
  }
  pthread_exit((void*)&retval);
  return (void*)NULL;
}

/**
 * @param object 
 * @return 
 */
void *doit( void *object )
{
    sched_yield();
    AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
    AIOUSB_DEVEL("\tAddress is 0x%x\n", (int)(unsigned long)(AIOContinuousBuf *)buf );
    unsigned  size  = 1000;
    AIOBufferType *tmp = (AIOBufferType*)malloc(size*sizeof(AIOBufferType));
    AIORET_TYPE retval;

    while ( buf->status == RUNNING ) { 
        fill_buffer( tmp, size );
        AIOUSB_DEVEL("\tLooping spinning wheels\n"); 
        retval = AIOContinuousBufWrite( buf, tmp, size, size , AIOCONTINUOUS_BUF_NORMAL );
        AIOUSB_DEVEL("\tWriting buf , attempted write of size size=%d, wrote=%d\n", size, (int)retval );
    }
    AIOUSB_DEVEL("Stopping\n");
    AIOUSB_DEVEL("Completed loop\n");
    free(tmp);
    pthread_exit((void*)&retval);
    return NULL;
}

/**
 * @param object 
 * @return 
 */
void *channel16_doit( void *object )
{
    sched_yield();
    AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
    AIOUSB_DEVEL("\tAddress is 0x%x\n", (int)(unsigned long)(AIOContinuousBuf *)buf );
    unsigned  size  = 16*64;
    AIOBufferType *tmp = (AIOBufferType*)malloc(size*sizeof(AIOBufferType));
    AIORET_TYPE retval;

    while ( buf->status == RUNNING ) { 
        fill_buffer( tmp, size );
        AIOUSB_DEVEL("\tLooping spinning wheels\n"); 
        retval = AIOContinuousBufWrite( buf, tmp, size ,size,  AIOCONTINUOUS_BUF_ALLORNONE );
        usleep( rand()%100 );
        if (  retval >= 0 && retval != size ) {
            AIOUSB_ERROR("Error writing. Wrote bytes of size=%d but should have written=%d\n", (int)retval, size );
            AIOUSB_ERROR("read_pos=%d, write_pos=%d\n", AIOFifoReadPosition(buf), AIOFifoWritePosition(buf));
            _exit(2);
        }
        AIOUSB_DEVEL("\tWriting buf , attempted write of size size=%d, wrote=%d\n", size, (int)retval );
    }
    AIOUSB_DEVEL("Stopping\n");
    AIOUSB_DEVEL("Completed loop\n");
    free(tmp);
    pthread_exit((void*)&retval);
    return NULL;
}

AIORET_TYPE read_data( unsigned short *data , unsigned size) 
{
  
  for ( int i = 0 ; i < size; i ++ ) { 
    data[i] = i % 256;
  }
  return (AIORET_TYPE)size;
}

int bufsize = 1000;

class AIOContinuousBufSetup : public ::testing::Test 
{
 protected:
    virtual void SetUp() {
        numAccesDevices = 0;
        AIOUSB_Init();
        result = AIOUSB_SUCCESS;
        AIODeviceTableAddDeviceToDeviceTableWithUSBDevice( &numAccesDevices, USB_AI16_16E, NULL );
        device = AIODeviceTableGetDeviceAtIndex( numAccesDevices ,  &result );
    }
  
    virtual void TearDown() { 

    }
    int numAccesDevices;
    AIORESULT result;
    AIOUSBDevice *device;
    unsigned short *data;
};

TEST(AIOContinuousBuf,WritingCounts ) 
{
    int num_channels = 16;
    int num_scans = 5000, size = num_scans;
    AIORET_TYPE retval;

    unsigned short *tobuf = (unsigned short *)malloc( num_scans*num_channels*2 );
    for ( int i = 0; i < num_channels*num_scans; i ++ ) tobuf[i] = i;

    AIOContinuousBuf *buf = NewAIOContinuousBufForCounts( 0, num_scans, num_channels );
    ASSERT_TRUE( buf );
    
    ASSERT_EQ( AIOContinuousBufGetSize(buf), AIOFifoGetSize(buf->fifo) );
    retval = AIOContinuousBufSetBaseSize( buf, 5000 );
    ASSERT_EQ( AIOContinuousBufGetSizeNumElements(buf), 5000*AIOContinuousBufGetNumberOfChannels(buf)*(1+AIOContinuousBufGetOversample(buf)));

    retval = buf->PushN( buf, tobuf, num_scans*num_channels );
    EXPECT_EQ( retval, num_scans*num_channels*sizeof(unsigned short) );

    DeleteAIOContinuousBuf(buf);
    free(tobuf);
    
}

TEST(AIOContinuousBuf, StreamingSize ) 
{
    int num_channels = 16;
    int num_scans = 5000, size = num_scans;
    AIORET_TYPE retval;

    unsigned short *tobuf = (unsigned short *)malloc( num_scans*num_channels*2 );

    AIOContinuousBuf *buf = NewAIOContinuousBufForCounts( 0, num_scans, num_channels );

    ASSERT_TRUE( buf );
    ASSERT_EQ( AIOContinuousBufGetStreamingBlockSize(buf), 64*1024 ) << "default size is " << 64*1024 << "\n";

    AIOContinuousBufSetStreamingBlockSize( buf, 513 );
    ASSERT_EQ( AIOContinuousBufGetStreamingBlockSize(buf), 512 ) << "Rounding of bufsize to multiple of 512\n";

    AIOContinuousBufSetStreamingBlockSize( buf, 1 );
    ASSERT_EQ( AIOContinuousBufGetStreamingBlockSize(buf), 512 ) << "Minimum size is 512\n";

    DeleteAIOContinuousBuf( buf );
}

class AIOBufParams {
public:
    int num_scans;
    int num_channels ;
    int num_oversamples;
    AIOBufParams( int numscans, int numchannels=16, int numoversamples=0 ) : num_scans(numscans), 
                                                                             num_channels(numchannels), 
                                                                             num_oversamples(numoversamples) { };
    friend std::ostream &operator<<( std::ostream &os, const AIOBufParams &p );
};

std::ostream &operator<< ( std::ostream &os, const AIOBufParams &p ) {
    os << "#Scans=" << p.num_scans << ", #Ch=" << p.num_channels << ", #OSamp=" << p.num_oversamples;
    return os;
}

class AIOContinuousBufThreeParamTest : public ::testing::TestWithParam<AIOBufParams> {};
TEST_P(AIOContinuousBufThreeParamTest,StressTestDrain) 
{
    AIOContinuousBuf *buf;
    int oversamples   = GetParam().num_oversamples;
    int num_scans     = GetParam().num_scans;
    int num_channels  = GetParam().num_channels;
    int repeat_count  = 20;
    int count  =0;
    int retval = 0;
    buf = NewAIOContinuousBufForCounts( 0, num_scans , num_channels );
    unsigned short *data = (unsigned short *)malloc(num_scans*(oversamples+1)*num_channels*sizeof(unsigned short) );

    int numAccesDevices = 0;
    AIOUSB_Init();
    AIORESULT result = AIOUSB_SUCCESS;
    AIODeviceTableAddDeviceToDeviceTableWithUSBDevice( &numAccesDevices, USB_AI16_16E, NULL );
    AIOUSBDevice *device = AIODeviceTableGetDeviceAtIndex( numAccesDevices ,  &result );
    AIOUSB_Init();

    AIOContinuousBufInitADCConfigBlock( buf, 20, AD_GAIN_CODE_0_5V, AIOUSB_FALSE , 255, AIOUSB_FALSE );

    EXPECT_EQ( AIOContinuousBufGetOversample( buf ), 255 );

    free(data);
    DeleteAIOContinuousBuf( buf );
}

// Combine a bunch of different entities
//INSTANTIATE_TEST_CASE_P(AllCombinations, AIOContinuousBufThreeParamTest, ::testing::Combine(::testing::ValuesIn(num_channels),::testing::ValuesIn(num_channels)));
INSTANTIATE_TEST_CASE_P(AllCombinations, AIOContinuousBufThreeParamTest, ::testing::Values( AIOBufParams(1,2,3),
                                                                                          AIOBufParams(100,16,0))
                        );


/**
 * @note originally was stress_copy_counts(bufsize)
 * @details This test checks the stress copying that can go on with the 
 * AIOContinuousBuf buffer. This test verifies the following
 * 1. Reading integer number of scans from the AIOBuffer
 * 2. Correctly not allowing us to read scans into a buffer with storage 
      < scan # of channels for storage
 * 3. Allow reading and writing to occur and loop in the case where
 *    write_pos increases beyond the end of the fifo
 *
 *
 * @brief  We have an AIOContinuousBuf twice the total size of the tobuf.
 *
 *
 * @param num_channels_per_scan := 16
 * @param num_scans             := 2048
 * 
 */
TEST_P(AIOContinuousBufThreeParamTest,BufferScanCounting ) 
{
    unsigned extra = 0;
    int core_size = 256;
    int num_scans     = GetParam().num_scans;
    int num_channels  = GetParam().num_channels;

    int tobuf_size     = num_channels * num_scans * sizeof(unsigned short);
    int use_data_size  = num_channels * num_scans * sizeof(unsigned short );

    unsigned short *use_data  = (unsigned short *)calloc(1, use_data_size  );
    unsigned short *tobuf     = (unsigned short *)calloc(1, tobuf_size );

    AIORET_TYPE retval;
    AIOContinuousBuf *buf = NewAIOContinuousBufForCounts( 0, num_scans+1, num_channels ); 

    /**
     * Pre allocate some values, linear range
     */
    for ( int i = 0 ; i < num_scans*num_channels; i ++ ) { 
        use_data[i] = (unsigned short)i;
    }

    /**
     * @brief We will write slightly less than 1 full buffer ( buffer_size ) of data into the 
     * Aiocontbuf, then set the read position to match the write position, then write one more almost
     * buffer size of data. We should see that the new write position is 2*bytes read % size of the buffer
     */ 
    retval = AIOContinuousBufWriteCounts( buf, use_data, use_data_size, use_data_size, AIOCONTINUOUS_BUF_ALLORNONE );
    EXPECT_EQ( retval, use_data_size ) << "Number of bytes written should equal the full size of the buffer";

    EXPECT_EQ( AIOContinuousBufGetRemainingSize(buf)/ sizeof(unsigned short), (1)*num_channels );

    /*----------------------------------------------------------------------------*/
    /**< Cleanup */
    DeleteAIOContinuousBuf(buf); 
    free(use_data);
    free(tobuf);
}

/**
 * @brief Test reading and writing from the AIOBuf
 *
 * @li Try to write in too much and show that it fails
 * @li Try to read out too much
 *
 */
TEST(AIOContinuousBuf,BasicFunctionality ) 
{
    int num_scans = 4000;
    int num_channels = 16;
    int size = num_scans*num_channels;
    AIOContinuousBuf *buf = NewAIOContinuousBufForCounts( 0 , num_scans, num_channels );

    int tmpsize = 4*num_scans*num_channels;

    AIOBufferType *frombuf = (AIOBufferType *)malloc(tmpsize*sizeof(AIOBufferType ));
    AIOBufferType *readbuf = (AIOBufferType *)malloc(tmpsize*sizeof(AIOBufferType ));
    AIORET_TYPE retval;
    for ( int i = 0 ; i < tmpsize; i ++ ) { 
        frombuf[i] = rand() % 1000;
    }

    /**
     * Should write since we are writing from a buffer of size tmpsize (80000) into a buffer of 
     * size 4000
     */
    retval = AIOContinuousBufWrite( buf, frombuf , tmpsize, tmpsize*sizeof(AIOBufferType) , AIOCONTINUOUS_BUF_ALLORNONE  );
    EXPECT_EQ( -AIOUSB_ERROR_NOT_ENOUGH_MEMORY, retval ) << "Should have not enough memory error\n";
  
    /* Test writing */
    retval = AIOContinuousBufWrite( buf, frombuf , tmpsize, size*sizeof(AIOBufferType) , AIOCONTINUOUS_BUF_NORMAL  );
    ASSERT_GE( retval, AIOUSB_SUCCESS ) << "not able to write even at write position=" << AIOFifoWritePosition(buf) << std::endl;
    retval = AIOContinuousBufWrite( buf, frombuf , tmpsize, size*sizeof(AIOBufferType) ,  AIOCONTINUOUS_BUF_NORMAL );
    ASSERT_LT( retval, AIOUSB_SUCCESS ) << "Can't write into a full buffer";

    /* Do a simple reset */
    AIOContinuousBufReset( buf );
    retval = AIOContinuousBufWrite( buf, frombuf , tmpsize, size*sizeof(AIOBufferType) , AIOCONTINUOUS_BUF_NORMAL  );
    ASSERT_GE( retval, AIOUSB_SUCCESS ) << "Able to write to a reset buffer" << AIOFifoWritePosition(buf) << std::endl;

    /* Full buffer */
    retval = AIOContinuousBufCountScansAvailable( buf );
    EXPECT_EQ( retval, num_scans );
    
    /* Test reading */
    retval = AIOContinuousBufRead( buf, readbuf, num_scans*num_channels*2, num_scans*num_channels*2);
    EXPECT_EQ( retval , num_scans*num_channels*2 );

    /*verify that the data matches the original */
    for ( int i = 0; i < num_scans*num_channels  ; i ++ ) { 
        EXPECT_EQ( frombuf[i], readbuf[i] );
    }

    /*  Testing writing, and then reading the integer number of scans remaining  */
    retval = AIOContinuousBufWrite( buf, frombuf , tmpsize, size*sizeof(AIOBufferType) , AIOCONTINUOUS_BUF_NORMAL  );
    ASSERT_GE( retval, AIOUSB_SUCCESS ) << "Should be able to write to an empty buffer" << AIOFifoWritePosition(buf) << std::endl;


    int num_scans_to_read = AIOContinuousBufCountScansAvailable( buf );
    retval = AIOContinuousBufReadIntegerScanCounts( buf, readbuf, tmpsize , tmpsize);
    EXPECT_EQ( retval, num_scans );

    DeleteAIOContinuousBuf( buf );

    free(frombuf);
}

/**
 * This test case builds up
 * parts of the new constructor 
 */
TEST(AIOContinuousBuf, NewConstructor ) 
{
    int numDevices = 0;
    AIODeviceTableInit();    
    AIODeviceTableAddDeviceToDeviceTable( &numDevices, USB_AIO16_16A );
    EXPECT_EQ( numDevices, 1 );
    int num_oversamples = 7;
    int num_channels = 9;
    AIOContinuousBuf *buf= NewAIOContinuousBuf(0, num_channels, num_oversamples, 1024 );
    AIORET_TYPE retval;
    int origsize = AIOContinuousBufGetSize(buf);

    ASSERT_TRUE( buf );

    AIOContinuousBufSetDeviceIndex( buf, numDevices - 1 );

    AIOContinuousBufSetNumberOfChannels( buf , num_channels );
    EXPECT_EQ( num_channels, AIOContinuousBufGetNumberOfChannels( buf ) );

    ASSERT_TRUE( buf->fifo->size % num_channels != 0 ) << "Changing the number of channels should adjust the fifo size " <<
        "so that it is divisible by the number of channels and oversamples\n";

    ASSERT_EQ( 0, AIOFifoGetSizeNumElements( buf->fifo ) % num_channels  );

    /**
     * Now make sure that if the number of channels changes, that we 
     * also resize
     */

    retval = AIOContinuousBufSetOversample( buf, num_oversamples ); 
    ASSERT_GE( 0, retval );
    ASSERT_EQ( 0, AIOFifoGetSizeNumElements( buf->fifo ) % (num_oversamples+1)  ) << "Must be divisible by 1 + the number of oversamples\n";

    DeleteAIOContinuousBuf( buf );
}

/**
 * @brief Goal is to have a simple buffer, write some data into it, and then
 *        have the callback be allerted when we have one oversample available
 *
 */ 
TEST(AIOContinuousBuf, NewAPIForReading )
{
    int numDevices = 0;
    AIODeviceTableInit();    
    AIODeviceTableAddDeviceToDeviceTable( &numDevices, USB_AIO16_16A );
    int num_oversamples = 7;
    int num_channels = 9;
    int num_scans = 1024;
    AIOContinuousBuf *buf= NewAIOContinuousBuf(0, num_channels, num_oversamples, num_scans );
    short tmpbuf[1024];
    short tmpbuf2[1024];
    AIORET_TYPE retval;


    memset(tmpbuf2,0,sizeof(tmpbuf2));
    for ( int i = 0; i < sizeof(tmpbuf)/sizeof(short); i ++ )
        tmpbuf[i] = i;


    ASSERT_GT( (int)buf->type, 0 );

    AIOContinuousBufSetDeviceIndex( buf, numDevices - 1);

    AIOContinuousBufSetNumberOfChannels( buf , 9 );
    EXPECT_EQ( 9, AIOContinuousBufGetNumberOfChannels( buf ) );
    int origsize = AIOContinuousBufGetSize(buf);
    EXPECT_GE( origsize, 0 );

    AIOContinuousBufPushN( buf, (unsigned short*)tmpbuf, sizeof(short)/sizeof(short));
    
    /* check position */
    ASSERT_EQ( 1, buf->fifo->write_pos / sizeof(short) );

    AIOFifoReset( buf->fifo );
    ASSERT_EQ( 0, buf->fifo->write_pos / sizeof(short) );

    AIOContinuousBufPushN( buf, (unsigned short*)tmpbuf, sizeof(tmpbuf)/sizeof(short));

    ASSERT_EQ( sizeof(tmpbuf)/sizeof(short) , buf->fifo->write_pos / sizeof(short) );

    AIOFifoReset( buf->fifo );
    

    _AIOContinuousBufWrite( buf, tmpbuf, 1024 );

    ASSERT_EQ( sizeof(tmpbuf)/sizeof(short) , buf->fifo->write_pos / sizeof(short) );

    /* Goal is to do small reads of certain sizes */
    retval = _AIOContinuousBufRead( buf, tmpbuf2, 1024 );
    ASSERT_GE( retval , 0 );
    EXPECT_EQ( 0 ,  memcmp( tmpbuf, tmpbuf2, sizeof( tmpbuf2 )));

    ASSERT_EQ( sizeof(tmpbuf2), buf->fifo->read_pos );    
    
    AIOFifoReset( buf->fifo );
    /* Start with getting all of the remaining oversamples */

    retval = AIOContinuousBufSetOversample( buf , num_oversamples );
    EXPECT_GE( retval, 0 );
    EXPECT_EQ( AIOContinuousBufGetOversample(buf ), num_oversamples );

    retval = _AIOContinuousBufRead( buf, tmpbuf2, buf->num_oversamples );
    ASSERT_GE( retval, 0 );

    DeleteAIOContinuousBuf( buf );
}

TEST(AIOContiuousBuf,FailCorrectly)
{
    
    unsigned long DeviceIndex = 0;
    unsigned scancounts = 1024;
    unsigned num_channels = 0;
    AIOUSB_BOOL counts  = AIOUSB_FALSE;
    
    ASSERT_DEATH( { NewAIOContinuousBufForCounts(DeviceIndex, scancounts, num_channels );  }, "Assertion `num_channels > 0' failed");
}

TEST(AIOContinuousBuf,Deprecated_buffer_size) 
{
    uint16_t counts[1024];
    AIOContinuousBuf *buf = NewAIOContinuousBuf(0,16,0,1024);

    for ( int i = 0; i < sizeof(counts)/sizeof(uint16_t) ; i ++ ) {
        counts[i] = (uint16_t)i;
    }

    AIOContinuousBufPushN(buf , counts, 20 );
    EXPECT_EQ( AIOContinuousBufGetSize( buf ), AIOFifoGetSize( buf->fifo ));
    ASSERT_GE( AIOContinuousBufGetSize( buf ), 0 );
}

TEST(AIOContinuousBuf,ChangeUnitSize)
{
    AIOContinuousBuf *buf = NewAIOContinuousBuf(0,16,0,1024);
    int before = AIOContinuousBufGetSize( buf );
    AIOContinuousBufSetUnitSize( buf, sizeof(double) );
    EXPECT_GT( AIOContinuousBufGetSize( buf ), before );


    ASSERT_TRUE( buf );
}



#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[] )
{
  
  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
  delete listeners.Release(listeners.default_result_printer());
#endif

  return RUN_ALL_TESTS();  

}

#endif





