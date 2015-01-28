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

#ifdef __cplusplus
namespace AIOUSB {
#endif 

AIOFifo *NewAIOFifo( unsigned int size )
{ 
    AIOFifo *nfifo = (AIOFifo *)calloc(1,sizeof(AIOFifo));
    nfifo->size = size;
    nfifo->data = malloc(size);
    return nfifo;
}

void DeleteAIOFifo( AIOFifo *fifo ) 
{
    free(fifo->data);
    free(fifo);
}

AIORET_TYPE AIOFifoRead( AIOFifo *fifo, void *tobuf , unsigned maxsize ) 
{
    int actsize = MIN( maxsize, fifo->size );
    int basic_copy = MIN( actsize + fifo->read_pos, fifo->size ) - fifo->read_pos, wrap_copy = actsize - basic_copy;
    memcpy( tobuf                       , &((char *)fifo->data)[fifo->read_pos], basic_copy );
    memcpy( &((char*)tobuf)[basic_copy] , &((char *)fifo->data)[0]             , wrap_copy );
    fifo->read_pos = (fifo->read_pos + actsize ) % fifo->size ;
    
    return basic_copy + wrap_copy;
}

AIORET_TYPE AIOFifoWrite( AIOFifo *fifo, void *frombuf , unsigned maxsize ) {
    int actsize = MIN( maxsize, fifo->size );
    int basic_copy = MIN( actsize + fifo->write_pos, fifo->size ) - fifo->write_pos, wrap_copy = actsize - basic_copy;

    memcpy( &((char *)fifo->data)[fifo->write_pos], frombuf, basic_copy );
    memcpy( &((char *)fifo->data)[0], &((char *)&frombuf)[basic_copy], wrap_copy );
    fifo->write_pos = (fifo->write_pos + actsize ) % fifo->size ;
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

    AIOFifo *bar = AIOUSB::NewAIOFifo(10000);
    AIOFifo *fifo = bar;
    int retval;
    int maxsize = 4000;
    unsigned short *frombuf = (unsigned short *)malloc(20000);
    unsigned short *tobuf = (unsigned short *)malloc(20000);
    for( int i = 0; i < 10000; i ++ ) { frombuf[i] = i ; }
    
    AIOFifoWrite( fifo, frombuf, 2000 );
    AIOFifoWrite( fifo, frombuf, 2000 );
    AIOFifoWrite( fifo, frombuf, 2000 );
    AIOFifoWrite( fifo, frombuf, 2000 );
    retval = AIOFifoRead( fifo, tobuf, 20000 ); 
    EXPECT_EQ( retval, 10000 );
    EXPECT_EQ( fifo->write_pos, 8000 );
    DeleteAIOFifo(fifo);
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
