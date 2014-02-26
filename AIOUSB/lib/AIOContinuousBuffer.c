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
 *        a data structure known as the AIOContinuousBuf that is implemented as a circular buffer.
 *        
 * @todo Make the number of channels in the ContinuousBuffer match the number of channels in the
 *       config object
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aiousb.h"
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


/**
 * @desc Constructor for AIOContinuousBuf object. Will set up the 
 * @param bufsize 
 * @param num_channels 
 * @return 
 * @todo Needs a smarter constructor for specifying the Initial mask .Currently won't work
 *       for num_channels > 32
 */
AIOContinuousBuf *NewAIOContinuousBufWithoutConfig( unsigned long DeviceIndex, int bufsize , unsigned num_channels )
{
  assert( num_channels > 0 );
  AIOContinuousBuf *tmp  = (AIOContinuousBuf *)malloc(sizeof(AIOContinuousBuf));
  tmp->mask              = NewAIOChannelMask( num_channels );

  /**
   * @todo fix this to accomodate larger masks, use AIOChannelMask_SetMaskFromChar
   */
  if ( num_channels > 32 ) { 
    char *bitstr = (char *)malloc( num_channels +1 );
    memset(bitstr, 49, num_channels ); /* Set all to 1s */
    bitstr[num_channels] = '\0';
    AIOChannelMask_SetMaskFromStr( tmp->mask, bitstr );
    free(bitstr);
  } else {
    AIOChannelMask_SetMaskFromInt( tmp->mask, (unsigned)-1 >> (BIT_LENGTH(unsigned)-num_channels),0 ); /**< Use all bits for each
                                                                                                          channel */
  }
  tmp->size        = num_channels * bufsize;
  tmp->buffer      = (AIOBufferType *)malloc( tmp->size *sizeof(AIOBufferType ));
  tmp->_read_pos   = 0;
  tmp->DeviceIndex = DeviceIndex;
  tmp->_write_pos  = 0;
  tmp->status      = NOT_STARTED;
  tmp->worker      = cont_thread;
  tmp->hz          = 100000; /**> Default value of 100khz  */
  tmp->timeout     = 1000;   /**> Defautl Timeout of 1000us  */
  tmp->extra       = 0;
#ifdef HAS_PTHREAD
  tmp->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;   /* Threading mutex Setup */
#endif

  return tmp;
}


AIOContinuousBuf *NewAIOContinuousBuf( unsigned long DeviceIndex , int bufsize , unsigned num_channels )
{
  AIOContinuousBuf *tmp = NewAIOContinuousBufWithoutConfig( DeviceIndex,  bufsize, num_channels );
  ADConfigBlock config;
  ADC_InitConfigBlock( &config, (void *)&deviceTable[DeviceIndex], AD_MAX_CONFIG_REGISTERS - 1 );
  AIOUSB_SetConfigBlock( AIOContinuousBuf_GetDeviceIndex( tmp ), &config );
  return tmp;
}

AIOContinuousBuf *NewAIOContinuousBufTesting( unsigned long DeviceIndex , int bufsize , unsigned num_channels )
{
  AIOContinuousBuf *tmp = NewAIOContinuousBufWithoutConfig( DeviceIndex,  bufsize, num_channels );
  ADConfigBlock config;
  ADC_InitConfigBlock( &config, (void *)&deviceTable[DeviceIndex], AD_MAX_CONFIG_REGISTERS - 1 );
  config.testing = AIOUSB_TRUE;
  AIOUSB_SetConfigBlock( AIOContinuousBuf_GetDeviceIndex( tmp ), &config );
  return tmp;
}






/**
 * @desc Destructor for AIOContinousBuf object
 * @param buf 
 */
void DeleteAIOContinuousBuf( AIOContinuousBuf *buf )
{
  free( buf->buffer );
  free( buf );
}


void AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) )
{
  AIOContinuousBufLock( buf );
  buf->callback = work;
  AIOContinuousBufUnlock( buf );
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

unsigned buffer_size( AIOContinuousBuf *buf )
{
  return buf->size;
}


/**
 * @desc Determines the remaining space in the buffer for writing.
 * @param buf 
 * @return 
 * @note Total bytes that can be written into the buffer are N-1
 * @note
 *  - if the abs(_write_pos - read_pos ) = Size - 1 , then our value is 
 *  0 
 *  - if the abs( _write_pos - _read_pos ) = 0 , then our value is Size
 */

unsigned write_size( AIOContinuousBuf *buf ) {
  unsigned retval = 0;
  unsigned read, write;
  read = (unsigned )get_read_pos(buf);
  write = (unsigned)get_write_pos(buf);
 if( read > write ) {
   retval =  read - write;
 } else {
   /* retval = write - read; */
   return buffer_size(buf) - (get_write_pos (buf) - get_read_pos (buf));
 }
 return retval;
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

unsigned int AIOContinuousBufGetReadPosition( AIOContinuousBuf *buf )
{
  return get_read_pos( buf );
}

unsigned int AIOContinuousBufGetWritePosition( AIOContinuousBuf *buf )
{
 return get_write_pos( buf );
}

unsigned AIOContinuousBufAvailableReadSize( AIOContinuousBuf *buf )
{
  return read_size(buf);
}

unsigned buffer_max( AIOContinuousBuf *buf )
{
  return buf->size-1;
}

void AIOContinuousBufReset( AIOContinuousBuf *buf )
{
  AIOContinuousBufLock( buf );
  set_read_pos(buf, 0 );
  set_write_pos(buf, 0 );
  AIOContinuousBufUnlock( buf );
}

/* void *(*g)(void *obj) */
/* AIOUSB_WorkFn */
/*        AIOContinuousBuf_GetCallback( AIOContinuousBuf *buf ) */

/** 
 * @desc Returns 
 * @param buf 
 * @return Pointer to our work function
 */
AIOUSB_WorkFn AIOContinuousBuf_GetCallback( AIOContinuousBuf *buf )
{
  return buf->callback;
}

void AIOContinuousBufSetClock( AIOContinuousBuf *buf, unsigned int hz )
{
  buf->hz = hz;
}


/**
 * @desc Starts the work function
 * @param buf 
 * @param work 
 * @return status code of start.
 */
AIORET_TYPE
AIOContinuousBufStart( AIOContinuousBuf *buf )
{
  /* AIORET_TYPE ret; */
  AIORET_TYPE retval;

#ifdef HAS_PTHREAD
  buf->status = RUNNING;

  retval = pthread_create( &(buf->worker), NULL, buf->callback, (void *)buf );

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
AIORET_TYPE CalculateClocks( AIOContinuousBuf *buf )
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
          AIOUSB_LOG( "Found new error: using lv=%d\n", (int)lv);
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
AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf )
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
 AIORET_TYPE copy_integer_number_bytes( AIOContinuousBuf *buf,  unsigned short *data , unsigned *size)
 {
     unsigned i = 0, write_count = 0;
     AIORET_TYPE retval;
     unsigned tmpcount, channel, pos = 0;
     unsigned stopval=0;
     unsigned long DeviceIndex = AIOContinuousBuf_GetDeviceIndex( buf );
     unsigned number_channels = AIOContinuousBuf_NumberChannels(buf);
     AIOBufferType *tmpbuf = (AIOBufferType *)malloc( (number_channels+256)*sizeof(AIOBufferType));
     int core_size = 256;
     unsigned tmpsize = *size;

     cull_and_average_counts( DeviceIndex, data, &tmpsize, AIOContinuousBuf_NumberChannels(buf) );

     /**
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
      */ 
     if( buf->extra ) {

       for ( channel = (number_channels - buf->extra), i = core_size; i < core_size + buf->extra ; i += 1 ) {
         AIOUSB_ArrayCountsToVolts( DeviceIndex,
                                    channel,
                                    1,
                                    &data[i],
                                    (double*)&tmpbuf[pos++]
                                    );
         channel = ( channel + 1 ) % number_channels;
       }
       write_count = buf->extra;
       for ( i = 0 ; write_count < number_channels; i ++ ) {
         AIOUSB_ArrayCountsToVolts( DeviceIndex,
                                    channel,
                                    1,
                                    &data[i],
                                    (double*)&tmpbuf[pos++]
                                    );
                                 
         write_count ++;
         channel = ( channel + 1 ) % number_channels;
       }
     } /* Completed one channel range from the extra packets */
     tmpcount = write_count - buf->extra;
     i = tmpcount;
     /* for ( channel = 0; i < (((512 - tmpcount * 2))/number_channels)*number_channels ; i += 1 ) { */
     stopval = ((core_size - tmpcount)/number_channels)*number_channels;
     for ( channel = 0; i < stopval ; i ++ ) {
       /**
        * Assign value while at the same time,
        * determine if we need to write the packets to the AIOContinuousBuf
        * or not */
       AIOUSB_ArrayCountsToVolts( DeviceIndex,
                                  channel,
                                  1,
                                  &data[i],
                                  (double*)&tmpbuf[pos++]
                                  );
       write_count ++;
       channel = ( channel + 1 ) % number_channels;
     }
     buf->extra = (core_size - i - tmpcount);

     AIOUSB_DEVEL( "After write: #Channels: %d, Wrote %d full channels, Extra %d\n", number_channels,write_count / number_channels , buf->extra );

     retval = AIOContinuousBufWrite( buf, (AIOBufferType *)tmpbuf, write_count, AIOCONTINUOUS_BUF_ALLORNONE );

     free(tmpbuf);
     return (AIORET_TYPE)retval;
 }

/**
 * @desc Copies data from the USB buffer into the Continuous Buffer
 * @param buf 
 * @param data 
 * @return success if >= 0 , failure otherwise
 */
AIORET_TYPE AIOContinuousBuf_CopyData( AIOContinuousBuf *buf , unsigned short *data , unsigned *size)
{
  AIORET_TYPE retval;
  assert( buf->DeviceIndex >= 0 );
  retval = copy_integer_number_bytes( buf, data, size);
  return retval;

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
  sched_yield();
  AIOContinuousBuf *buf = (AIOContinuousBuf*)object;
  int  bufsize = 256;
  unsigned long result;
  int bytes;
  AIOBufferType *tmpbuf = (AIOBufferType *)malloc( bufsize * sizeof(AIOBufferType));
  /* int numrepeat = 30; */
  /* unsigned char *data = (unsigned char *)malloc(numrepeat * bufsize * sizeof(AIOBufferType)); */
  unsigned char *data = (unsigned char *)malloc( bufsize * sizeof(AIOBufferType));
  unsigned timeout = 7000;
  int position = 0;

  DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex(buf), &result);
  if(!deviceDesc || result != AIOUSB_SUCCESS) {
    retval = -result;
    goto out_ActualWorkFunction;
  }


 
  while ( buf->status == RUNNING ) { 

    AIOUSB_DEVEL("Doing something here\n");
    
    usbresult = libusb_bulk_transfer( AIOUSB_GetDeviceHandle( AIOContinuousBuf_GetDeviceIndex( buf )),
                                      0x86,
                                      &data[position*16],
                                      512,
                                      &bytes,
                                      timeout );

    AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , bytes);
    /* need to convert multiple entries into several voltages */
    if ( usbresult  == 0 ) {
      for( int i = 0; i < 512 / 2  / 16; i ++ ) { 
        AIOUSB_MultipleCountsToVolts( AIOContinuousBuf_GetDeviceIndex( buf ),
                                      0, 15,
                                      (unsigned short *)&data[i*16*sizeof(unsigned short)],
                                      (double *)tmpbuf
                                      );


        retval = AIOContinuousBufWrite( buf, (AIOBufferType *)tmpbuf, 16, AIOCONTINUOUS_BUF_ALLORNONE );
        if( retval < AIOUSB_SUCCESS ){
          AIOUSB_ERROR("Error writing to Actual AIOContinuousBuf\n");
        } else {
          AIOUSB_DEVEL("Wrote to buf\n");
        }
      }
#ifdef TESTING
      if( data[0] > 0 ) {
        counter ++ ;
        /* position ++; */
      }
      if( counter > 20 ) {
        usleep(4);
        counter = 0;
      }
#endif
    }
  }
out_ActualWorkFunction:
  AIOUSB_DEVEL("Stopping\n");
  AIOContinuousBufCleanup( buf );
  free(tmpbuf);
  pthread_exit((void*)&retval);

}


