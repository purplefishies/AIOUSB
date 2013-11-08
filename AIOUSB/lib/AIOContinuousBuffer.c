#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aiousb.h"
#include "AIOTypes.h"
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

pthread_t worker;
pthread_mutex_t lock;
pthread_attr_t tattr;


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
#define AIOUSB_DEVEL(...)  AIOUSB_LOG( "<Devel>\t" __VA_ARGS__ )
#define AIOUSB_DEBUG(...)  AIOUSB_LOG( "<Debug>\t" __VA_ARGS__ )
#else
#define AIOUSB_DEVEL( ... ) if ( 0 ) { }
#define AIOUSB_DEBUG( ... ) if ( 0 ) { }
#endif

#define AIOUSB_WARN(...)   AIOUSB_LOG("<Warn>\t"  __VA_ARGS__ )
#define AIOUSB_ERROR(...)  AIOUSB_LOG("<Error>\t" __VA_ARGS__ )
#define AIOUSB_FATAL(...)  AIOUSB_LOG("<Fatal>\t" __VA_ARGS__ )



AIOContinuousBuf *NewAIOContinuousBuf( int bufsize )
{
  AIOContinuousBuf *tmp  = (AIOContinuousBuf *)malloc(sizeof(AIOContinuousBuf));

#ifdef BUFFER_ANYSIZE
  tmp->size              = bufsize;
#else
  tmp->size = pow(NUMBER_CHANNELS,(double)ceil( ((double)log((double)bufsize)) / log(NUMBER_CHANNELS)));
   /* (int)( (bufsize)  / 16 )*16); */
  if( (bufsize-1) % 16 ) {
    AIOUSB_WARN("Buffer (%d) isn't a factor of %d...converting to %d\n",bufsize, NUMBER_CHANNELS,tmp->size);
  }

#endif

  tmp->buffer            = (AIOBufferType *)malloc(tmp->size*sizeof(AIOBufferType));
  tmp->_read_pos         = 0;
  tmp->_write_pos        = 0;
  tmp->status            = NOT_STARTED;
  tmp->worker            = cont_thread;
  tmp->hz                = 100000; /**> Default value of 100khz  */
  tmp->timeout           = 1000;   /**> Timeout of 1000us  */
  /* Threading mutex Setup */
#ifdef HAS_PTHREAD
  tmp->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
#endif

  return tmp;
}

/** 
 * 
 * @param buf 
 */
void
DeleteAIOContinuousBuf( AIOContinuousBuf *buf )
{
  free( buf->buffer );
  free( buf );
}

void
AIOContinuousBuf_SetCallback(AIOContinuousBuf *buf , void *(*work)(void *object ) )
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

unsigned buffer_max( AIOContinuousBuf *buf )
{
  return buf->size-1;
}

