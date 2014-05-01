/**
 * @file   AIOContinuousBuffer.c
 * @author  $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "aiousb.h"
#include "AIOUSB_Core.h"
#include "AIOTypes.h"
#include "AIOContinuousBuffer.h"
#include "AIOChannelMask.h"
#include "AIOUSB_Core.h"
#include <libusb.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif


static pthread_t cont_thread;
static pthread_mutex_t message_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *outfile = NULL;


#undef AIOUSB_LOG
#define AIOUSB_LOG(fmt, ... ) do {                                      \
    pthread_mutex_lock( &message_lock );                                \
    fprintf( (!outfile ? stdout : outfile ), fmt,  ##__VA_ARGS__ );     \
    pthread_mutex_unlock(&message_lock);                                \
  } while ( 0 )

#undef AIOUSB_DEVEL
#undef AIOUSB_DEBUG
#undef AIOUSB_WARN 
#undef AIOUSB_ERROR
#undef AIOUSB_FATAL 

#ifdef AIOUSB_DEBUG_LOG
/**
 * If you _REALLY_ want to see Development messages, you will
 * need to compile with  with -DREALLY_USE_DEVEL_DEBUG
 **/
#ifdef REALLY_USE_DEVEL_DEBUG
#define AIOUSB_DEVEL(...)  if( 1 ) { AIOUSB_LOG( "<Devel>\t" __VA_ARGS__ ); }
#define AIOUSB_TAP(x,...)  if( 1 ) { AIOUSB_LOG( ( x ? "ok -" : "not ok" ) __VA_ARGS__ ); }
#else
#define AIOUSB_DEVEL(...)  if( 0 ) { AIOUSB_LOG( "<Devel>\t" __VA_ARGS__ ); }
#define AIOUSB_TAP(x,...)  if( 0 ) { AIOUSB_LOG( ( x ? "ok -" : "not ok" ) __VA_ARGS__ ); }
#endif
#define AIOUSB_DEBUG(...)  AIOUSB_LOG( "<Debug>\t" __VA_ARGS__ )
#else

#define AIOUSB_DEVEL( ... ) if ( 0 ) { }
#define AIOUSB_DEBUG( ... ) if ( 0 ) { }
#endif


void *ActualWorkFunction( void *object );
void *RawCountsWorkFunction( void *object );

/**
 * Compile with -DAIOUSB_DISABLE_LOG_MESSAGES 
 * if you don't wish to see these warning messages
 **/
#ifndef AIOUSB_DISABLE_LOG_MESSAGES
#define AIOUSB_WARN(...)   AIOUSB_LOG("<Warn>\t"  __VA_ARGS__ )
#define AIOUSB_INFO(...)   AIOUSB_LOG("<Info>\t"  __VA_ARGS__ )
#define AIOUSB_ERROR(...)  AIOUSB_LOG("<Error>\t" __VA_ARGS__ )
#define AIOUSB_FATAL(...)  AIOUSB_LOG("<Fatal>\t" __VA_ARGS__ )
#endif

PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufForCounts( unsigned long DeviceIndex, unsigned scancounts, unsigned num_channels )
{
  assert( num_channels > 0 );
  AIOContinuousBuf *tmp  = NewAIOContinuousBufWithoutConfig( DeviceIndex, scancounts, num_channels, AIOUSB_TRUE );
  AIOContinuousBuf_SetCallback( tmp, RawCountsWorkFunction );
  return tmp;
}

/**
 * @desc Constructor for AIOContinuousBuf object. Will set up the 
 * @param bufsize 
 * @param num_channels 
 * @return 
 * @todo Needs a smarter constructor for specifying the Initial mask .Currently won't work
 *       for num_channels > 32
 */
PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts )
{
  assert( num_channels > 0 );
  AIOContinuousBuf *tmp  = (AIOContinuousBuf *)malloc(sizeof(AIOContinuousBuf));
  tmp->mask              = NewAIOChannelMask( num_channels );

  if ( num_channels > 32 ) { 
    char *bitstr = (char *)malloc( num_channels +1 );
    memset(bitstr, 49, num_channels ); /* Set all to 1s */
    bitstr[num_channels] = '\0';
    AIOChannelMask_SetMaskFromStr( tmp->mask, bitstr );
    free(bitstr);
  } else {
    AIOChannelMask_SetMaskFromInt( tmp->mask, (unsigned)-1 >> (BIT_LENGTH(unsigned)-num_channels),0 ); /**< Use all bits for each channel */
  }
  tmp->testing      = AIOUSB_FALSE;
  tmp->size         = num_channels * scancounts;
  if( counts ) {
      tmp->buffer = (AIOBufferType *)malloc( tmp->size * sizeof(unsigned short));
      tmp->bufunitsize = sizeof(unsigned short);
  } else {
      tmp->buffer      = (AIOBufferType *)malloc( tmp->size *sizeof(AIOBufferType ));
      tmp->bufunitsize = sizeof(AIOBufferType);
  }
  tmp->basesize     = scancounts;
  tmp->exitcode     = 0;
  tmp->usbbuf_size  = 128*512;

  tmp->_read_pos    = 0;
  tmp->DeviceIndex  = DeviceIndex;
  tmp->_write_pos   = 0;
  tmp->status       = NOT_STARTED;
  tmp->worker       = cont_thread;
  tmp->hz           = 100000; /**> Default value of 100khz  */
  tmp->timeout      = 1000;   /**> Default Timeout of 1000us  */
  tmp->extra        = 0;
  tmp->tmpbuf       = NULL;
  tmp->tmpbufsize   = 0;
#ifdef HAS_PTHREAD
  tmp->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;   /* Threading mutex Setup */
#endif
  AIOContinuousBuf_SetCallback( tmp , ActualWorkFunction );

  return tmp;
}

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_InitConfiguration(  AIOContinuousBuf *buf ) 
{
  ADConfigBlock config;
  AIORET_TYPE retval;
  unsigned long tmp;
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_NoCheck( AIOContinuousBuf_GetDeviceIndex(buf));

  ADC_InitConfigBlock( &config, (void *)&deviceTable[AIOContinuousBuf_GetDeviceIndex( buf )], deviceDesc->ConfigBytes  );
  config.testing = buf->testing;
  AIOContinuousBuf_SendPreConfig( buf );
  tmp = AIOUSB_SetConfigBlock( AIOContinuousBuf_GetDeviceIndex( buf ), &config );
  if ( tmp != AIOUSB_SUCCESS ) {
      retval = -(AIORET_TYPE)tmp;
  }
  return retval;
}

PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , unsigned scancounts , unsigned num_channels )
{
  AIOContinuousBuf *tmp = NewAIOContinuousBufWithoutConfig( DeviceIndex,  scancounts, num_channels , AIOUSB_FALSE );
  return tmp;
}

PUBLIC_EXTERN AIOContinuousBuf *NewAIOContinuousBufTesting( unsigned long DeviceIndex , unsigned scancounts , unsigned num_channels , AIOUSB_BOOL counts )
{
  AIOContinuousBuf *tmp = NewAIOContinuousBufWithoutConfig( DeviceIndex,  scancounts, num_channels , counts );
  tmp->testing = AIOUSB_TRUE;
  return tmp;
}

PUBLIC_EXTERN AIOBufferType *AIOContinuousBuf_CreateTmpBuf( AIOContinuousBuf *buf, unsigned size )
{
    if ( ! buf->tmpbuf || buf->tmpbufsize != size ) {
        if ( buf->tmpbuf )
          free(buf->tmpbuf);
        buf->tmpbuf = (AIOBufferType *)malloc(sizeof(AIOBufferType)*size);
        buf->tmpbufsize = size;
    }
    return buf->tmpbuf;
}

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SendPreConfig( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval;
    unsigned wLength = 0x1, wIndex = 0x0, wValue = 0x0, bRequest = AUR_PROBE_CALFEATURE;
    int usbresult = 0;
    unsigned char data[1];
    if( !buf->testing ) {
        usbresult = libusb_control_transfer( AIOUSB_GetDeviceHandle( AIOContinuousBuf_GetDeviceIndex( buf )),
                                             USB_READ_FROM_DEVICE,
                                             bRequest,
                                             wValue,
                                             wIndex,
                                             data,
                                             wLength,
                                             buf->timeout
                                             );
    }
    
    if (usbresult < 0 ) {
        retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT( usbresult );
    }
    return retval;
}

PUBLIC_EXTERN void AIOContinuousBuf_DeleteTmpBuf( AIOContinuousBuf *buf )
{
    if ( buf->tmpbuf || buf->tmpbufsize > 0 ) {
        free(buf->tmpbuf);
    }
}

/**
 * @desc Destructor for AIOContinuousBuf object
 * @param buf 
 */
PUBLIC_EXTERN void DeleteAIOContinuousBuf( AIOContinuousBuf *buf )
{
    DeleteAIOChannelMask( buf->mask );
    AIOContinuousBuf_DeleteTmpBuf( buf );
    free( buf->buffer );
    free( buf );
}