/* void *ActualWorkFunction( void *object ) */
/* { */
/*   AIORET_TYPE retval; */
/*   int usbresult; */
/*   sched_yield(); */
/*   AIOContinuousBuf *buf = (AIOContinuousBuf*)object; */
/*   unsigned long result; */
/*   int buflength = 512 + AIOContinuousBuf_NumberChannels(buf); /\**< Extra length is for keeping extra elements *\/ */
/*   int bytes; */
/*   AIORET_TYPE size; */

/*   unsigned short *data   = (unsigned short*)malloc( buflength ); */
/*   unsigned timeout = 7000; */
/*   /\* int rollover; *\/ */
/*   buf->extra = 0; */
/*   DeviceDescriptor *deviceDesc = AIOUSB_GetDevice_Lock( AIOContinuousBuf_GetDeviceIndex(buf), &result); */
/*   if(!deviceDesc || result != AIOUSB_SUCCESS) { */
/*     retval = -result; */
/*     goto out_ActualWorkFunction; */
/*   } */
 
/*   while ( buf->status == RUNNING ) {  */

/*     AIOUSB_DEVEL("Doing something here\n"); */
    
/*     usbresult = libusb_bulk_transfer( AIOUSB_GetDeviceHandle( AIOContinuousBuf_GetDeviceIndex( buf )), */
/*                                       0x86, */
/*                                       (unsigned char *)&data[0], */
/*                                       /\* &data[position*16], *\/ */
/*                                       512, */
/*                                       &bytes, */
/*                                       timeout ); */

