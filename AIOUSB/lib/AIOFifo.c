/**
 * @file   AIOFifo.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %t$
 * @brief  General structure for AIOUSB Fifo
 *
 */

#include "AIOTypes.h"
#include "AIOFifo.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif 

size_t delta( AIOFifo *fifo  ) 
{
    return ( fifo->write_pos < fifo->read_pos ? (fifo->read_pos - fifo->write_pos - 1 ) : ( (fifo->size - fifo->write_pos) + fifo->read_pos - 1 ));
}

size_t rdelta( AIOFifo *fifo  ) 
{
    return ( fifo->read_pos < fifo->write_pos ? (fifo->write_pos - fifo->read_pos ) : ( (fifo->size - fifo->read_pos) + fifo->write_pos ));
}

AIOFifo *NewAIOFifo( unsigned int size , unsigned refsize )
{ 
    AIOFifo *nfifo  = (AIOFifo *)calloc(1,sizeof(AIOFifo));
    nfifo->size     = size;
    nfifo->refsize  = refsize;
    nfifo->data     = malloc(size);
    nfifo->Read     = AIOFifoRead;
    nfifo->Write    = AIOFifoWrite;
    nfifo->delta    = delta;
#ifdef HAS_PTHREAD
    nfifo->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;   /* Threading mutex Setup */
#endif

    return nfifo;
}

void DeleteAIOFifo( AIOFifo *fifo ) 
{
    free(fifo->data);
    free(fifo);
}

size_t increment(AIOFifo *fifo, size_t idx)
{
    return (idx + 1) % fifo->size; 
}

AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize ) 
{
    pthread_mutex_lock( &fifo->lock );
    int actsize = MIN(MIN( maxsize, fifo->size), rdelta(fifo) );
    int basic_copy = MIN( actsize + fifo->read_pos, fifo->size ) - fifo->read_pos, wrap_copy = actsize - basic_copy;
    memcpy( tobuf                       , &((char *)fifo->data)[fifo->read_pos], basic_copy );
    memcpy( &((char*)tobuf)[basic_copy] , &((char *)fifo->data)[0]             , wrap_copy );
    fifo->read_pos = (fifo->read_pos + actsize ) % fifo->size ;
    pthread_mutex_unlock( &fifo->lock );
    return basic_copy + wrap_copy;
}

AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize ) {
    pthread_mutex_lock( &fifo->lock );
    int actsize = MIN(MIN( maxsize, fifo->size ), fifo->delta(fifo ));
    actsize = (actsize / fifo->refsize) * fifo->refsize;
    int basic_copy = MIN( actsize + fifo->write_pos, fifo->size ) - fifo->write_pos, wrap_copy = actsize - basic_copy;
    memcpy( &((char *)fifo->data)[fifo->write_pos], frombuf, basic_copy );
    memcpy( &((char *)fifo->data)[0], &((char *)&frombuf)[basic_copy], wrap_copy );
    fifo->write_pos = (fifo->write_pos + actsize ) % fifo->size ;
    pthread_mutex_unlock( &fifo->lock );
    return basic_copy + wrap_copy;
}

AIORET_TYPE AIOFifoWriteAllOrNone( AIOFifo *fifo, void *frombuf , unsigned maxsize ) {
    pthread_mutex_lock( &fifo->lock );
    int actsize = MIN(MIN( maxsize, fifo->size ), fifo->delta(fifo ));
    int basic_copy = MIN( actsize + fifo->write_pos, fifo->size ) - fifo->write_pos, wrap_copy = actsize - basic_copy;
    memcpy( &((char *)fifo->data)[fifo->write_pos], frombuf, basic_copy );
    memcpy( &((char *)fifo->data)[0], &((char *)&frombuf)[basic_copy], wrap_copy );
    fifo->write_pos = (fifo->write_pos + actsize ) % fifo->size ;
    pthread_mutex_unlock( &fifo->lock );
    return basic_copy + wrap_copy;
}

AIORET_TYPE AIOFifoReadAllOrNone( AIOFifo *fifo, void *tobuf , unsigned maxsize ) 
{
    pthread_mutex_lock( &fifo->lock );
    int actsize = MIN(MIN( maxsize, fifo->size), fifo->delta(fifo) );
    int basic_copy = MIN( actsize + fifo->read_pos, fifo->size ) - fifo->read_pos, wrap_copy = actsize - basic_copy;
    memcpy( tobuf                       , &((char *)fifo->data)[fifo->read_pos], basic_copy );
    memcpy( &((char*)tobuf)[basic_copy] , &((char *)fifo->data)[0]             , wrap_copy );
    fifo->read_pos = (fifo->read_pos + actsize ) % fifo->size ;
    pthread_mutex_unlock( &fifo->lock );
    return basic_copy + wrap_copy;
}




