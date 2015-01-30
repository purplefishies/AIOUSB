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
#include <stdarg.h>


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

size_t _calculate_size_write( AIOFifo *fifo, unsigned maxsize)
{
    int actsize = MIN(MIN( maxsize, fifo->size ), fifo->delta(fifo ));
    actsize = (actsize / fifo->refsize) * fifo->refsize;
    return actsize;
}

size_t _calculate_size_read( AIOFifo *fifo, unsigned maxsize)
{
    return MIN(MIN( maxsize, fifo->size), rdelta(fifo) );
}

size_t _calculate_size_aon_write( AIOFifo *fifo, unsigned maxsize)
{
    return ( fifo->delta(fifo) < maxsize ? 0 : MIN(fifo->delta(fifo), maxsize ));
}

size_t _calculate_size_aon_read( AIOFifo *fifo, unsigned maxsize )
{
    return ( rdelta(fifo) < maxsize ? 0 : maxsize );
}

void AIOFifoInitialize( AIOFifo *nfifo, unsigned int size, unsigned refsize )
{
    nfifo->size     = size;
    nfifo->refsize  = refsize;
    nfifo->data     = malloc(size);
    nfifo->Read     = AIOFifoRead;
    nfifo->Write    = AIOFifoWrite;
    nfifo->delta    = delta;
    nfifo->_calculate_size_write = _calculate_size_write;
    nfifo->_calculate_size_read  = _calculate_size_read;
#ifdef HAS_THREAD
    nfifo->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;   /* Threading mutex Setup */
#endif
}

AIOFifo *NewAIOFifo( unsigned int size , unsigned refsize )
{ 
    AIOFifo *nfifo  = (AIOFifo *)calloc(1,sizeof(AIOFifo));
    AIOFifoInitialize( nfifo, size, refsize );
    return nfifo;
}

void AIOFifoAllOrNoneInitialize( AIOFifo *nfifo , unsigned int size, unsigned refsize ) 
{
    nfifo->Read     = AIOFifoReadAllOrNone;
    nfifo->Write    = AIOFifoWriteAllOrNone;
    nfifo->_calculate_size_write = _calculate_size_aon_write;
    nfifo->_calculate_size_read  = _calculate_size_aon_read;
}

AIOFifo *NewAIOFifoAllOrNone( unsigned int size , unsigned refsize )
{
    AIOFifo *nfifo  = NewAIOFifo( size, refsize );
    AIOFifoAllOrNoneInitialize( nfifo, size, refsize );
    return nfifo;
}


#define LOOKUP(T) aioeither_value_ ## T


AIORET_TYPE Push( AIOFifoTYPE *fifo, TYPE a )
{
    TYPE tmp = a;
    int val = fifo->Write( (AIOFifo*)fifo, &tmp, sizeof(TYPE) );
    return val;
}

AIORET_TYPE PushN( AIOFifoTYPE *fifo, TYPE *a, unsigned N ) {
    return fifo->Write( (AIOFifo*)fifo, a, N*sizeof(TYPE));
}

AIOEither Pop( AIOFifoTYPE *fifo )
{
    TYPE tmp;
    AIOEither retval = {0};
    int tmpval = fifo->Read( (AIOFifo*)fifo, &tmp, sizeof(TYPE) );

    if( tmpval <= 0 ) {
        retval.left = tmpval;
    } else {
        AIOEitherSetRight( &retval, LOOKUP( uint32_t ), &tmp );
    }

    return retval;
}

AIORET_TYPE PopN( AIOFifoTYPE *fifo, TYPE *in, unsigned N)
{
    AIORET_TYPE retval = {0};
    int tmpval = fifo->Read( (AIOFifo*)fifo, in, sizeof(TYPE)*N );

    if( tmpval <= 0 ) {
        retval = -AIOUSB_FIFO_COPY_ERROR;
    } 
    
    return retval;
}


AIOFifoTYPE *NewAIOFifoTYPE( unsigned int size )
{
    AIOFifoTYPE *nfifo = (AIOFifoTYPE*)calloc(1,sizeof(AIOFifoTYPE));
    AIOFifoInitialize( (AIOFifo*)nfifo , (size+1)*sizeof(TYPE), sizeof(TYPE));
    nfifo->Push = Push;
    nfifo->PushN = PushN;
    nfifo->Pop = Pop;
    nfifo->PopN = PopN;
    return nfifo;
}
void DeleteAIOFifoTYPE( AIOFifoTYPE *fifo )
{
    DeleteAIOFifo( (AIOFifo*)fifo);
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
    GRAB_RESOURCE( fifo );
    int actsize = fifo->_calculate_size_read( fifo, maxsize );
    if ( actsize ) {
        int basic_copy = MIN( actsize + fifo->read_pos, fifo->size ) - fifo->read_pos, wrap_copy = actsize - basic_copy;
        memcpy( tobuf                       , &((char *)fifo->data)[fifo->read_pos], basic_copy );
        memcpy( &((char*)tobuf)[basic_copy] , &((char *)fifo->data)[0]             , wrap_copy );
        fifo->read_pos = (fifo->read_pos + actsize ) % fifo->size ;
    }
    RELEASE_RESOURCE( fifo );
    return actsize;
}

AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize ) {
    GRAB_RESOURCE( fifo );
    int actsize = fifo->_calculate_size_write( fifo, maxsize );
    if ( actsize ) {
        int basic_copy = MIN( actsize + fifo->write_pos, fifo->size ) - fifo->write_pos, wrap_copy = actsize - basic_copy;
        memcpy( &((char *)fifo->data)[fifo->write_pos], frombuf, basic_copy );
        memcpy( &((char *)fifo->data)[0], &((char *)&frombuf)[basic_copy], wrap_copy );
        fifo->write_pos = (fifo->write_pos + actsize ) % fifo->size ;
    }
    RELEASE_RESOURCE( fifo );
    return actsize;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief for AllOrNoneTesting
 */
AIORET_TYPE AIOFifoWriteAllOrNone( AIOFifo *fifo, void *frombuf , unsigned maxsize ) {
    GRAB_RESOURCE( fifo );
    int actsize = fifo->_calculate_size_write( fifo, maxsize );
    if ( actsize ) {
        int basic_copy = MIN( actsize + fifo->write_pos, fifo->size ) - fifo->write_pos, wrap_copy = actsize - basic_copy;
        memcpy( &((char *)fifo->data)[fifo->write_pos], frombuf, basic_copy );
        memcpy( &((char *)fifo->data)[0], &((char *)&frombuf)[basic_copy], wrap_copy );
        fifo->write_pos = (fifo->write_pos + actsize ) % fifo->size ;
    }
    RELEASE_RESOURCE( fifo );
    return actsize;
}

AIORET_TYPE AIOFifoReadAllOrNone( AIOFifo *fifo, void *tobuf , unsigned maxsize ) 
{
    GRAB_RESOURCE( fifo );
    int actsize = fifo->_calculate_size_read( fifo, maxsize );
    if ( actsize ) { 
        int basic_copy = MIN( actsize + fifo->read_pos, fifo->size ) - fifo->read_pos, wrap_copy = actsize - basic_copy;
        memcpy( tobuf                       , &((char *)fifo->data)[fifo->read_pos], basic_copy );
        memcpy( &((char*)tobuf)[basic_copy] , &((char *)fifo->data)[0]             , wrap_copy );
        fifo->read_pos = (fifo->read_pos + actsize ) % fifo->size ;
    }
    RELEASE_RESOURCE( fifo );
    return actsize;
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


TEST(Initialization,AllOrNoneTesting )
{
    int size = 1000;
    AIOFifo *bar = AIOUSB::NewAIOFifoAllOrNone((size+1)*sizeof(unsigned short),sizeof(unsigned short));
    AIOFifo *fifo = bar;
    int retval;
    int maxsize = 4000;
    unsigned short *frombuf = (unsigned short *)malloc(size*sizeof(unsigned short));
    unsigned short *tobuf = (unsigned short *)malloc(size*sizeof(unsigned short));
    for( int i = 0; i < size; i ++ ) { frombuf[i] = i ; }
    
    retval = fifo->Write( fifo, frombuf, (2*size* sizeof(unsigned short ))/3 );
    EXPECT_EQ( retval, (2*size * sizeof(unsigned short ))/3);
    retval = fifo->Write( fifo, frombuf, (2*size * sizeof(unsigned short ))/3 );
    EXPECT_EQ( 0, retval ) << "Should only be able to write an integer of the smallest size";

    retval = fifo->Read( fifo, tobuf, size );
    EXPECT_EQ( retval, size );

    retval = fifo->Read( fifo, tobuf, size );
    EXPECT_EQ( 0, retval ) << "Should limit the size of the Read ";

    DeleteAIOFifo(fifo);
}

TEST(NewType,PushAndPop )
{
    int size = 1000;
    int retval = 0;
    AIOEither keepvalue;
    AIOFifoTYPE *fifo = AIOUSB::NewAIOFifoTYPE( size );
    TYPE *tmp = (TYPE*)malloc(sizeof(TYPE)*size);
    for( int i = 0 ; i < size ; i ++ ) tmp[i] = (TYPE)i;

    for( int i = 0 ; i < size ; i ++ ) {
        retval = fifo->Push( fifo, tmp[i] );
        EXPECT_EQ( sizeof(TYPE), retval ) << "Expected 4==" << retval << " on index " << i ;
    }

    for (int i = 0 ; i < size ; i ++ ) { 
        TYPE tval;
        keepvalue = fifo->Pop( fifo );
        EXPECT_EQ( keepvalue.left, 0 );
        AIOEitherGetRight( &keepvalue,&tval);
        EXPECT_EQ( tval , i );
    }
    AIOUSB::DeleteAIOFifoTYPE( fifo );
}

TEST(NewType,PushAndPopArrays )
{
    int size = 1000;
    int retval = 0;
    AIOEither keepvalue;
    AIOFifoTYPE *fifo = AIOUSB::NewAIOFifoTYPE( size );
    TYPE *tmp = (TYPE*)malloc(sizeof(TYPE)*size);
    for( int i = 0 ; i < size ; i ++ ) tmp[i] = (TYPE)i;
    
    fifo->PushN( fifo, tmp, size ); /* Write the whole array at once */

    for (int i = 0 ; i < size ; i ++ ) { 
        TYPE tval;
        keepvalue = fifo->Pop( fifo );
        EXPECT_EQ( keepvalue.left, 0 );
        AIOEitherGetRight( &keepvalue,&tval);
        EXPECT_EQ( tval , i );
    }
    keepvalue = fifo->Pop( fifo );
    EXPECT_EQ( keepvalue.left, 0 );

    fifo->PushN( fifo, tmp, size );
    ASSERT_EQ( tmp[3], 3 );
    memset(tmp,0,sizeof(TYPE)*size );
    ASSERT_EQ( tmp[3], 0 );

    fifo->PopN( fifo, tmp, size );

    ASSERT_EQ( tmp[3], 3 );

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