/*     AIOUSB_DEVEL("libusb_bulk_transfer returned  %d as usbresult, bytes=%d\n", usbresult , bytes); */
/*     /\* need to convert multiple entries into several voltages *\/ */
/*     if ( usbresult  == 0 ) { */

/*       /\* rollover = 512 / AIOContinuousBuf_NumberChannels( buf ); *\/ */
/*       for ( unsigned i = 0 ; i < 512 / 2 / AIOContinuousBuf_NumberChannels( buf ); i ++ ) {        */

/*         /\* size = copy_integer_number_bytes( buf, offset, data, buf_unit, &extra ); *\/ */
/*         size = AIOContinuousBuf_CopyData( buf , data  ); */

/*         if ( size < 0 ) { */
/*           AIOUSB_ERROR("Error writing to Actual AIOContinuousBuf\n"); */
/*         } else { */
/*           AIOUSB_DEVEL("Wrote to buf\n"); */
/*         } */
/*         /\* for( int i = 0; i < 512 / 2 / 16; i ++ ) {  *\/ */
/*         /\* AIOUSB_MultipleCountsToVolts( AIOContinuousBuf_GetDeviceIndex( buf ), *\/ */
/*         /\*                               0, 15, *\/ */
/*         /\*                               (unsigned short *)&data[i*16*sizeof(unsigned short)], *\/ */
/*         /\*                               (double *)tmpbuf *\/ */
/*         /\*                               ); *\/ */

/*       } */
/* #ifdef TESTING */
/*       if( data[0] > 0 ) { */
/*         counter ++ ; */
/*         /\* position ++; *\/ */
/*       } */
/*       if( counter > 20 ) { */
/*         usleep(4); */
/*         counter = 0; */
/*       } */
/* #endif */
/*     } */
/*   } */
/* out_ActualWorkFunction: */
/*   AIOUSB_DEVEL("Stopping\n"); */
/*   AIOContinuousBufCleanup( buf ); */
/*   /\* free(tmpbuf); *\/ */
/*   pthread_exit((void*)&retval); */

/* } */

/* AIOBufferType *tmpbuf = (AIOBufferType *)malloc( bufsize * sizeof(AIOBufferType)); */
/* int numrepeat = 30; */
/* unsigned char *data = (unsigned char *)malloc(numrepeat * bufsize * sizeof(AIOBufferType)); */
/* unsigned char *data = (unsigned char *)malloc( AIOContinuousBuf_NumberChannels(buf) * sizeof(AIOBufferType)); */


AIORET_TYPE AIOContinuousBufLoadCounters( AIOContinuousBuf *buf, unsigned countera, unsigned counterb )
{
  AIORET_TYPE retval = AIOUSB_SUCCESS;
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  unsigned wValue = countera;
  unsigned wIndex = counterb;
  unsigned wLength = 0;
  unsigned char data[0];
  unsigned timeout = 3000;
  int usbval = libusb_control_transfer(deviceHandle, 
                                       USB_WRITE_TO_DEVICE, 
                                       0xC5,
                                       wValue,
                                       wIndex,
                                       data,
                                       wLength,
                                       timeout
                                       );
  if ( usbval != 0 ) {
    retval = -usbval;
  }
  return retval;
}


