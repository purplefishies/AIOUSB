/**
 * @file   DIOBuf.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  Buffers for DIO elements
 *
 */

#include "DIOBuf.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
DIOBuf *NewDIOBuf( unsigned size ) {
  DIOBuf *tmp = (DIOBuf *)malloc(sizeof(DIOBuf));
  if( ! tmp ) 
    return tmp;
  tmp->_buffer = (DIOBufferType *)calloc(size, sizeof(DIOBufferType));
  if ( !tmp->_buffer ) {
    free( tmp );
    return NULL;
  }
  tmp->_strbuf = (char *)malloc(sizeof(char)*size+1);
  if ( !tmp->_strbuf ) {
    free(tmp->_buffer);
    free(tmp);
    return NULL;
  }
  tmp->_size = size;
  return tmp;
}
/*----------------------------------------------------------------------------*/
void _copy_to_buf( DIOBuf *tmp, const char *ary, int size_array ) {
    int tot_bit_size = tmp->_size;
    int i; 
    for ( i = tot_bit_size-1 ; i >= 0 ; i -- ) {
        int curindex = i / 8;
        tmp->_buffer[ i ] = (( ary[curindex] >> (( 8-1 ) - (i%8)) ) & 1 ? 1 : 0 );
    }
}
/*----------------------------------------------------------------------------*/
DIOBuf *NewDIOBufFromChar( const char *ary, int size_array ) {
    int tot_bit_size = size_array*8;
    DIOBuf *tmp = NewDIOBuf( tot_bit_size );
    if( ! tmp ) 
      return tmp;

    _copy_to_buf( tmp, ary , size_array );
    return tmp;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Constructor from a string argument like "101011011";
 */
DIOBuf *NewDIOBufFromBinStr( const char *ary ) {
    int tot_bit_size = strlen(ary);
    DIOBuf *tmp = NewDIOBuf( tot_bit_size );
    int i;
    if( ! tmp ) 
      return tmp;
    for ( i = tot_bit_size ; i >= 0 ; i -- ) {
        DIOBufSetIndex( tmp, i, (ary[i] == '0' ? 0 : 1)  );
    }
    return tmp;
}
/*----------------------------------------------------------------------------*/
DIOBuf *DIOBufReplaceString( DIOBuf *buf, char *ary, int size_array ) 
{ 
  if (!buf  ) { 
    /* buf = NewDIOBuf( size_array*8 ); */
  } else if ( !DIOBufResize( buf, size_array*8 )  ) 
    return NULL;
  _copy_to_buf( buf, ary, size_array );
  return buf;
}
/*----------------------------------------------------------------------------*/
void DeleteDIOBuf( DIOBuf *buf ) {
    buf->_size = 0;
    free( buf->_buffer );
    free( buf->_strbuf );
    free( buf );
}
/*----------------------------------------------------------------------------*/
DIOBuf *DIOBufResize( DIOBuf *buf , unsigned newsize ) {
  buf->_buffer = (unsigned char *)realloc( buf->_buffer, newsize*sizeof(unsigned char));
  if ( !buf->_buffer ) {
    buf->_size = 0;
    buf->_strbuf = (char *)realloc( buf->_strbuf, (newsize+1)*sizeof(char));
    buf->_strbuf[0] = '\0';
    return NULL;
  }
  if( newsize > buf->_size )
    memset( &buf->_buffer[buf->_size], 0, ( newsize - buf->_size ));

  buf->_strbuf = (char *)realloc( buf->_strbuf, (newsize+1)*sizeof(char));
  if ( !buf->_strbuf )
    return NULL;
  buf->_size = newsize;
  return buf;
}
/*----------------------------------------------------------------------------*/
unsigned DIOBufSize( DIOBuf *buf ) {
  return buf->_size;
}
/*----------------------------------------------------------------------------*/
char *DIOBufToString( DIOBuf *buf ) {
  unsigned i;
  memset(buf->_strbuf,0, DIOBufSize(buf)+1);
  for( i = 0; i < buf->_size ; i ++ )
    buf->_strbuf[i] = ( buf->_buffer[i] == 0 ? '0' : '1' );
  buf->_strbuf[buf->_size] = '\0';
  return buf->_strbuf;
}
/*----------------------------------------------------------------------------*/
char *DIOBufToHex( DIOBuf *buf ) {
    char *tmp = strdup( DIOBufToBinary( buf ));
    memset(buf->_strbuf, 0, DIOBufSize(buf)/8 );
    strcpy(&buf->_strbuf[0], "0x" );
    int j = strlen(buf->_strbuf);
    for ( int i = 0 ; i < (int)strlen(tmp) ; i ++ , j = strlen(buf->_strbuf)) {
        sprintf(&buf->_strbuf[j], "%x", tmp[i] );
    }
    buf->_strbuf[j] = 0;
    free(tmp);
    return buf->_strbuf;
}
/*----------------------------------------------------------------------------*/
char *DIOBufToBinary( DIOBuf *buf ) {
    int i, j;
    memset(buf->_strbuf, 0, DIOBufSize(buf)/8 );
    for( i = DIOBufSize(buf)-1, j = 0 ;  i >= 0 ; i -- , j ++  ) {
        buf->_strbuf[ i / 8] |= ( DIOBufGetIndex(buf, i ) & 1 ) << ( j % 8 );
    }
    buf->_strbuf[ DIOBufSize(buf) / 8] = '\0';
    return buf->_strbuf;
}
/*----------------------------------------------------------------------------*/
int DIOBufSetIndex( DIOBuf *buf, unsigned index, unsigned value )
{

  if ( index >= buf->_size ) {
    return -AIOUSB_ERROR_INVALID_INDEX;
  } 
  buf->_buffer[index] = value;
  return 0;
}

/*----------------------------------------------------------------------------*/
int DIOBufGetIndex( DIOBuf *buf, unsigned index ) {
  if ( index > buf->_size ) 
    return -1;
  
  return buf->_buffer[ index ];
}

/* #if defined(__cplusplus) && !defined(SELF_TEST) */
#ifdef __cplusplus 
}
#endif