PUBLIC_EXTERN void AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) )
{
  AIOContinuousBufLock( buf );
  buf->callback = work;
  AIOContinuousBufUnlock( buf );
}

static unsigned buffer_size( AIOContinuousBuf *buf )
{
  return buf->size;
}

static unsigned buffer_max( AIOContinuousBuf *buf )
{
  return buffer_size(buf)-1;
}

void set_read_pos(AIOContinuousBuf *buf , unsigned pos )
{
  if( pos > buffer_max( buf ) )
    buf->_read_pos = buffer_max(buf);
  else
    buf->_read_pos = pos;
}

unsigned get_read_pos( AIOContinuousBuf *buf )
{
  return buf->_read_pos;
}

void set_write_pos(AIOContinuousBuf *buf , unsigned pos )
{
  if( pos > buffer_max( buf ) )
    buf->_write_pos = buffer_max(buf);
  else
    buf->_write_pos = pos;
}

unsigned get_write_pos( AIOContinuousBuf *buf )
{
  return buf->_write_pos;
}



PUBLIC_EXTERN unsigned AIOContinuousBuf_BufSizeForCounts( AIOContinuousBuf * buf) {
  return buffer_size(buf);
}

static unsigned write_size( AIOContinuousBuf *buf ) {
  unsigned retval = 0;
  unsigned read, write;
  read = (unsigned )get_read_pos(buf);
  write = (unsigned)get_write_pos(buf);
 if( read > write ) {
   retval =  read - write;
 } else {
   return buffer_size(buf) - (get_write_pos (buf) - get_read_pos (buf));
 }
 return retval;
}

static unsigned write_size_num_scan_counts( AIOContinuousBuf *buf ) {
  float tmp = write_size(buf) / AIOContinuousBuf_NumberChannels(buf);
  if( tmp > (int)tmp ) {
      tmp = (int)tmp;
  } else {
      tmp = ( tmp - 1 < 0 ? 0 : tmp -1 );
  }
  return (unsigned)tmp;
}

PUBLIC_EXTERN unsigned AIOContinuousBuf_NumberWriteScansInCounts( AIOContinuousBuf *buf ) {
  return AIOContinuousBuf_NumberChannels(buf)*write_size_num_scan_counts( buf ) ;
}

/**
 * @desc Returns the amount of data available in the buffer
 * @param buf 
 * @return 
 */
unsigned read_size( AIOContinuousBuf *buf ) 
{
  return ( buffer_size(buf) - write_size(buf) );
}

PUBLIC_EXTERN unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf )
{
  return get_read_pos( buf );
}

PUBLIC_EXTERN unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf )
{
 return get_write_pos( buf );
}

PUBLIC_EXTERN unsigned AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf )
{
  return read_size(buf);
}

PUBLIC_EXTERN unsigned int AIOContinuousBufGetSize( AIOContinuousBuf *buf )
{
  return buffer_size(buf);
}

PUBLIC_EXTERN THREAD_STATUS AIOContinuousBufGetStatus( AIOContinuousBuf *buf )
{
  return buf->status;
}
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufGetExitCode( AIOContinuousBuf *buf )
{
  return buf->exitcode;
}


/** 
 * @desc returns the number of Scans accross all channels that still 
 *       remain in the buffer
 * @param buf 
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufCountScansAvailable(AIOContinuousBuf *buf) 
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  retval = AIOContinuousBufAvailableReadSize( buf ) / AIOContinuousBuf_NumberChannels(buf);
  return retval;
}

#undef MIN
#define MIN(X,Y) ( (X) < (Y) ? (X) : (Y) )
/** 
 * @desc will read in an integer number of scan counts if there is room.
 * @param buf 
 * @param tmp 
 * @param size The size of the tmp buffer
 * @return 
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufReadIntegerScanCounts( AIOContinuousBuf *buf, 
                                                                 unsigned short *tmp , 
                                                                 unsigned tmpsize, 
                                                                 unsigned size 
                                                                 )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  int debug = 0;
  if( size < AIOContinuousBuf_NumberChannels(buf) ) {
      return -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
  }
  int numscans = size / AIOContinuousBuf_NumberChannels(buf);
  
  for ( int i = 0, pos=0 ; i < numscans && (pos + AIOContinuousBuf_NumberChannels(buf)-1) < size ; i ++ , pos += AIOContinuousBuf_NumberChannels(buf) ) {
      if( i == 0 )
        retval = AIOUSB_SUCCESS;
      if( debug ) { 
          printf("Using i=%d\n",i );
      }
      retval += AIOContinuousBufRead( buf, (AIOBufferType *)&tmp[pos] , tmpsize-pos, AIOContinuousBuf_NumberChannels(buf) );

  }

  return retval;
}


PUBLIC_EXTERN void AIOContinuousBufReset( AIOContinuousBuf *buf )
{
  AIOContinuousBufLock( buf );
  set_read_pos(buf, 0 );
  set_write_pos(buf, 0 );
  AIOContinuousBufUnlock( buf );
}

/** 
 * @desc Returns 
 * @param buf 
 * @return Pointer to our work function
 */
PUBLIC_EXTERN AIOUSB_WorkFn AIOContinuousBuf_GetCallback( AIOContinuousBuf *buf )
{
  return buf->callback;
}

PUBLIC_EXTERN void AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz )
{
  buf->hz = hz;
}


/**
 * @desc Starts the work function
 * @param buf 
 * @param work 
 * @return status code of start.
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufStart( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval;
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
  if( retval != 0 ) {
    buf->status = TERMINATED;
    AIOUSB_ERROR("Unable to create thread for Continuous acquisition");
    return -1;
  }
#endif  

  return retval;
}

/**
 * @desc Calculates the register values for buf->divisora, and buf->divisorb to create
 * an output clock that matches the value stored in buf->hz
 * @param buf AIOContinuousBuf object that we will be reading data into
 * @return Success(0) or failure( < 0 ) if we can't set the clocks
 */