unsigned buffer_size( AIOContinuousBuf *buf )
{
  return buf->size;
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
/* void *(*)(void *) */
/*        AIOContinuousBuf_GetCallback( AIOContinuousBuf *buf ) */
/* void *(*AIOContinuousBuf_GetCallback(AIOContinuousBuf *buf))(void *) */

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
/* , void *(*work) (void *)  ) */
AIORET_TYPE
AIOContinuousBufStart( AIOContinuousBuf *buf )
{
  /* AIORET_TYPE ret; */
  AIORET_TYPE retval;

#ifdef HAS_PTHREAD
  buf->status = RUNNING;
  /* retval = pthread_create( &(buf->worker), NULL , buf->callback, (void *)buf  ); */
  /* retval = pthread_create( &(buf->worker), NULL , work, (void *)buf  ); */

  /* retval = pthread_create( &(buf->worker), NULL, buf->callback, (void *)buf ); */
  retval = pthread_create( &worker, NULL, buf->callback, (void *)buf );

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

      /* min_err = ( hz - ((ROOTCLOCK ) / (divisora * l))).abs */
      min_err = abs(((ROOTCLOCK / hz) - (divisora * l)));
      
      /* l.downto(1) { |lv|  */
      for( unsigned lv = l ; lv >= 2 ; lv -- ) {
        unsigned olddivisora = (int)round((double)divisorab / lv);
        if( olddivisora > 0xffff ) { 
          AIOUSB_DEVEL( "Found value > 0xff..resetting" );
          break;
        } else { 
          divisora = olddivisora;
        }
        /* err = abs((hz - ((ROOTCLOCK ) / (divisora * lv)))) */
        err = abs((ROOTCLOCK / hz) - (divisora * lv));
        if( err <= 0  ) {
          min_err = 0;
          /* puts "Found zero error: lv=#{lv}" */
          AIOUSB_DEVEL("Found zero error: %d\n", lv );
          divisorb = lv;
          break;
        } 
        if( err < min_err  ) {
          AIOUSB_LOG( "Found new error: using lv=%d", (int)lv);
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



AIORET_TYPE Launch( AIOUSB_WorkFn callback, AIOContinuousBuf *buf )
{
  /** create thread to launch function */
  AIORET_TYPE retval;
  /* retval = pthread_create( &(buf->worker), NULL , callback, (void *)buf  ); */
  retval = pthread_create( &worker, NULL , callback, (void *)buf  );
  if( retval != 0 ) {
    retval = -abs(retval);
  }
  return retval;
}


void *ActualWorkFunction( void *object )
{
  /* sched_yield(); */
  /* AIOContinuousBuf *buf = (AIOContinuousBuf*)object; */
  /* AIOUSB_DEVEL("\tAddress is 0x%x\n", (int)(unsigned long)(AIOContinuousBuf *)buf ); */
  /* unsigned  size  = 1000; */
  /* AIOBufferType *tmp = (AIOBufferType*)malloc(size*sizeof(AIOBufferType)); */
  /* AIORET_TYPE retval; */

  /* while ( buf->status == RUNNING ) {  */

  /*   /\* fill_buffer( tmp, size ); *\/ */
  /*   AIOUSB_DEVEL("\tLooping spinning wheels\n");  */
  /*   retval = AIOContinuousBufWrite( buf, tmp, size , AIOCONTINUOUS_BUF_NORMAL ); */
  /*   AIOUSB_DEVEL("\tWriting buf , requested size=%d, got=%d\n", size, (int)retval ); */
  /* } */
  /* AIOUSB_DEVEL("Stopping\n"); */
  /* AIOUSB_DEVEL("Completed loop\n"); */
  /* free(tmp); */
  /* pthread_exit((void*)&retval); */
  return NULL;
}

AIORET_TYPE AIOContinuousBufSetupCounters( AIOContinuousBuf *buf ) 
{
  AIORET_TYPE retval;
  int BlockIndex = 0;

  retval = (AIORET_TYPE)CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 1, 2); /* Counter 1, Mode 2 */
  if( retval != AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufSetupCounters;

  retval = (AIORET_TYPE)CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 2, 3); /* Counter 2, Mode 3 */
  if( retval != AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufSetupCounters;

  CalculateClocks( buf );

  if( (retval = CTR_8254ModeLoad(AIOContinuousBuf_GetDeviceIndex( buf ) , BlockIndex, 1, 2, buf->divisora )) != 
      AIOUSB_SUCCESS )
    goto out_AIOContinuousBufSetupCounters;

  if( (retval = CTR_8254ModeLoad(AIOContinuousBuf_GetDeviceIndex( buf ) , BlockIndex, 2, 3, buf->divisorb )) != 
      AIOUSB_SUCCESS ) 
    goto out_AIOContinuousBufSetupCounters;

out_AIOContinuousBufSetupCounters:
  return retval;

}

AIORET_TYPE DoBCControl( AIOContinuousBuf *buf, long TCSize, unsigned long ControlData)
{
  unsigned long control_data_size;
  AIORET_TYPE result =  GenericVendorWrite( AIOContinuousBuf_GetDeviceIndex( buf ), 
                                            AUR_START_ACQUIRING_BLOCK, 
                                            (TCSize >> 16), 
                                            TCSize, 
                                            &control_data_size,
                                            &ControlData
                                            );
  return result;
}


AIORET_TYPE AIOContinuousBufCallbackStartClocked( AIOContinuousBuf *buf, AIOUSB_WorkFn callback  )
{
  AIORET_TYPE retval;

  /** 
   * Setup counters
   * see reference in <a href="http://accesio.com/MANUALS/USB-AIO%20Series.PDF">USB AIO documentation</a>
   **/
  if( buf->counter_control ) { 
    /* _GenericVendorWrite(DI, $C5, DivisorA, DivisorB, 0, nil); */
    AIOContinuousBufSetupCounters( buf );
  } else {
    DoBCControl(buf, 0, 0x01000007);
  }
  AIOContinuousBuf_SetCallback( buf , ActualWorkFunction );

  retval = AIOContinuousBufStart( buf ); /**< Fills up the buf buffer  */
  if ( retval != AIOUSB_SUCCESS )
    goto out_AIOContinuousBufCallbackStartClocked;
  /**
   * Now call their function in a thread to perform 
   * the call back , using the access to the the 
   * AIOContinuousBuf buf
   */
  
  /**
   * Allow the other command to be run
   */
  Launch( callback, buf );
  /* callback( (void*)buf ); */
out_AIOContinuousBufCallbackStartClocked:
  return retval;
}

/** 
 * 
 * @param buf 
 * @param tmpbuf 
 * @param bufsize 
 * 
 * @return Success if >= 0, else error value
 */
AIORET_TYPE
AIOContinuousBufBulkTransfer( AIOContinuousBuf *buf ,  AIOBufferType *tmpbuf , unsigned bufsize  )
{
  AIORET_TYPE retval = 0; 
  int transferred = 0;
  
  libusb_device_handle *deviceHandle = AIOUSB_GetDeviceHandle(  AIOContinuousBuf_GetDeviceIndex( buf ));
  if( ! deviceHandle ) {
    retval = -AIOUSB_ERROR_DEVICE_NOT_CONNECTED;
    goto out_AIOContinuousBufBulkTransfer;
  }
                                                                


  AIOUSB_DEBUG("Doing something here\n");
  retval = AIOUSB_BulkTransfer( deviceHandle,
                                LIBUSB_ENDPOINT_IN | USB_BULK_READ_ENDPOINT,
                                ( unsigned char* )tmpbuf,
                                bufsize*sizeof(AIOBufferType),
                                &transferred,
                                buf->timeout
                                );  
  AIOUSB_DEBUG("Completed Bulk acquire");

 out_AIOContinuousBufBulkTransfer:
  return retval;
}


/** 
 * 
 * @param buf 
 * @return 
 */
AIORET_TYPE CaptureData( AIOContinuousBuf *buf ) 
{
  AIORET_TYPE retval;
  int bufsize = buffer_size(buf);
  AIOBufferType *tmpbuf = (AIOBufferType *)malloc( bufsize * sizeof(AIOBufferType ));
  /* int size =3; */
  /* unsigned tcsize = 0x01000007; */
  unsigned long write_size = 0;

  /** 
   * Call this the setup (bc) 
   * This causes the problems
   * 0x0100 0007
   */
  /* DoBCControl( buf, 0, 0x01000007 ); */
  buf->counter_control = 1;
  if ( buf->counter_control ) { 
    retval = GenericVendorWrite( AIOContinuousBuf_GetDeviceIndex(buf), 
                                 0xC5, 
                                 buf->divisora, 
                                 buf->divisorb, 
                                 tmpbuf,
                                 &write_size
                                 );
    AIOUSB_DEVEL("Value returned was %d\n", (int)retval );
    if ( retval != AIOUSB_SUCCESS ) 
      goto out_CaptureData;
  } else {
    DoBCControl(buf,0, 0x01000007);
  }

  /**> Now start collecting the data  */

#if 1
   while ( buf->status == RUNNING ) {
    AIOUSB_WARN("Doing something here !\n");
    sleep(1);

    retval = AIOContinuousBufBulkTransfer( buf ,  tmpbuf , bufsize  );
    if( retval != AIOUSB_SUCCESS ) {
      AIOUSB_ERROR("Error reading bulk\n");
    }
  
    /* retval =  _GenericBulkIn(0 ,  */
    /*                          TargetPipe,  */
    /*                          @ThisBuf.ADBuf[0],  */
    /*                          Length(ThisBuf.ADBuf),  */
    /*                          ThisBuf.UsedSize */
    /*                          ); */

  }
#endif
 out_CaptureData:
  free(buf);
  
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
    basic_copy = MIN(size, buffer_max(buf) - get_read_pos(buf));
    wrap_copy  = MIN(size - basic_copy, get_write_pos(buf) );
  }
  /* Now copy the data into readbuf */

  memcpy( &readbuf[0]          , &buf->buffer[get_read_pos(buf)]  , basic_copy*sizeof(AIOBufferType) );
  memcpy( &readbuf[basic_copy] , &buf->buffer[0]                  , wrap_copy*sizeof(AIOBufferType)  );
  
  if( wrap_copy ) {
    retval = basic_copy + wrap_copy;
    set_read_pos( buf, ( get_read_pos(buf) + retval) % buffer_max(buf) );
  } else {
    retval = basic_copy;
    set_read_pos( buf , ( get_read_pos(buf) + retval) );
  }

  AIOContinuousBufUnlock( buf );
  return retval;
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
 * 
 * @return 
 */
unsigned read_size( AIOContinuousBuf *buf ) 
{
  return ( buffer_size(buf) - write_size(buf) );
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
  /* if( write_size(buf) >= size || flag == AIOCONTINUOUS_BUF_NORMAL ) {  */
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
  AIORET_TYPE retval;
#ifdef HAS_PTHREAD
  /* retval = pthread_mutex_lock( &buf->lock ); */
  retval = pthread_mutex_lock( &lock );
  if ( retval != 0 ) {
    retval = -retval;
  }
#endif
  return retval;
}
AIORET_TYPE AIOContinuousBufUnlock( AIOContinuousBuf *buf )
{
  int retval;
#ifdef HAS_PTHREAD
  /* retval = pthread_mutex_unlock( &buf->lock ); */
  retval = pthread_mutex_unlock( &lock );
  if ( retval !=  0 ) {
    retval = -retval;
    AIOUSB_ERROR("Unable to unlock mutex");
  }
#endif
  return retval;
}

AIORET_TYPE AIOContinuousBufReadChannels( AIOContinuousBuf *buf , AIOBufferType *readbuf, unsigned bufsize )
{
  int actbufsize = bufsize  / 16;
  AIORET_TYPE retval;

  if ( actbufsize < bufsize )
    AIOUSB_WARN("Truncating buffer size to be multiple of 16 channels\n");
  if ( actbufsize < 0 ) {
    retval = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
  }

  retval = AIOContinuousBufRead( buf, readbuf, actbufsize );

 out_AIOContinuousBufReadChannels:
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
  /* ret = pthread_join( buf->worker , &ptr ); */
  ret = pthread_join( worker, &ptr );
#endif
  if ( ret != 0 ) {
    AIOUSB_ERROR("Error joining threads");
  }
  buf->status = JOINED;
  return ret;
}



void AIOContinuousBuf_SetDeviceIndex( AIOContinuousBuf *buf , unsigned long DeviceIndex )
{
  AIOContinuousBufLock( buf );
  buf->DeviceIndex = DeviceIndex; 
  AIOContinuousBufUnlock( buf );

}

unsigned long AIOContinuousBuf_GetDeviceIndex( AIOContinuousBuf *buf )
{
  return buf->DeviceIndex;
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
  AIOContinuousBuf *buf = NewAIOContinuousBuf( size );
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
    usleep(100);
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
}


void basic_functionality()
{
  AIOContinuousBuf *buf = NewAIOContinuousBuf( 10000 );
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
  AIOContinuousBuf *buf = NewAIOContinuousBuf( bufsize );
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
    usleep(10);
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
  
  printf("%s - was able to read for keysize %d\n", (retval == AIOUSB_SUCCESS ? "ok" : "not ok" ), keysize);
  free(tmp);
  DeleteAIOContinuousBuf( buf );
}





int main(int argc, char *argv[] )
{
  
  AIORET_TYPE retval;

  /* AIOContinuousBuf_SetDeviceIndex( buf, 0 ); */
  /* AIOContinuousBuf_SetCallback( buf , doit ); */
  printf("1..93\n");
  basic_functionality();
  int bufsize = 10000;
  for( int i = bufsize; i > 1 ; i /= 2 ) {
    /* printf("Using i:%d\n",i); */
    stress_test_one( bufsize , bufsize - bufsize / i);
  }


  bufsize = 1000006;
  for( int i = bufsize; i > 1 ; i /= 2 ) {
    stress_test_one( bufsize , bufsize - bufsize / i);
  }

  bufsize = 2000000;
  for ( int i = 1 , j = 2; i < 20 ; j*=2 , i += 1) {
    stress_test_read_channels( bufsize, j );
  }

}


#endif



  /* unsigned readbufsize = 1000000; */
  /* AIOBufferType *readbuf = (AIOBufferType *)malloc(sizeof(AIOBufferType)*readbufsize); */
  /* DeleteAIOContinuousBuf( buf ); */
  /* buf = NewAIOContinuousBuf( 10000 ); */
  /* AIOContinuousBuf_SetCallback( buf , newdoit ); */
  /* retval = AIOContinuousBufStart( buf ); */
  /* for(int i = 0 ; i < 500; i ++ ) { */
  /*   retval = AIOContinuousBufRead( buf,  readbuf, readbufsize ); */
  /*   usleep(100); */
  /*   AIOUSB_DEVEL("Got value, %d\n",(int)retval ); */
  /* } */
  /* AIOContinuousBufEnd( buf ); */
  /* free(readbuf); */

  /* buf = NewAIOContinuousBuf( 10000 ); */
  /* AIOContinuousBuf_SetCallback( buf , newdoit ); */
  /* retval = AIOContinuousBufStart( buf ); */
  /* sleep(60); */
  /* AIOContinuousBufEnd( buf ); */

  /* DeleteAIOContinuousBuf( buf ); */


  /* /\** */
  /*  *  Simple capture of data */
  /*  *\/  */
  /* AIOUSB_Init(); */
  /* AIOUSB_ListDevices(); */
  /* /\** */
  /*  * Setup the counter control  */
  /*  *\/ */
  /* AIOContinuousBufSetupCounters( buf ); */
  /* /\* CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 1, 2); /\\* Counter 1, Mode 2 *\\/ *\/ */
  /* /\* CTR_8254Mode( AIOContinuousBuf_GetDeviceIndex( buf ), 0, 2, 3); /\\* Counter 2, Mode 3 *\\/ *\/ */

  /* AIOContinuousBufSetClock( buf, 1000 ); /\* Set the clock rate *\/ */
  
  /* CalculateClocks( buf ); */

  /* /\* retval = CTR_8254ModeLoad(AIOContinuousBuf_GetDeviceIndex( buf ) , BlockIndex, 1, 2, buf->divisora ); *\/ */
  /* /\* retval = CTR_8254ModeLoad(AIOContinuousBuf_GetDeviceIndex( buf ) , BlockIndex, 2, 3, buf->divisorb ); *\/ */

  /* CaptureData( buf ); */

