/**
 * @brief Self test for verifying basic functionality of the DIO interface
 *
 */ 
#ifdef SELF_TEST

#include <math.h>

#include "gtest/gtest.h"
#include "tap.h"

using namespace AIOUSB;

TEST(DIOBuf , Toggle_Bits ) {

    DIOBuf *buf = NewDIOBuf(100);
    for ( int i = 2 ; i < 10 ; i ++ ) {
      float c = powf( 2, (float)i );
      int size = (int)c;
      DIOBufResize( buf, size );
      EXPECT_EQ( size, DIOBufSize(buf) );
      for( int j = 0 ; j < DIOBufSize(buf); j ++ ) {
        DIOBufSetIndex( buf, j, ( i % 2 == 0 ? 1 : 0 ));
      }
      char *tmp = (char *)malloc( (DIOBufSize(buf)+1)*sizeof(char));
      for( int k = 0; k < DIOBufSize( buf ); k ++ ) {
        tmp[k] = ( i % 2 == 0 ? '1': '0' );
      }
      tmp[DIOBufSize(buf)] = '\0';
      EXPECT_STREQ( DIOBufToString(buf), tmp );
      free(tmp);
    }
    DeleteDIOBuf(buf);
}

TEST(DIOBuf, CharStr_Constructor ) {
    DIOBuf *buf = NewDIOBufFromChar((char *)"Test",4 );
    EXPECT_STREQ( DIOBufToString(buf), "01010100011001010111001101110100" );
    DIOBufReplaceString( buf, (char *)"FooBar", 6);
    EXPECT_STREQ( DIOBufToString(buf), "010001100110111101101111010000100110000101110010" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, BinStr_Constructor ) {
    DIOBuf *buf = NewDIOBufFromBinStr("10110101101010101111011110111011111" );
    EXPECT_STREQ( DIOBufToString(buf), "10110101101010101111011110111011111" );    
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Binary_Output ) {
    DIOBuf *buf = NewDIOBufFromChar("Test",4 );
    EXPECT_STREQ( DIOBufToBinary(buf), "Test" );
    DeleteDIOBuf( buf );
}


TEST(DIOBuf, Resize_Test ) {
    DIOBuf *buf = NewDIOBuf(0);
    DIOBufResize(buf, 10 ); 
    EXPECT_STREQ( DIOBufToBinary(buf), "1111111111" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Hex_Output ) {
    DIOBuf *buf = NewDIOBufFromChar("Test",4 );
    EXPECT_STREQ( DIOBufToHex(buf), "0x54657374");
    DIOBufReplaceString( buf, (char *)"This is a very long string to convert", 37);
    EXPECT_STREQ( DIOBufToHex(buf), "0x5468697320697320612076657279206c6f6e6720737472696e6720746f20636f6e76657274");
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Toggle_interview ) {
    DIOBuf *buf = NewDIOBuf(100);
    char *tmp = (char*)malloc(DIOBufSize(buf)+1);
    for ( int i = 1 ; i < DIOBufSize(buf); i ++ ) {
        for ( int j = i ; j < DIOBufSize(buf); j += i ) {
            DIOBufSetIndex( buf, j , DIOBufGetIndex( buf , j ) == 0 ? 1 : 0 );
        }
    }
    for( int k = 0; k <DIOBufSize(buf); k ++ ) {
        tmp[k] = '0';
    }
    for( int k = 1; k*k < DIOBufSize(buf); k ++ ) {
        tmp[k*k] = '1';
    }
    tmp[100] = '\0';
    EXPECT_STREQ( DIOBufToString(buf), tmp );
    DeleteDIOBuf(buf);
}


int main( int argc , char *argv[] ) 
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
  delete listeners.Release(listeners.default_result_printer());
#endif
    listeners.Append( new tap::TapListener() );

    
    DIOBuf *buf = NewDIOBuf( 100 );
    DeleteDIOBuf( buf );
  
    return RUN_ALL_TESTS();  

}


#endif