PUBLIC_EXTERN AIORET_TYPE CalculateClocks( AIOContinuousBuf *buf )
{

  unsigned  hz = buf->hz;
  float l;
  unsigned ROOTCLOCK = 10000000;
  unsigned divisora, divisorb, divisorab;
  unsigned min_err, err;

  if( hz == 0 ) {
    return -AIOUSB_ERROR_INVALID_PARAMETER;
  }
  if(  hz * 4 >= ROOTCLOCK ) {
    divisora = 2;
    divisorb = 2;
  } else { 
    divisorab = ROOTCLOCK / hz;
    l = sqrt( divisorab );
    if ( l > 0xffff ) { 
      divisora = 0xffff;
      divisorb = 0xffff;
      min_err  = abs(round(((ROOTCLOCK / hz) - (divisora * l))));
    } else  { 
      divisora  = round( divisorab / l );
      l         = round(sqrt( divisorab ));
      divisorb  = l;

      min_err = abs(((ROOTCLOCK / hz) - (divisora * l)));
      
      for( unsigned lv = l ; lv >= 2 ; lv -- ) {
        unsigned olddivisora = (int)round((double)divisorab / lv);
        if( olddivisora > 0xffff ) { 
          AIOUSB_DEVEL( "Found value > 0xff..resetting" );
          break;
        } else { 
          divisora = olddivisora;
        }

        err = abs((ROOTCLOCK / hz) - (divisora * lv));
        if( err <= 0  ) {
          min_err = 0;
          AIOUSB_DEVEL("Found zero error: %d\n", lv );
          divisorb = lv;
          break;
        } 
        if( err < min_err  ) {
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


/** create thread to launch function */
PUBLIC_EXTERN AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf )
{
  AIORET_TYPE retval;
  retval = pthread_create( &(buf->worker), NULL , callback, (void *)buf  );
  if( retval != 0 ) {
    retval = -abs(retval);
  }
  return retval;
}

/**
 * @desc Sets the channel mask
 * @param buf 
 * @param mask 
 * @return 
 */
AIORET_TYPE AIOContinuousBuf_SetChannelMask( AIOContinuousBuf *buf, AIOChannelMask *mask )
{
  buf->mask   = mask;
  buf->extra  = 0;
  return 0;
}

unsigned AIOContinuousBuf_NumberChannels( AIOContinuousBuf *buf )
{
  return AIOChannelMask_NumberChannels(buf->mask );
}

/** 
 * @desc A simple copy of one ushort buffer to one of AIOBufferType and converts
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
    AIORET_TYPE retval = 0;
    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_NoCheck( AIOContinuousBuf_GetDeviceIndex(buf));
    int number_channels = AIOContinuousBuf_NumberChannels(buf);
    assert(channel);
    if( ! deviceDesc ) {
      retval = -1;
    } else {
      for(unsigned ch = 0; ch < count;  ch ++ , *channel = ((*channel+1)% number_channels ) , *pos += 1 ) {
        unsigned gain = ADC_GainCode_Cached( &deviceDesc->cachedConfigBlock , *channel );
        struct ADRange *range = &adRanges[ gain ];
        tobuf[ *pos ] = ( (( double )data[ ch ] / ( double )AI_16_MAX_COUNTS) * range->range ) + range->minVolts;
        retval += 1;
      }
   }
    return retval;
}

/**
 * @desc Performs the maintenance of copying an exact channel Sample number of all channels
 *       into the ContinuousBuffer. First it must conver the raw data from the USB capture
 *       to AIOBufferType using the Configuration settings to determine voltage range and scaling,
 *       then after a large buffer has been filled up, one large write is performed at once.
 * @param buf Continuous Buffer that we will write to
 * @param data buffer that is read from a usb transaction, 512 bytes  plus the extra
 *        padding at the end that is used for storing extr data.
 * @param extra Number of extra records that we will be saving
 * @return Success if >=0 , error otherwise
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_CopyData( AIOContinuousBuf *buf , unsigned short *data , unsigned *size )
{
     assert(data);
     assert(*size > 0 );
     unsigned i = 0, write_count = 0;
     AIORET_TYPE retval;
     unsigned tmpcount, channel = 0, pos = 0;
     unsigned stopval=0;
     unsigned number_oversamples = AIOContinuousBuf_GetOverSample(buf)+1;
     unsigned long DeviceIndex = AIOContinuousBuf_GetDeviceIndex( buf );
     unsigned number_channels = AIOContinuousBuf_NumberChannels(buf);
     AIOBufferType *tmpbuf = AIOContinuousBuf_CreateTmpBuf( buf, (*size / (AIOContinuousBuf_GetOverSample(buf) + 1)) + number_channels );

     int core_size = *size / number_oversamples;
     unsigned tmpsize = *size;

     cull_and_average_counts( DeviceIndex, data, &tmpsize, AIOContinuousBuf_NumberChannels(buf) );
     /**
      * @note
      * @verbatim
      *                      | Extra 
      *   ----   ----   ---- | ----   ----   ----   ---- 
      *  |    | |    | |    |||    | |    | |    | |    |
      *   ----   ----   ---- | ----   ----   ----   ---- 
      *                      |
      *                            
      *                      ^
      *                      |
      *                    buf end
      *
      *  Extra data is behind the buf end in the data
      *  buffer. In this case it's two extra shorts
      * @endverbatim
      */ 
     if( buf->extra ) {
       
       channel = (number_channels - buf->extra );
       write_count += AIOContinuousBuf_SmartCountsToVolts( buf,  &channel,  &data[i], buf->extra,  &tmpbuf[pos], &pos );
       write_count += AIOContinuousBuf_SmartCountsToVolts( buf,  &channel,  &data[0], (number_channels - write_count),  &tmpbuf[pos], &pos );
     } 
     /* Completed one channel range from the extra packets */
     tmpcount = write_count - buf->extra;
     write_count = tmpcount;
     stopval = ((core_size - tmpcount)/number_channels)*number_channels;
     write_count += AIOContinuousBuf_SmartCountsToVolts( buf,  &channel,  &data[tmpcount], stopval, &tmpbuf[0], &pos );
     buf->extra = ( core_size - write_count );
     /* (1) can you correct this */
     memcpy(&data[*size], &data[write_count], buf->extra*sizeof(data[0]) );

     AIOUSB_DEVEL( "After write: #Channels: %d, Wrote %d full channels, Extra %d\n", number_channels,write_count / number_channels , buf->extra );

     retval = AIOContinuousBufWrite( buf, (AIOBufferType *)tmpbuf,  (*size / (AIOContinuousBuf_GetOverSample(buf) + 1)),write_count, AIOCONTINUOUS_BUF_ALLORNONE );

     return (AIORET_TYPE)retval;
 }

void *RawCountsWorkFunction( void *object )
{
  AIORET_TYPE retval;
  int usbresult;
  AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
  int bytes;
  srand(3);

  unsigned datasize = AIOContinuousBuf_NumberChannels(buf)*16*512;
  int usbfail = 0;
  int usbfail_count = 5;
  unsigned char *data   = (unsigned char *)malloc( datasize );
  unsigned count = 0;
  /* unsigned printcount = 0; */
  /* int totalcount = 0; */

  while ( buf->status == RUNNING  ) {

#ifdef TESTING
    FILE *tmpf;
    tmpf = fopen("tmpdata.txt","w");
    unsigned short *usdata = (unsigned short *)&data[0];
      int tval = MIN(AIOContinuousBuf_NumberWriteScansInCounts(buf)/AIOContinuousBuf_NumberChannels(buf), 
                     datasize / 2 / AIOContinuousBuf_NumberChannels(buf) );
      int trand = (rand() % tval + 1 );
      bytes = 2*AIOContinuousBuf_NumberChannels(buf)*trand;
      for( int i = 0, ch = 0; i < AIOContinuousBuf_NumberChannels(buf)*trand; i ++ , ch = ((ch+1)%AIOContinuousBuf_NumberChannels(buf))) {
          usdata[i] = ch*1000 + rand()%20;
          fprintf(tmpf, "%u,",usdata[i] );
          if( (ch +1) % AIOContinuousBuf_NumberChannels(buf) == 0 ) {
              totalcount ++;
              fprintf(tmpf,"\n",usdata[i] );
          }
      }
      printf("");
#else
      usbresult = libusb_bulk_transfer( AIOUSB_GetDeviceHandle( AIOContinuousBuf_GetDeviceIndex( buf )),
                                        0x86,
                                        data,
                                        datasize,
                                        &bytes,
                                        3000
                                        );
#endif

      AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , (int)bytes);

      if( bytes ) {
        /* only write bytes that exist */
          int tmpcount = MIN((buffer_size(buf)-get_write_pos(buf)) - AIOContinuousBuf_NumberChannels(buf), bytes/2 );
          int tmp = AIOContinuousBufWriteCounts( buf, 
                                                (unsigned short *)&data[0],
                                                datasize/2,
                                                tmpcount,
                                                AIOCONTINUOUS_BUF_ALLORNONE
                                                );
          if( tmp >= 0 ) {
              count += tmp;
          }

          AIOUSB_DEVEL("Tmpcount=%d,count=%d,Bytes=%d, Write=%d,Read=%d, max=%d\n", tmpcount,count,bytes,get_write_pos(buf) , get_read_pos(buf),buffer_size(buf));

          if( count >= AIOContinuousBuf_BufSizeForCounts(buf) - AIOContinuousBuf_NumberChannels(buf) ) {
              AIOContinuousBufLock(buf);
              buf->status = TERMINATED;
              AIOContinuousBufUnlock(buf);
          }
      } else if( usbresult < 0  && usbfail < usbfail_count ) {
          AIOUSB_ERROR("Error with usb: %d\n", (int)usbresult );
          usbfail ++;
      } else {
          if( usbfail >= usbfail_count  ){
              AIOUSB_ERROR("Erroring out. too many usb failures: %d\n", usbfail_count );
              retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
              AIOContinuousBufLock(buf);
              buf->status = TERMINATED;
              AIOContinuousBufUnlock(buf);
              buf->exitcode = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
          } 
      }
  }
#ifdef TESTING
  fclose(tmpf);
#endif
  AIOContinuousBufLock(buf);
  buf->status = TERMINATED;
  AIOContinuousBufUnlock(buf);
  AIOUSB_DEVEL("Stopping\n");
  AIOContinuousBufCleanup( buf );
  pthread_exit((void*)&retval);
  
}

/**
 * @desc Main work function for collecting data. Also performs copies from 
 *       the raw acquiring buffer into the AIOContinuousBuf
 * @param object 
 * @return 
 * @todo Ensure that copying matches the actual size of the data
 */
void *ActualWorkFunction( void *object )
{
  AIORET_TYPE retval;
  int usbresult;
  /* sched_yield(); */
  AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
  unsigned long result;
  int bytes;
  unsigned datasize = 128*512;
  int usbfail = 0;
  int usbfail_count = 5;
  unsigned char *data   = (unsigned char *)malloc( datasize );
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex(buf), &result);
  if(!deviceDesc || result != AIOUSB_SUCCESS) {
    retval = -result;
    goto out_ActualWorkFunction;
  }

  while ( buf->status == RUNNING ) {
    usbresult = libusb_bulk_transfer( AIOUSB_GetDeviceHandle( AIOContinuousBuf_GetDeviceIndex( buf )),
                                      0x86,
                                      data,
                                      /* buf->usbbuf_size, */
                                      datasize,
                                      &bytes,
                                      3000
                                      );

    AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , (int)bytes);
    if( bytes ) {
      retval = AIOContinuousBuf_CopyData( buf, (unsigned short*)data , (unsigned *)&bytes );
    } else if( usbresult < 0  && usbfail < usbfail_count ) {
      AIOUSB_ERROR("Error with usb: %d\n", (int)usbresult );
      usbfail ++;
    } else {
      if( usbfail >= usbfail_count  ){
        AIOUSB_ERROR("Erroring out. too many usb failures: %d\n", usbfail_count );
        retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
        AIOContinuousBufLock(buf);
        buf->status = TERMINATED;
        AIOContinuousBufUnlock(buf);
        buf->exitcode = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbresult);
      } 
    }
  }