AIORET_TYPE AIOContinuousBufSetupCounters( AIOContinuousBuf *buf ) 
{
  AIORET_TYPE retval;
  /* int BlockIndex = 0; */

  retval = (AIORET_TYPE)CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 1, 2); /* Counter 1, Mode 2 */
  if( retval != AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufSetupCounters;

  retval = (AIORET_TYPE)CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 2, 3); /* Counter 2, Mode 3 */
  if( retval != AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufSetupCounters;

out_AIOContinuousBufSetupCounters:
  return retval;

}

AIORET_TYPE AIOContinuousBufCleanup( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval;
  int usbval;
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  unsigned char data[4] = {2,0,2,0};
  unsigned wLength = 4;
  int wValue  = 0, wIndex = 0;
  unsigned timeout = 7000;
  /* Write 02 00 02 00 */
  /* 40 bc 00 00 00 00 04 00 */
  usbval = libusb_control_transfer( deviceHandle, 
                                    USB_WRITE_TO_DEVICE,
                                    AUR_START_ACQUIRING_BLOCK,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );

  /* Read bc */
  wLength = 0;
  data[0] = 0;
  usbval = libusb_control_transfer( deviceHandle, 
                                    USB_READ_FROM_DEVICE,
                                    AUR_START_ACQUIRING_BLOCK,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );


  if (usbval != 0 ) {
    retval = -(usbval);
    goto out_AIOContinuousBufCleanup;
  }

  /* Read c0 bc 00 00 00 00 04 00 */ 
  usbval = libusb_control_transfer( deviceHandle,
                                    USB_READ_FROM_DEVICE,
                                    AUR_START_ACQUIRING_BLOCK,
                                    wValue,
                                    wIndex,
                                    data,
                                    wLength,
                                    timeout
                                    );
  if (usbval != 0 )
    retval = -(usbval);
out_AIOContinuousBufCleanup:
  return retval;

}

AIORET_TYPE
AIOContinuousBufPreSetup( AIOContinuousBuf * buf )
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


/** 
 * @desc Setups the Automated runs for continuous mode runs
 * @param buf 
 * @return 
 */