#ifdef __cplusplus
}
#endif 

#ifdef SELF_TEST

#include "AIOUSBDevice.h"
#include "gtest/gtest.h"
#include "tap.h"
#include <iostream>
using namespace AIOUSB;


TEST(Initialization,Callback )
{

    AIOFifo *bar = AIOUSB::NewAIOFifo(10000,sizeof(unsigned short));
    AIOFifo *fifo = bar;
    int retval;
    int maxsize = 4000;
    unsigned short *frombuf = (unsigned short *)malloc(20000);
    unsigned short *tobuf = (unsigned short *)malloc(20000);
    for( int i = 0; i < 10000; i ++ ) { frombuf[i] = i ; }
    
    fifo->Write( fifo, frombuf, 2000 );
    fifo->Write( fifo, frombuf, 2000 );
    fifo->Write( fifo, frombuf, 2000 );    
    fifo->Write( fifo, frombuf, 2000 );

    retval = fifo->Read( fifo, tobuf, 20000 );

    EXPECT_EQ( retval, 8000 );
    EXPECT_EQ( fifo->write_pos, 8000 );
    DeleteAIOFifo(fifo);
}

TEST(Initialization,NoOverWrite )
{
    int size = 1000;
    AIOFifo *bar = AIOUSB::NewAIOFifo((size+1)*sizeof(unsigned short),sizeof(unsigned short));
    AIOFifo *fifo = bar;
    int retval;
    int maxsize = 4000;
    unsigned short *frombuf = (unsigned short *)malloc(size*sizeof(unsigned short));
    unsigned short *tobuf = (unsigned short *)malloc(size*sizeof(unsigned short));
    for( int i = 0; i < size; i ++ ) { frombuf[i] = i ; }
    
    retval = fifo->Write( fifo, frombuf, size/2 * sizeof(unsigned short ) );
    EXPECT_EQ( size, retval );
    retval = fifo->Write( fifo, frombuf, size/2 * sizeof(unsigned short ) );
    EXPECT_EQ( size, retval );
    retval = fifo->Write( fifo, frombuf, size/2 * sizeof(unsigned short ) );

    EXPECT_EQ( 0, retval ) << "Should only be able to write an integer of the smallest size";

    retval = fifo->Read( fifo, tobuf, size );
    EXPECT_EQ( retval, size );

    EXPECT_EQ( fifo->write_pos, size*sizeof(unsigned short ) );
    DeleteAIOFifo(fifo);
}


struct args {
    unsigned short *buf;
    unsigned int size;
    unsigned int count_to;
    AIOFifo *fifo;
};

void *write_lots( void *arg ) {
    struct args *tmp = (struct args*)arg;
    int j = 0;
    while ( j < tmp->count_to ) { 
        for ( int i = 0; i < tmp->size / 4 ; i ++ , j ++ ) {
            tmp->buf[i] = j;
        }
        tmp->fifo->Write( tmp->fifo, tmp->buf, (tmp->size / 4)*sizeof(unsigned short)  );
    }
    return NULL;
}

void *read_lots( void *arg ) {
    struct args *tmp = (struct args*)arg;
    int j = 0;
    while ( j < tmp->count_to ) { 
        tmp->fifo->Read( tmp->fifo, &tmp->buf[0] , (tmp->size / 4)*sizeof(unsigned short)  );
    }

    return NULL;
}


TEST(Initialization, ThreadedReadAndWrite )
{
    AIOFifo *fifo = AIOUSB::NewAIOFifo(100000*sizeof(unsigned short),sizeof(unsigned short));
    unsigned short *frombuf = (unsigned short *)malloc(20000);
    unsigned short *tobuf   = (unsigned short *)malloc(20000);
    struct args from_buf_args = { .buf = frombuf, .size = 20000 , .count_to = 100000 , .fifo = fifo };
    struct args to_buf_args   = { .buf = tobuf  , .size = 20000 , .count_to = 100000 , .fifo = fifo };
    pthread_t writer, reader;
    /* pthread_create( &writer, NULL, write_lots, &from_buf_args ); */
    /* pthread_create( &reader, NULL, read_lots, &from_buf_args ); */
    /* printf("here\n"); */

}


int main(int argc, char *argv[] )
{

  AIORET_TYPE retval;

  testing::InitGoogleTest(&argc, argv);
  testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
  delete listeners.Release(listeners.default_result_printer());
#endif

  listeners.Append( new tap::TapListener() );
  return RUN_ALL_TESTS();  
}

#endif