out_ActualWorkFunction:
  AIOUSB_DEVEL("Stopping\n");
  AIOContinuousBufCleanup( buf );

  pthread_exit((void*)&retval);
}

PUBLIC_EXTERN AIORET_TYPE StartStreaming( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  unsigned wValue = 0;
  unsigned wLength = 4;
  unsigned wIndex = 0;
  unsigned char data[] = {0x07, 0x0, 0x0, 0x1 } ;
  int usbval = libusb_control_transfer(deviceHandle, 
                                       USB_WRITE_TO_DEVICE, 
                                       AUR_START_ACQUIRING_BLOCK,
                                       wValue,
                                       wIndex,
                                       data,
                                       wLength,
                                       buf->timeout
                                       );
  if ( usbval < 0 ) {
      retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval );
  }
  return retval;
}

PUBLIC_EXTERN AIORET_TYPE SetConfig( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    unsigned long result;
    DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex( buf ), &result );
    if (!deviceDesc || result != AIOUSB_SUCCESS ) {
        retval = (AIORET_TYPE)result;
        goto out_SetConfig;
    }
    if( AIOContinuousBuf_NumberChannels(buf) > 16 ) {
      deviceDesc->cachedConfigBlock.size = AD_MUX_CONFIG_REGISTERS;
    }
    retval = ADC_WriteADConfigBlock( AIOContinuousBuf_GetDeviceIndex( buf ), &deviceDesc->cachedConfigBlock );
    /* ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex( buf ),  */
    /*                &deviceDesc->cachedConfigBlock.registers[0],  */
    /*                &deviceDesc->cachedConfigBlock.size ); */

 out_SetConfig:
    return retval;
}

PUBLIC_EXTERN AIORET_TYPE ResetCounters( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  unsigned wValue = 0x7400;
  unsigned wLength = 0;
  unsigned wIndex = 0;
  unsigned char data[0];
  int usbval = libusb_control_transfer(deviceHandle, 
                                       USB_WRITE_TO_DEVICE, 
                                       AUR_CTR_MODE,
                                       wValue,
                                       wIndex,
                                       data,
                                       wLength,
                                       buf->timeout
                                       );
  if ( usbval  != 0 ) {
      retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval);
      goto out_ResetCounters;
  }
  wValue = 0xb600;
  usbval = libusb_control_transfer(deviceHandle, 
                                   USB_WRITE_TO_DEVICE, 
                                   AUR_CTR_MODE,
                                   wValue,
                                   wIndex,
                                   data,
                                   wLength,
                                   buf->timeout
                                   );
  if ( usbval  != 0 )
      retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval);
out_ResetCounters:
  return retval;

}


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufLoadCounters( AIOContinuousBuf *buf, unsigned countera, unsigned counterb )
{
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
    /* unsigned wValue = countera; */
    /* unsigned wIndex = counterb; */
    unsigned wValue = 0x7400;
    unsigned wLength = 0;
    unsigned char data[0];
    unsigned timeout = 3000;
    int usbval = libusb_control_transfer(deviceHandle, 
                                         USB_WRITE_TO_DEVICE, 
                                         AUR_CTR_MODELOAD,
                                         wValue,
                                         countera,
                                         data,
                                         wLength,
                                         timeout
                                         );
    if ( usbval != 0 ) {
        retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval);
        goto out_AIOContinuousBufLoadCounters;
    }
    wValue = 0xb600;
    usbval = libusb_control_transfer(deviceHandle, 
                                     USB_WRITE_TO_DEVICE, 
                                     AUR_CTR_MODELOAD,
                                     wValue,
                                     counterb,
                                     data,
                                     wLength,
                                     timeout
                                     );
    if ( usbval != 0 )
        retval = -(AIORET_TYPE)LIBUSB_RESULT_TO_AIOUSB_RESULT(usbval);

out_AIOContinuousBufLoadCounters:
    return retval;
}

int continuous_end( libusb_device_handle *deviceHandle , unsigned char *data, unsigned length )
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
                           bmRequestType,
                           bRequest,
                           wValue,
                           wIndex,
                           &data[0],
                           wLength,
                           1000
                           );

  wValue = 0xb600;
  libusb_control_transfer( deviceHandle,
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


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval;
  unsigned char data[4] = {0};
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  retval = (AIORET_TYPE)continuous_end( deviceHandle, data, 4 );
  return retval;
}

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufPreSetup( AIOContinuousBuf * buf )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  int usbval;
  unsigned long result;
  libusb_device_handle *deviceHandle;
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex(buf), &result);
  if(!deviceDesc || result != AIOUSB_SUCCESS)
    return -result;

  deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  unsigned char data[0];
  unsigned wLength = 0;
  int wValue  = 0x7400, wIndex = 0;
  unsigned timeout = 7000;
  /* Write 02 00 02 00 */
  /* 40 bc 00 00 00 00 04 00 */
  
  usbval = libusb_control_transfer( deviceHandle, 
                                    USB_WRITE_TO_DEVICE,
                                    AUR_CTR_MODE,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );
  if( usbval != AIOUSB_SUCCESS ) {
    retval = -usbval;
    goto out_AIOContinuousBufPreSetup;
  }
  wValue = 0xb600;

  /* Read c0 bc 00 00 00 00 04 00 */ 
  usbval = libusb_control_transfer( deviceHandle,
                                    USB_WRITE_TO_DEVICE,
                                    AUR_CTR_MODE,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );
  if( usbval != 0 )
    retval = -usbval;

 out_AIOContinuousBufPreSetup:
  return retval;

}