AIORET_TYPE AIOContinuousBufCallbackStart( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval;

  /** 
   * Setup counters
   * see reference in [USB AIO documentation](http://accesio.com/MANUALS/USB-AIO%20Series.PDF)
   **/
  if( buf->counter_control ) { 
    AIOContinuousBufSetupCounters( buf );
  } 
  
  AIOContinuousBuf_SetCallback( buf , ActualWorkFunction );

  /* Start the clocks, and need to get going capturing data
   */
  if( (retval = AIOContinuousBufPreSetup( buf )) != AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufCallbackStart;

  if ( (retval = CalculateClocks( buf ) ) != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStart;

  if( ( retval  = AIOContinuousBufLoadCounters( buf, buf->divisora, buf->divisorb )) != AIOUSB_SUCCESS)
    goto out_AIOContinuousBufCallbackStart;

  retval = AIOContinuousBufStart( buf ); /**< Fills up the buf buffer  */
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

/** 
 * @desc Reads the current available amount of data from buf, into 
 *       the readbuf datastructure
 * @param buf 
 * @param readbuf 
 * @return If number is positive, it is the number of bytes that have been read.
 */
AIORET_TYPE AIOContinuousBufRead( AIOContinuousBuf *buf, AIOBufferType *readbuf , unsigned size)
{

  AIORET_TYPE retval;
  
  unsigned basic_copy , wrap_copy ;
  
  /* First lock the Buffer */
  AIOContinuousBufLock( buf );
  if( get_read_pos(buf) <= get_write_pos(buf) ) {
    basic_copy = MIN(size, get_write_pos(buf) - get_read_pos( buf ));
    wrap_copy  = 0;
  } else {
    basic_copy = MIN(size, buffer_size(buf) - get_read_pos(buf));
    wrap_copy  = MIN(size - basic_copy, get_write_pos(buf) );
  }
  /* Now copy the data into readbuf */

  memcpy( &readbuf[0]          , &buf->buffer[get_read_pos(buf)]  , basic_copy*sizeof(AIOBufferType) );
  memcpy( &readbuf[basic_copy] , &buf->buffer[0]                  , wrap_copy*sizeof(AIOBufferType)  );
  
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
 * @desc Allows one to write in to the buffer of a given size
 * @param buf 
 * @param writebuf 
 * @param size 
 * @param flag
 * @return Status of whether the write was successful , if so returning the number of bytes written
 *         or if there was insufficient space, it returns negative error code. If the number is >= 0, then 
 *         this corresponds to the number of bytes that were written into the buffer.
 */
AIORET_TYPE AIOContinuousBufWrite( AIOContinuousBuf *buf, AIOBufferType *writebuf, unsigned size, AIOContinuousBufMode flag )
{
  AIORET_TYPE retval;
  unsigned basic_copy, wrap_copy;
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
      basic_copy = MIN( size, ( get_read_pos( buf ) - get_write_pos( buf ) - 1 ));
      wrap_copy  = 0;
    } else {
      basic_copy = MIN( size, ( buffer_max(buf) - get_write_pos( buf ) ));
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
  
  memcpy( &buf->buffer[ get_write_pos( buf ) ] , &writebuf[0]           , basic_copy*sizeof(AIOBufferType) );
  memcpy( &buf->buffer[ 0 ]                    , &writebuf[basic_copy]  , wrap_copy*sizeof(AIOBufferType)  );
  
  set_write_pos( buf, (get_write_pos(buf) + basic_copy + wrap_copy ) % buf->size );
  retval = basic_copy+wrap_copy;

  /* If the flag is set such that we can
   * overwrite , then we are ok, otherwise, 
   * let's do something different */

 out_AIOContinuousBufWrite:
  AIOContinuousBufUnlock( buf );
  return retval;
}

/** 
 * @desc 
 * @param buf 
 * @return 
 */
AIORET_TYPE AIOContinuousBufLock( AIOContinuousBuf *buf )
{
  AIORET_TYPE retval = 0;
#ifdef HAS_PTHREAD
  retval = pthread_mutex_lock( &buf->lock );
  /* retval = pthread_mutex_lock( &lock ); */
  if ( retval != 0 ) {
    retval = -retval;
  }
#endif
  return retval;
}
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf )
{
  int retval = 0;
  /* int retval = 0; */
#ifdef HAS_PTHREAD
  retval = pthread_mutex_unlock( &buf->lock );
  /* retval = pthread_mutex_unlock( &lock ); */
  if ( retval !=  0 ) {
    retval = -retval; 
    AIOUSB_ERROR("Unable to unlock mutex");
  }
#endif
  return retval;
}

AIORET_TYPE AIOContinuousBufReadChannels( AIOContinuousBuf *buf , AIOBufferType *readbuf, unsigned bufsize )
{
  unsigned actbufsize = bufsize  / 16;
  AIORET_TYPE retval;

  if ( actbufsize < bufsize )
    AIOUSB_WARN("Truncating buffer size to be multiple of 16 channels\n");
  if ( actbufsize < 0 ) {
    retval = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
    goto out_AIOContinuousBufReadChannels;
  }

  retval = AIOContinuousBufRead( buf, readbuf, actbufsize );

out_AIOContinuousBufReadChannels:
  return retval;

}

AIORET_TYPE AIOContinuousBufSimpleSetupConfig( AIOContinuousBuf *buf, ADGainCode gainCode )
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


AIORET_TYPE AIOContinuousBufEnd( AIOContinuousBuf *buf )
{ 
  /* int *retval; */
  void *ptr;
  AIORET_TYPE ret;
  AIOContinuousBufLock( buf );

  AIOUSB_DEVEL("Locking and finishing thread\n");

  buf->status = TERMINATED;
  AIOUSB_DEVEL("\tWaiting for thread to terminate\n");
  /* AIOContinuousBufTerminate( buf ); */
  AIOUSB_DEVEL("Set flag to FINISH\n");

  AIOContinuousBufUnlock( buf );


#ifdef HAS_PTHREAD
  ret = pthread_join( buf->worker , &ptr );
  /* ret = pthread_join( worker, &ptr ); */
#endif
  if ( ret != 0 ) {
    AIOUSB_ERROR("Error joining threads");
  }
  buf->status = JOINED;
  return ret;
}

void AIOContinousBuf_SetTesting( AIOContinuousBuf *buf, AIOUSB_BOOL testing )
{
  AIOContinuousBufLock( buf );
  ADC_SetTestingMode( AIOUSB_GetConfigBlock( AIOContinuousBuf_GetDeviceIndex(buf)), testing );
  AIOContinuousBufUnlock( buf );
}


void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex )
{
  AIOContinuousBufLock( buf );
  buf->DeviceIndex = DeviceIndex; 
  AIOContinuousBufUnlock( buf );
}

void AIOContinuousBuf_SetOverSample( AIOContinuousBuf *buf, unsigned os )
{
  AIOContinuousBufLock( buf );
  ADC_SetOversample( AIOContinuousBuf_GetDeviceIndex(buf), os );
  AIOContinuousBufUnlock( buf );
}

void AIOContinuousBuf_SetAllGainCodeAndDiffMode( AIOContinuousBuf *buf, ADGainCode gain, AIOUSB_BOOL diff )
{
  AIOContinuousBufLock( buf );
  ADC_SetAllGainCodeAndDiffMode( AIOContinuousBuf_GetDeviceIndex( buf ), gain , diff );
  AIOUSB_UnLock();
  AIOContinuousBufUnlock( buf );
}

void AIOContinuousBuf_SetDiscardFirstSample(  AIOContinuousBuf *buf , AIOUSB_BOOL discard ) 
{
  AIOContinuousBufLock( buf );
  AIOUSB_SetDiscardFirstSample( AIOContinuousBuf_GetDeviceIndex(buf), discard );
  AIOContinuousBufUnlock( buf );
}



unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf )
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
    retval = AIOContinuousBufWrite( buf, tmp, size , AIOCONTINUOUS_BUF_NORMAL );
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
    retval = AIOContinuousBufWrite( buf, tmp, size , AIOCONTINUOUS_BUF_ALLORNONE );
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
    retval = AIOContinuousBufRead( buf,  readbuf, readbuf_size );
    usleep(rand() % 100);
    AIOUSB_DEVEL("Read number of bytes=%d\n",(int)retval );
  }
  AIOContinuousBufEnd( buf );
  int distance = ( get_read_pos(buf) > get_write_pos(buf) ? 
                   (buffer_max(buf) - get_read_pos(buf) ) + get_write_pos(buf) :
                   get_write_pos(buf) - get_read_pos(buf) );

  AIOUSB_DEVEL("Read: %d, Write: %d\n", get_read_pos(buf),get_write_pos(buf));
  for( int i = 0; i <= distance / readbuf_size ; i ++ ) {
    retval = AIOContinuousBufRead( buf, readbuf, readbuf_size );
  }
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size );
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
  retval = AIOContinuousBufWrite( buf, tmp , tmpsize , AIOCONTINUOUS_BUF_ALLORNONE  );
  printf("%s", ( (int)retval == -AIOUSB_ERROR_NOT_ENOUGH_MEMORY ? "ok" : "not ok" ));
  printf(" - Able to perform first write, count is %d \n", (int)retval );
  
  free(tmp);
  
  unsigned size = 4999;
  tmp = (AIOBufferType *)malloc(size*sizeof(AIOBufferType ));
  for( int i = 0; i < 3; i ++ ) {
    for( int j = 0 ; j < size; j ++ ) {
      tmp[j] = rand() % 1000;
    }
    retval = AIOContinuousBufWrite( buf, tmp , size , AIOCONTINUOUS_BUF_ALLORNONE  );
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
  retval = AIOContinuousBufWrite( buf, tmp , size , AIOCONTINUOUS_BUF_NORMAL  );
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - able to write, count is %d\n", get_write_pos(buf) );
  
  retval = AIOContinuousBufWrite( buf, tmp , size , AIOCONTINUOUS_BUF_OVERRIDE );
  printf("%s", ( (int)retval != 0 ? "ok" : "not ok" ));
  printf(" - Correctly writes with override \n");
  
  int readbuf_size = size - 10;
  AIOBufferType *readbuf = (AIOBufferType *)malloc( readbuf_size*sizeof(AIOBufferType ));
  
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size );
  printf("%s", ( (int)retval != 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");

  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size );
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");


  free(tmp);
  size = 6000;
  tmp = (AIOBufferType *)malloc(size*sizeof(AIOBufferType ));
  for( int j = 0 ; j < size; j ++ ) {
    tmp[j] = rand() % 1000;
  }
  retval = AIOContinuousBufWrite( buf, tmp , size , AIOCONTINUOUS_BUF_NORMAL);
  printf("%s", ( (int)retval >= 0 ? "ok" : "not ok" ));
  printf(" - Able to read correctly \n");

  free(readbuf);
  readbuf_size = (  buffer_max(buf) - get_read_pos (buf) + 2000 );
  readbuf = (AIOBufferType *)malloc(readbuf_size*sizeof(AIOBufferType ));
  retval = AIOContinuousBufRead( buf, readbuf, readbuf_size );
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
    retval = AIOContinuousBufRead( buf, tmp, mybufsize );
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
    retval = AIOContinuousBufRead( buf, tmp, mybufsize );
  }
  retval = AIOContinuousBufRead( buf, tmp, mybufsize );

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
    retval = AIOContinuousBufRead( buf, tmp, tmpsize );
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
    /* data[i] = (unsigned short)(rand() % 0xffff); */
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
  deviceDesc->ADCChannelsPerGroup = 1;
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

  int i, count = 0, buf_unit;
  int tmpsize;
  AIORET_TYPE retval = -2;


  dummy_init();
  for( i = 0 ; i < sizeof(channel_list)/sizeof(int); i ++ ) {
    count = 0;
    /* tmpsize = (core_size+channel_list[i])*sizeof(unsigned short); */
    tmpsize = (channel_list[i] * oversamples[0]* core_size);
    AIOUSB_DEVEL("Allocating tmpsize=%d\n", tmpsize );
    /**
     * Make a buffer that includes many channels plus 
     */
    unsigned short *data = (unsigned short *)malloc(tmpsize *sizeof(unsigned short) );

    buf_unit  = channel_list[i];

    buf = NewAIOContinuousBufTesting( 0, bufsize , buf_unit );
    AIOContinuousBuf_SetAllGainCodeAndDiffMode( buf, AD_GAIN_CODE_0_5V , AIOUSB_FALSE );
    AIOContinuousBuf_SetOverSample( buf, 255 );
    AIOContinuousBuf_SetDiscardFirstSample( buf, 0 );

    while ( count < 20 ) {

      read_data(data, tmpsize );          /* Load data with repeating data */
      retval = AIOContinuousBuf_CopyData( buf, data , &tmpsize );
      if ( retval < 0 )  {
        printf("not ok - Received retval: %d\n", (int)retval );
      }
      count ++; 
    }
    /* Check that the remainders are correct */
    printf("%s - Remain=%d, expected=%d\n", ( buf->extra == expected_list[i] ? "ok" : "not ok" ), 
           (int)buf->extra, expected_list[i] );
    /* prove able to drain the buffer */

    /* Also show that we have the correct number of fully written packets */
    free(data);
    DeleteAIOContinuousBuf( buf );
  }
}
/* buf = NewAIOContinuousBuf */
/* AIOContinuousBuf_SetDeviceIndex( buf, 0 ); */
/* AIOContinousBuf_SetTesting( buf, 1 ); */
/* Three extra functions to help set default behavior */
/* AIOContinuousBuf_SetConfig( buf ); */

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
  usbval = libusb_bulk_transfer( deviceHandle, 0x86, data, 512, &bytes, timeout );
  if( usbval != AIOUSB_SUCCESS ) {
    AIOUSB_ERROR("ERROR: can't bulk acquire: %d\n", usbval );
    _exit(1);
  }


}


int main(int argc, char *argv[] )
{
  
  AIORET_TYPE retval;

  /* AIOContinuousBuf_SetDeviceIndex( buf, 0 ); */
  /* AIOContinuousBuf_SetCallback( buf , doit ); */

  printf("1..104\n");
  int bufsize = 10000;
#if 0
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

  for ( int i = 1 , j = 1; i < 20 ; j*=2 , i += 1) {
    stress_test_read_channels( bufsize, j );
  }

  bufsize = 1000;

  /* now a test using a different number of channels */
#endif  
  stress_test_drain_buffer( bufsize );  


  /* bulk_transfer_test( bufsize ); */
  /* continuous_stress_test( bufsize ); */
 

}

#endif