int continuous_setup( libusb_device_handle *deviceHandle , unsigned char *data, unsigned length )
{
  unsigned bmRequestType, wValue = 0x0, wIndex = 0x0, bRequest = 0xba, wLength = 0x01;
  unsigned tmp[] = {0xC0, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
  memcpy(data,tmp, 8);
  int usbval = libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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
  libusb_control_transfer( deviceHandle,
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



/** 
 * @desc Setups the Automated runs for continuous mode runs
 * @param buf 
 * @return 
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf )
{
    AIORET_TYPE retval;
  /** 
   * Setup counters
   * see reference in [USB AIO documentation](http://accesio.com/MANUALS/USB-AIO%20Series.PDF)
   **/
  /* Start the clocks, and need to get going capturing data */
  if( (retval = ResetCounters(buf)) != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStart;
  if( (retval = SetConfig(buf)) != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStart;
  if ( (retval = CalculateClocks( buf ) ) != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStart;
  /* Try a switch */
  if( (retval = StartStreaming(buf)) != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStart;
  if( ( retval = AIOContinuousBufLoadCounters( buf, buf->divisora, buf->divisorb )) != AIOUSB_SUCCESS)
    goto out_AIOContinuousBufCallbackStart;

  retval = AIOContinuousBufStart( buf ); /* Startup the thread that handles the data acquisition */

  if( retval != AIOUSB_SUCCESS )
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

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_ResetDevice( AIOContinuousBuf *buf) 
{
  libusb_device_handle *deviceHandle;
  unsigned char data[2] = {0x01};
  AIORET_TYPE retval;
  int usbval;
  deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf )); 
  
  usbval = libusb_control_transfer( deviceHandle, 0x40, 0xA0, 0xE600, 0 , data, 1, buf->timeout );
  data[0] = 0;
  usbval = libusb_control_transfer( deviceHandle,0x40, 0xA0, 0xE600, 0 , data, 1, buf->timeout );
  retval = (AIORET_TYPE )usbval;
  return retval;
}

/** 
 * @desc Reads the current available amount of data from buf, into 
 *       the readbuf datastructure
 * @param buf 
 * @param readbuf 
 * @return If number is positive, it is the number of bytes that have been read.
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned readbufsize, unsigned size)
{

  AIORET_TYPE retval;
  unsigned basic_copy , wrap_copy ;
  char *tbuf;

  AIOContinuousBufLock( buf );

  if ( get_read_pos(buf) <= get_write_pos(buf) ) {
    basic_copy = MIN(size, get_write_pos(buf) - get_read_pos( buf ));
    wrap_copy  = 0;
  } else {
    basic_copy = MIN(size, buffer_size(buf) - get_read_pos(buf));
    wrap_copy  = MIN(size - basic_copy, get_write_pos(buf) );
  }
  /* Now copy the data into readbuf */
  tbuf = (char *)&buf->buffer[0] + get_read_pos(buf)*buf->bufunitsize;
  memcpy( &readbuf[0]          , &tbuf[0], basic_copy*buf->bufunitsize );
  memcpy( &readbuf[basic_copy] , &buf->buffer[0]                  , wrap_copy*buf->bufunitsize );
  
  if( wrap_copy ) {
    retval = basic_copy + wrap_copy;
    set_read_pos( buf, ( get_read_pos(buf) + retval) % buffer_size(buf) );
  } else {
    retval = basic_copy;
    set_read_pos( buf , ( get_read_pos(buf) + retval) % buffer_size(buf) );
  }

  AIOContinuousBufUnlock( buf );
  return retval;
}

/** 
 * @desc Allows one to write in to the AIOContinuousBuf buffer a given amount of data.
 * @param buf 
 * @param writebuf 
 * @param size 
 * @param flag
 * @return Status of whether the write was successful , if so returning the number of bytes written
 *         or if there was insufficient space, it returns negative error code. If the number 
 *         is >= 0, then this corresponds to the number of bytes that were written into the buffer.
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, 
                                                 AIOBufferType *writebuf, 
                                                 unsigned wrbufsize, 
                                                 unsigned size, 
                                                 AIOContinuousBufMode flag )
{
  AIORET_TYPE retval;
  unsigned basic_copy, wrap_copy;
  char *tbuf;
  ERR_UNLESS_VALID_ENUM( AIOContinuousBufMode ,  flag );
    
  /* First try to lock the buffer */
  /* printf("trying to lock buffer for write\n"); */
  AIOContinuousBufLock( buf );

  /* Then see if the remaining size is large enough */
  if( size > buffer_size( buf ) ) {
    retval = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
    goto out_AIOContinuousBufWrite;
  }

  if( write_size(buf) > size || flag == AIOCONTINUOUS_BUF_NORMAL ) {
    if( get_read_pos(buf) > get_write_pos(buf) ) { 
      basic_copy = MIN( wrbufsize, (MIN( size, ( get_read_pos( buf ) - get_write_pos( buf ) - 1 ))));
      wrap_copy  = 0;
    } else {
      basic_copy = MIN((MIN( size, ( buffer_max(buf) - get_write_pos( buf ) + 1 ))), wrbufsize );
      wrap_copy  = MIN( size - basic_copy, get_read_pos(buf) );
    }
  } else {            /* not enough room in remaining space */
    if( flag & AIOCONTINUOUS_BUF_OVERRIDE )  {
      basic_copy = MIN( size, ( buffer_max(buf) - get_write_pos(buf)  ));
      wrap_copy  = size - basic_copy;
    } else {                    /* Assuming All or none */
      retval = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
      goto out_AIOContinuousBufWrite;
    }
  }
  
  tbuf = (char *)&buf->buffer[ 0] + get_write_pos(buf)*buf->bufunitsize;
  memcpy( &tbuf[0] , &writebuf[0] , basic_copy*buf->bufunitsize );
  memcpy( &buf->buffer[ 0 ] , &writebuf[basic_copy]  , wrap_copy*buf->bufunitsize  );
  
  set_write_pos( buf, (get_write_pos (buf) + basic_copy + wrap_copy ) % buffer_size(buf) );
  retval = basic_copy+wrap_copy;

  /* If the flag is set such that we can
   * overwrite , then we are ok, otherwise, 
   * let's do something different */

 out_AIOContinuousBufWrite:
  AIOContinuousBufUnlock( buf );
  return retval;
}

AIORET_TYPE AIOContinuousBufWriteCounts( AIOContinuousBuf *buf, unsigned short *data, unsigned datasize, unsigned size , AIOContinuousBufMode flag )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  retval += AIOContinuousBufWrite( buf, (AIOBufferType *)data, datasize, size , flag  );

  return retval;
}



/** 
 * @desc 
 * @param buf 
 * @return 
 */
PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf )
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

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode )
{
  AIORET_TYPE retval;
  ADConfigBlock configBlock = {'\0'};
  AIOUSB_InitConfigBlock( &configBlock, AIOContinuousBuf_GetDeviceIndex(buf), AIOUSB_FALSE );
  AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, gainCode, AIOUSB_FALSE );
  AIOUSB_SetTriggerMode( &configBlock, AD_TRIGGER_SCAN | AD_TRIGGER_TIMER ); /* 0x05 */
  AIOUSB_SetScanRange( &configBlock, 0, 15 ); /* All 16 channels */
  ADC_QueryCal( AIOContinuousBuf_GetDeviceIndex(buf) );
  retval = ADC_SetConfig( AIOContinuousBuf_GetDeviceIndex(buf), configBlock.registers, &configBlock.size );
  if ( retval != AIOUSB_SUCCESS ) 
    return (AIORET_TYPE)(-retval);
  return retval;
}


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf )
{ 
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

PUBLIC_EXTERN void AIOContinuousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing )
{
  AIOContinuousBufLock( buf );
  ADC_SetTestingMode( AIOUSB_GetConfigBlock( AIOContinuousBuf_GetDeviceIndex(buf)), testing );
  AIOContinuousBufUnlock( buf );
}


PUBLIC_EXTERN void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex )
{
  AIOContinuousBufLock( buf );
  buf->DeviceIndex = DeviceIndex; 
  AIOContinuousBufUnlock( buf );
}


PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SaveConfig( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  AIOContinuousBufLock(buf);
  SetConfig( buf );
  AIOContinuousBufUnlock(buf);
  return retval;
}

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SetStartAndEndChannel( AIOContinuousBuf *buf, 
                                                                  unsigned startChannel, 
                                                                  unsigned endChannel )
{
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_NoCheck( AIOContinuousBuf_GetDeviceIndex(buf));
  if ( AIOContinuousBuf_NumberChannels( buf ) > 16 ) {
    deviceDesc->cachedConfigBlock.size = AD_MUX_CONFIG_REGISTERS;
  }

  return -(AIORET_TYPE)abs(AIOUSB_SetScanRange( &deviceDesc->cachedConfigBlock , startChannel, endChannel ));
}

PUBLIC_EXTERN AIORET_TYPE AIOContinuousBuf_SetChannelRangeGain( AIOContinuousBuf *buf, 
                                                                unsigned startChannel, 
                                                                unsigned endChannel , 
                                                                unsigned gainCode )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_NoCheck( AIOContinuousBuf_GetDeviceIndex(buf));
  for ( unsigned i = startChannel; i <= endChannel ; i ++ ) {
    AIOUSB_SetGainCode( &deviceDesc->cachedConfigBlock, i, gainCode);
  }
  return retval;
}



PUBLIC_EXTERN void AIOContinuousBuf_SetOverSample( AIOContinuousBuf *buf, unsigned os )
{
  AIOContinuousBufLock( buf );
  ADC_SetOversample( AIOContinuousBuf_GetDeviceIndex(buf), os );
  AIOContinuousBufUnlock( buf );
}

PUBLIC_EXTERN unsigned AIOContinuousBuf_GetOverSample( AIOContinuousBuf *buf )
{
    return ADC_GetOversample( AIOContinuousBuf_GetDeviceIndex(buf) );
}



PUBLIC_EXTERN void AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff )
{
  AIOContinuousBufLock( buf );
  ADC_SetAllGainCodeAndDiffMode( AIOContinuousBuf_GetDeviceIndex( buf ), (unsigned)gain , diff );
  AIOUSB_UnLock();
  AIOContinuousBufUnlock( buf );
}

PUBLIC_EXTERN void AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard ) 
{
  AIOContinuousBufLock( buf );
  AIOUSB_SetDiscardFirstSample( AIOContinuousBuf_GetDeviceIndex(buf), discard );
  AIOContinuousBufUnlock( buf );
}



PUBLIC_EXTERN unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf )
{
  assert( buf->DeviceIndex >= 0 );
  return (unsigned long)buf->DeviceIndex;
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
    usleep(rand()%100);
    if( retval >= 0 && retval != size ) {
      AIOUSB_ERROR("Error writing. Wrote bytes of size=%d but should have written=%d\n", (int)retval, size );
      AIOUSB_ERROR("read_pos=%d, write_pos=%d\n", get_read_pos(buf), get_write_pos(buf));
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



void
stress_test_one( int size , int readbuf_size )
{
  AIORET_TYPE retval;
  /* int readbuf_size = size - 10; */
  AIOBufferType *readbuf = (AIOBufferType *)malloc( readbuf_size*sizeof(AIOBufferType ));
  AIOContinuousBuf *buf = NewAIOContinuousBuf( 0, size , 16 );
  AIOUSB_DEVEL("Original address is 0x%x\n", (int)(unsigned long)(AIOContinuousBuf *)buf );
  AIOContinuousBufReset( buf );
  AIOContinuousBuf_SetCallback( buf , doit );
  AIOUSB_DEBUG("Was able to reset device\n");
  retval = AIOContinuousBufStart( buf );
  AIOUSB_DEBUG("Able to start new Acquisition\n");
  printf("%s", ( retval < 0 ? "not ok -" : "ok - " ));
  printf("Ran threaded collection with readbuf_size=%d\n",readbuf_size);
  for(int i = 0 ; i < 500; i ++ ) {
    /* retval = AIOContinuousBufRead( buf,  readbuf, readbuf_size ); */
    retval = AIOContinuousBufRead( buf,  readbuf, readbuf_size, readbuf_size );
    usleep(rand() % 100);
    AIOUSB_DEVEL("Read number of bytes=%d\n",(int)retval );
  }
  AIOContinuousBufEnd( buf );
  int distance = ( get_read_pos(buf) > get_write_pos(buf) ? 
                   (buffer_max(buf) - get_read_pos(buf) ) + get_write_pos(buf) :
                   get_write_pos(buf) - get_read_pos(buf) );

  AIOUSB_DEVEL("Read: %d, Write: %d\n", get_read_pos(buf),get_write_pos(buf));
  for( int i = 0; i <= distance / readbuf_size ; i ++ ) {
    retval = AIOContinuousBufRead( buf, readbuf,readbuf_size,readbuf_size  );
  }
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size ,readbuf_size );
  printf("%s", ( (int)retval == 0 ? "ok" : "not ok" ));
  printf(" - Completely read in the buffer for size=%d\n",readbuf_size);
  DeleteAIOContinuousBuf( buf );
  free(readbuf);
}


void basic_functionality()
{
  AIOContinuousBuf *buf = NewAIOContinuousBuf(0,  4000 , 16 );
  int tmpsize = 80000;
  AIOBufferType *tmp = (AIOBufferType *)malloc(tmpsize*sizeof(AIOBufferType ));
  AIORET_TYPE retval;
  for ( int i = 0 ; i < tmpsize; i ++ ) { 
    tmp[i] = rand() % 1000;
  }
  retval = AIOContinuousBufWrite( buf, tmp , tmpsize, tmpsize , AIOCONTINUOUS_BUF_ALLORNONE  );
  printf("%s", ( (int)retval == -AIOUSB_ERROR_NOT_ENOUGH_MEMORY ? "ok" : "not ok" ));
  printf(" - Able to perform first write, count is %d \n", (int)retval );
  
  free(tmp);
  
  unsigned size = 4999;
  tmp = (AIOBufferType *)malloc(size*sizeof(AIOBufferType ));
  for( int i = 0; i < 3; i ++ ) {
    for( int j = 0 ; j < size; j ++ ) {
      tmp[j] = rand() % 1000;
    }
    retval = AIOContinuousBufWrite( buf, tmp , tmpsize, size , AIOCONTINUOUS_BUF_ALLORNONE  );
    if( i == 0 ) {
      printf("%s", ( AIOContinuousBufAvailableReadSize(buf) == 4999 ? "ok" : "not ok" ));
      printf(" - Able to find available read space\n");
    }
    if( i == 2 ) { 
      printf("%s", ( (int)retval != 0 ? "ok" : "not ok" ));
      printf(" - Correctly stops writing\n");
    } else {
      printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
      printf(" - Still able to write, count is %d\n", get_write_pos(buf) );
    }
  }
  retval = AIOContinuousBufWrite( buf, tmp , tmpsize, size , AIOCONTINUOUS_BUF_NORMAL  );
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - able to write, count is %d\n", get_write_pos(buf) );
  
  retval = AIOContinuousBufWrite( buf, tmp , tmpsize, size , AIOCONTINUOUS_BUF_OVERRIDE );
  printf("%s", ( (int)retval != 0 ? "ok" : "not ok" ));
  printf(" - Correctly writes with override \n");
  
  int readbuf_size = size - 10;
  AIOBufferType *readbuf = (AIOBufferType *)malloc( readbuf_size*sizeof(AIOBufferType ));
  
  /* 
   * Problem here.
   */  
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size, readbuf_size );
  printf("%s", ( (int)retval != 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");

  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size, readbuf_size );
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");


  free(tmp);
  size = 6000;
  tmp = (AIOBufferType *)malloc(size*sizeof(AIOBufferType ));
  for( int j = 0 ; j < size; j ++ ) {
    tmp[j] = rand() % 1000;
  }
  retval = AIOContinuousBufWrite( buf, tmp , size, size , AIOCONTINUOUS_BUF_NORMAL);
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");

  free(readbuf);
  readbuf_size = (  buffer_max(buf) - get_read_pos (buf) + 2000 );
  readbuf = (AIOBufferType *)malloc(readbuf_size*sizeof(AIOBufferType ));
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size, readbuf_size );
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");

  DeleteAIOContinuousBuf( buf );
  free(readbuf);
  free(tmp);

}

void stress_test_read_channels( int bufsize, int keysize  ) 
{
  AIOContinuousBuf *buf = NewAIOContinuousBuf( 0,  bufsize , 16 );
  int mybufsize = (16*keysize);
  AIOBufferType *tmp = (AIOBufferType *)malloc(mybufsize*sizeof(AIOBufferType ));
  AIORET_TYPE retval;
  AIOContinuousBuf_SetCallback( buf , channel16_doit);
  AIOContinuousBufReset( buf );
  retval = AIOContinuousBufStart( buf );
  if( retval < AIOUSB_SUCCESS )
    goto out_stress_test_read_channels;

  for ( int i = 0; i < 2000; i ++ ) {
    retval = AIOContinuousBufRead( buf, tmp, mybufsize, mybufsize );
    AIOUSB_DEVEL("Read %d bytes\n", (int)retval );
    usleep(rand()%100);
    if ( retval < AIOUSB_SUCCESS )
      goto out_stress_test_read_channels;
  }
  AIOContinuousBufEnd( buf );
  /* Now read out all of the remaining sizes */
  int stopval =read_size(buf) / mybufsize;
  if( stopval == 0 )
    stopval = 1;
  for( int i = 1 ; i <= stopval ; i ++ ) {
    retval = AIOContinuousBufRead( buf, tmp, mybufsize, mybufsize );
  }
  retval = AIOContinuousBufRead( buf, tmp, mybufsize, mybufsize );

out_stress_test_read_channels:
  
  /* printf("%s - was able to read for keysize %d\n", (retval == AIOUSB_SUCCESS ? "ok" : "not ok" ), keysize); */
  printf("%s - was able to read for keysize %d: %d\n", (retval == AIOUSB_SUCCESS ? "ok" : "not ok" ), keysize, (int)retval);

  free(tmp);
  DeleteAIOContinuousBuf( buf );
}


void continuous_stress_test( int bufsize )
{
  AIOContinuousBuf *buf = NewAIOContinuousBuf( 0, bufsize , 16 );
  int tmpsize = pow(16,(double)ceil( ((double)log((double)(bufsize/1000))) / log(16)));
  int keepgoing = 1;
  AIORET_TYPE retval;
  AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);
  int ntest_count = 0;

  AIOUSB_Init();
  GetDevices();
  AIOContinuousBufSetClock( buf, 1000 );
  AIOContinuousBufCallbackStart( buf );

  while ( keepgoing ) {
    retval = AIOContinuousBufRead( buf, tmp, tmpsize, tmpsize );
    sleep(1);
    AIOUSB_INFO("Waiting : readpos=%d, writepos=%d\n", get_read_pos(buf),get_write_pos(buf));
    if( get_read_pos(buf) < 1000 ) {
      ntest_count ++;
    }
#ifdef NTEST
    if( ntest_count > 5000 ) {
      AIOContinuousBufEnd( buf );
      keepgoing = 0;
    }
#else
    if( get_read_pos( buf )  > 60000 ) {
      AIOContinuousBufEnd( buf );
      keepgoing = 0;
    }
#endif
  }
#ifdef TESTING
  set_read_pos(buf,0);
  for(int i = 0; i < get_write_pos(buf) /16 ; i ++ ) {
    for( int j =0; j < 16 ; j ++ ) { 
      printf("%f,", buf->buffer[i*16+j] );
    }
    printf("\n");
  }
#endif
  printf("%s - Able to finish reading buffer\n", (retval >= AIOUSB_SUCCESS ? "ok" : "not ok" ));
}

AIORET_TYPE read_data( unsigned short *data , unsigned size) {
  
  for ( int i = 0 ; i < size; i ++ ) { 
    data[i] = i % 256;
  }
  return (AIORET_TYPE)size;
}

/* 
 * Dummy setup
 */
void dummy_init(void)
{
  int numAccesDevices = 0;
  aiousbInit = AIOUSB_INIT_PATTERN;
  AIOUSB_Init();
  DeviceDescriptor *deviceDesc = &deviceTable[ numAccesDevices++ ];
  deviceDesc->DIOBytes = 2;
  deviceDesc->Counters = 1;
  deviceDesc->RootClock = 10000000;
  deviceDesc->bADCStream = AIOUSB_TRUE;
  deviceDesc->ImmADCs = 1;
  deviceDesc->ADCChannels = deviceDesc->ADCMUXChannels = 128;
  deviceDesc->ADCChannelsPerGroup = 16; /* Needed for larger than 16 signals */
  deviceDesc->ConfigBytes = AD_CONFIG_REGISTERS;
  deviceDesc->bClearFIFO = AIOUSB_TRUE;
  deviceDesc->device = (libusb_device *)42;
  deviceDesc->deviceHandle = (libusb_device_handle *)42;
  deviceDesc->cachedConfigBlock.size = 20;
  /* Set up a dummy config object */

}


void stress_test_drain_buffer( int bufsize ) 
{
  AIOContinuousBuf *buf;
  unsigned extra = 0;
  int core_size = 256;
  int channel_list[] = { 9,19, 3, 5, 7, 9 ,11,31, 37 , 127};
  int oversamples[]  = {255};
  int prev;
  int repeat_count = 20;
  int expected_list[] = { (core_size*20)%channel_list[0], 
                          (core_size*20)%channel_list[1],
                          (core_size*20)%channel_list[2],
                          (core_size*20)%channel_list[3],
                          (core_size*20)%channel_list[4],
                          (core_size*20)%channel_list[5],
                          (core_size*20)%channel_list[6],
                          (core_size*20)%channel_list[7],
                          (core_size*20)%channel_list[8],
                          (core_size*20)%channel_list[9]};

  int i, count = 0, buf_unit = 10;
  unsigned tmpsize;
  int databuf_size;
  int datatransferred = 0;
  int actual_bufsize = 10;
  int oversample = 255;
  AIORET_TYPE retval = -2;

  buf = NewAIOContinuousBufTesting( 0, actual_bufsize , buf_unit , AIOUSB_FALSE );
  AIOContinuousBuf_CreateTmpBuf(buf, 100 );
  DeleteAIOContinuousBuf(buf);
  printf("ok - Able to create and delete AIOContinuousBuf\n");

  dummy_init();
  for( i = 0 ; i < sizeof(channel_list)/sizeof(int); i ++ ) {
    count = 0;

    /** This part is all garbage ! 
     *
     * Example: 256 (sample + 255 oversamples ) * 256 number of data elements
     */
    tmpsize = core_size * ( oversample + 1 );
    /* However, we must allocate a buffer slightly large so that we can actually
       accomodate overlapping ranges */

    AIOUSB_DEVEL("Allocating tmpsize=%d\n", tmpsize );
    unsigned short *data = (unsigned short *)malloc( ( tmpsize + channel_list[i] ) * sizeof(unsigned short));
    buf_unit  = channel_list[i];

    actual_bufsize = 1000 * ( tmpsize / (oversample+1));
    buf = NewAIOContinuousBufTesting( 0, actual_bufsize , buf_unit , AIOUSB_FALSE );
    AIOContinuousBuf_InitConfiguration(buf); /* Needed to enforce Testing mode */
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf, AD_GAIN_CODE_0_5V , AIOUSB_FALSE );
    AIOContinuousBuf_SetOverSample( buf, 255 );
    AIOContinuousBuf_SetDiscardFirstSample( buf, 0 );
    datatransferred = 0;
    while ( count < repeat_count ) {
      read_data(data, tmpsize );          /* Load data with repeating data */
      retval = AIOContinuousBuf_CopyData( buf, data , &tmpsize );
      datatransferred += retval;
      if ( retval < 0 )  {
        printf("not ok - Channel_list=%d Received retval: %d\n", channel_list[i], (int)retval );
      }
      count ++; 
    }
    /* Check that the remainders are correct */
    printf("%s - Ch=%d 1st Remain=%d, expected=%d\n", ( buf->extra == expected_list[i] ? "ok" : "not ok" ),
           channel_list[i], 
           (int)buf->extra, 
           expected_list[i] );
    printf("%s - Ch=%d 1st Bufwrite=%d expected=%d\n",( datatransferred == get_write_pos(buf) ? "ok" : "not ok" ), 
           channel_list[i],  
           (int)datatransferred, 
           get_write_pos(buf)
           );
    printf("%s - Ch=%d 1st Avgd=%f expected=%f\n",  ( roundf(1000*buf->buffer[get_read_pos(buf)]) == roundf(1000*(data[0] / 65538.0)*5.0) ? "ok" : "not ok" ),
           channel_list[i],
           buf->buffer[get_read_pos(buf)], (data[0] / 65538.0)*5.0);

    /* Drain the buffer */
    datatransferred = 0;
    while ( get_read_pos(buf) != get_write_pos(buf) ) {
      datatransferred += AIOContinuousBufRead( buf, (AIOBufferType *)data, tmpsize, tmpsize );
    }
    printf("%s - Ch=%d 1st Bufread=%d expected=%d\n", ( datatransferred == get_read_pos(buf) ? "ok" : "not ok" ), 
           channel_list[i],
           (int)datatransferred, 
           get_read_pos(buf));
    
    count = 0;
    while ( count < repeat_count ) {
      memset(data,'\377', tmpsize * sizeof(short)); /* Set to 0xffff */
      retval = AIOContinuousBuf_CopyData( buf, data, &tmpsize );
      count ++;
    }
    printf("%s - Ch=%d 2nd avgd=%f expected=%f\n", ( buf->buffer[get_read_pos(buf)] == 5.0 ? "ok" : "not ok" ), channel_list[i], buf->buffer[get_read_pos(buf)], 5.0 );

    datatransferred = 0;
    prev = get_read_pos(buf);
    while ( get_read_pos(buf) != get_write_pos(buf) ) {
      datatransferred += AIOContinuousBufRead( buf, (AIOBufferType *)data, tmpsize, tmpsize );
    }
    printf("%s - Ch=%d 2nd Bufread=%d expected=%d\n", ( (datatransferred + prev) % buffer_size(buf) == get_read_pos(buf) ? "ok" : "not ok" ), channel_list[i], (datatransferred + prev) % buffer_size(buf), get_read_pos(buf));

    count = 0;
    while ( count < repeat_count ) {
      memset(data,'\0', tmpsize * sizeof(short)); /* Set to 0xffff */
      retval = AIOContinuousBuf_CopyData( buf, data, &tmpsize );
      count ++;
    }
    printf("%s - Ch=%d 3rd avgd=%f expected=%f\n", ( buf->buffer[get_read_pos(buf)] == 0.0 ? "ok" : "not ok" ), channel_list[i], buf->buffer[get_read_pos(buf)], 0.0 );

    while ( get_read_pos(buf) != get_write_pos(buf) ) {
      datatransferred += AIOContinuousBufRead( buf, (AIOBufferType *)data, tmpsize, tmpsize );
    }
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf, AD_GAIN_CODE_5V,  AIOUSB_FALSE );
    memset(data,'\0', tmpsize * sizeof(short)); /* Should set to -5 v */
    retval = AIOContinuousBuf_CopyData( buf, data, &tmpsize );
    printf("%s - Ch=%d 4th avgd=%f expected=%f\n", ( buf->buffer[get_read_pos(buf)] == -5.0 ? "ok" : "not ok" ), channel_list[i] , buf->buffer[get_read_pos(buf)], -5.0 );

    while ( get_read_pos(buf) != get_write_pos(buf) ) {
      datatransferred += AIOContinuousBufRead( buf, (AIOBufferType *)data, tmpsize, tmpsize );
    }
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf, AD_GAIN_CODE_0_2V,  AIOUSB_FALSE );
    memset(data,'\377', tmpsize * sizeof(short)); /* Should set to 2 v */
    buf->extra = 0;
    retval = AIOContinuousBuf_CopyData( buf, data, &tmpsize );
    printf("%s - Ch=%d 5th avgd=%lf expected=%lf\n", ( buf->buffer[get_read_pos(buf)] == 2.0 ? "ok" : "not ok" ), channel_list[i], buf->buffer[get_read_pos(buf)], 2.0 );
   


    /* Also show that we have the correct number of fully written packets */
    free(data);
    DeleteAIOContinuousBuf( buf );
  }
}


void bulk_transfer_test( int bufsize )
{
  AIOContinuousBuf *buf = NewAIOContinuousBuf( 0, bufsize , 16 );
  int tmpsize = pow(16,(double)ceil( ((double)log((double)(bufsize/1000))) / log(16)));
  int keepgoing = 1;
  AIORET_TYPE retval;
  unsigned long result;
  int usbval;
  libusb_device_handle *deviceHandle;
  unsigned char data[0];
  unsigned wLength = 0;
  int wValue  = 0x7400, wIndex = 0;
  unsigned timeout = 7000;
  int bytes;
  /* Write 02 00 02 00 */
  /* 40 bc 00 00 00 00 04 00 */
  AIOBufferType *tmp = (AIOBufferType *)malloc(sizeof(AIOBufferType *)*tmpsize);
  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex(buf), &result);
  if(!deviceDesc || result != AIOUSB_SUCCESS) {
    
  }
  AIOUSB_Init();
  GetDevices();

  deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  
  AIOContinuousBufSetClock( buf, 1000 );

  usbval = libusb_control_transfer( deviceHandle, 
                                    USB_WRITE_TO_DEVICE,
                                    AUR_CTR_MODE,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );
  if( usbval != AIOUSB_SUCCESS ) {
    AIOUSB_ERROR("ERROR: can't set counters\n");
    _exit(1);
  }
  wValue = 0xb600;

  /* Read c0 bc 00 00 00 00 04 00 */ 
  usbval = libusb_control_transfer( deviceHandle,
                                    USB_WRITE_TO_DEVICE,
                                    AUR_CTR_MODE,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );
  if( usbval != AIOUSB_SUCCESS ) {
    AIOUSB_ERROR("ERROR: can't set counters\n");
    _exit(1);
  }
  wValue = 100;
  wIndex = 100;

  usbval = libusb_control_transfer(deviceHandle, 
                                   USB_WRITE_TO_DEVICE, 
                                   0xC5,
                                   wValue,
                                   wIndex,
                                   data,
                                   wLength,
                                   timeout
                                   );
  if( usbval != AIOUSB_SUCCESS ) {
    AIOUSB_ERROR("ERROR: can't set divisors: %d\n",usbval);
    _exit(1);
  }
  /* usbval = libusb_bulk_transfer( deviceHandle, 0x86, data, 512, &bytes, timeout ); */
  if( usbval != AIOUSB_SUCCESS ) {
    AIOUSB_ERROR("ERROR: can't bulk acquire: %d\n", usbval );
    _exit(1);
  }


}

void stress_copy_counts (int bufsize) 
{

  unsigned char *data = (unsigned char *)malloc( bufsize ); /* Represents USB transaction, smaller in size */
  unsigned short *usdata = (unsigned short *)&data[0];
  unsigned short tobuf[32768] = {0};

  AIOContinuousBuf *buf = NewAIOContinuousBufTesting(0, bufsize , 16 , AIOUSB_TRUE ); /* Num channels is 16*bufsize  */
  AIORET_TYPE retval;
  int failed = 0;
  /* setup the data */
  memset(data,0,bufsize);
  for ( int i = 0; i < 32768 ;) { 
    for ( int ch = 0; ch < 16 && i < 32768 ; ch ++ , i ++ ) { 
      usdata[i] = (ch*20)+rand()%20;
    }
  }
  set_write_pos( buf, 16 );
  set_read_pos( buf, 0 );
  printf("%s - Minimum size is correct\n", ( AIOContinuousBufCountScansAvailable(buf) == 1 ? "ok" : "not ok" ));
  set_write_pos(buf, 0 );

  /* printf("%s - received correct number of scans left\n", AIOContinuousBufCountScansAvailable(buf) == bufsize/ 4 ? "ok" : "not ok" ); */
  set_read_pos(buf,AIOContinuousBuf_NumberChannels(buf));
  printf("%s - received correct write space left\n",  ( write_size(buf) == AIOContinuousBuf_NumberChannels(buf) ? "ok" : "not ok" ));

  /* printf("%s - Buffer Size is correct\n",  ( buffer_size(buf) == bufsize*16*sizeof(short)/sizeof(AIOBufferType) ? "ok" : "not ok" )); */
  printf("%s - Buffer Size is correct\n",  ( buffer_size(buf) == bufsize*16) ? "ok" : "not ok" );
  

  set_read_pos(buf,0);

  retval = AIOContinuousBufWriteCounts( buf, usdata, bufsize/2, bufsize/2  , AIOCONTINUOUS_BUF_ALLORNONE );
  if( retval <  0 ) {
    printf("not ok - Cant copy counts correctly\n");
  }

 
  /* printf("%s - Got expected number of Counts available\n",( AIOContinuousBufCountScansAvailable(buf) ==  bufsize/2 / 16 ? "ok" : "not ok" )); */
  printf("%s - Got expected number of Counts available\n",( AIOContinuousBufCountScansAvailable(buf) ==  bufsize / 2 / AIOContinuousBuf_NumberChannels(buf) ? "ok" : "not ok" ));

  if( AIOContinuousBufCountScansAvailable(buf)  ) { 
    retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf , 32768, AIOContinuousBuf_NumberChannels(buf)-1 );
    printf("%s - got correct response when not enough memory available\n", ( retval == -AIOUSB_ERROR_NOT_ENOUGH_MEMORY ? "ok" : "not ok" ));
  }
  


  while (  AIOContinuousBufCountScansAvailable(buf)  && !failed) {
    /* retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf , AIOContinuousBufCountScansAvailable(buf)*AIOContinuousBuf_NumberChannels(buf) ); */
    retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf , 32768, 32768 );
    if ( retval < AIOUSB_SUCCESS ) {
      printf("not ok - ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf));
    } else {
      /* unsigned short *tmpbuf = (unsigned short *)&tobuf[0]; */
      for( int i = 0, ch = 0 ; i < retval; i ++, ch = ((ch+1)% AIOContinuousBuf_NumberChannels(buf)) ) {
        if( tobuf[i] != usdata[i] ) {
          printf("not ok - got %u,  not %u\n", tobuf[i],  usdata[i] );
          failed ++;
          break;
        }
      }
    }
  }
  if( !failed ) 
    printf("ok - got matching data\n");

  /* now try writing past the end */
  int i;
  /* for ( i = 0; i < (write_size(buf) / bufsize / 2) + 1; i ++ ) { */
  /* int total_write = ( (buffer_size(buf) / 4 - get_write_pos(buf))/ ( bufsize / 8 )); */
  int total_write = write_size (buf) / ( bufsize / (AIOContinuousBuf_NumberChannels(buf) ));

  for ( i = 0; i < total_write + 2; i ++ ) {
    AIOContinuousBufWriteCounts( buf, usdata, bufsize/2, bufsize/2 , AIOCONTINUOUS_BUF_OVERRIDE );
  }
  
  /* free(data); */
  /* Read=0,Write=16384,size=4000000,Avail=4096; */
  DeleteAIOContinuousBuf(buf);
  /* --buffersize 1000000 --numchannels 16  --clockrate 10000; */
  buf = NewAIOContinuousBufTesting( 0, 1000000, 16 , AIOUSB_TRUE );
  /* set_write_pos(buf, 16384 ); */
  memset(usdata,0,bufsize/2);
  AIOContinuousBufWriteCounts( buf, usdata, bufsize/2,bufsize/2 , AIOCONTINUOUS_BUF_OVERRIDE );
  failed = 0;
  while (  AIOContinuousBufCountScansAvailable(buf)  && !failed ) {
      /* retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf , AIOContinuousBufCountScansAvailable(buf)*AIOContinuousBuf_NumberChannels(buf) ); */
    retval = AIOContinuousBufReadIntegerScanCounts( buf, tobuf , 32768, 32768 );
      if ( retval < AIOUSB_SUCCESS ) {
          printf("not ok - ERROR reading from buffer at position: %d\n", AIOContinuousBufGetReadPosition(buf));
      } else {
          unsigned short *tmpbuf = (unsigned short *)&tobuf[0];
          for( int i = 0, ch = 0 ; i < retval; i ++, ch = ((ch+1)% AIOContinuousBuf_NumberChannels(buf)) ) {
              if( tobuf[i] != usdata[i] ) {
                  printf("not ok - got %u,  not %u\n", tobuf[i],  usdata[i] );
                  failed ++;
                  break;
              }
          }
      }
  }
  DeleteAIOContinuousBuf(buf);
  buf = NewAIOContinuousBufTesting( 0, 10, 16 , AIOUSB_TRUE );
  retval = AIOContinuousBufWriteCounts( buf, usdata, bufsize/2,bufsize/2 , AIOCONTINUOUS_BUF_ALLORNONE ); 
  printf("%s - Able to prevent writes when not enough space\n", ( retval < 0 ? "ok" : "not ok" ));
  /* figure out how to add only the amount we can */
  /* int tmpsize = MIN( write_size_counts(buf), bufsize/2 ); */
  /* int tmpsize = MIN( write_size_num_scan_counts(buf) * AIOContinuousBuf_NumberChannels(buf), bufsize/2); */
  unsigned tmpsize = MIN( AIOContinuousBuf_NumberWriteScansInCounts(buf) , bufsize/2 );
  retval = AIOContinuousBufWriteCounts( buf, usdata, bufsize/2,tmpsize , AIOCONTINUOUS_BUF_ALLORNONE ); 
  printf("%s - Able to write just enough\n", ( retval == tmpsize ? "ok" : "not ok" ));

  DeleteAIOContinuousBuf(buf); 
  free(data);

}


int main(int argc, char *argv[] )
{
  
  AIORET_TYPE retval;

  printf("1..193\n");
  int bufsize = 10000;
#if 1
  basic_functionality();
  for( int i = bufsize; i > 1 ; i /= 2 ) {
    /* printf("Using i:%d\n",i); */
    stress_test_one( bufsize , bufsize - bufsize / i);
  }

  bufsize = 1000006;
  for( int i = bufsize; i > 1 ; i /= 2 ) {
    stress_test_one( bufsize , bufsize - bufsize / i);
  }

  bufsize = 20000;
#endif
#if 1
  for ( int i = 1 , j = 1; i < 20 ; j*=2 , i += 1) {
    stress_test_read_channels( bufsize, j );
  }

  bufsize = 1000;

  /* now a test using a different number of channels */

  stress_test_drain_buffer( bufsize );  
#endif  
  bufsize = 65536;
  stress_copy_counts( bufsize );

  /* bulk_transfer_test( bufsize ); */
  /* continuous_stress_test( bufsize ); */
 

}

#endif


