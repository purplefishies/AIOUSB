/**
 * @file   DIOBuf.c
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @version $Format: %h$
 * @brief  Buffers for DIO elements
 *
 */

#include "DIOBuf.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif


int _determine_strbuf_size( unsigned size )
{
    return (((size / BITS_PER_BYTE)+1)*BITS_PER_BYTE) + strlen("0x") + 1;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief Constructor for 
 * @param size Preallocates the buffer to size 
 * @return DIOBuf * or Null if failure
 */
DIOBuf *NewDIOBuf( unsigned size ) {
    DIOBuf *tmp = (DIOBuf *)malloc( sizeof(DIOBuf) );
    if( ! tmp ) 
        return tmp;
    tmp->buffer = (DIOBufferType *)calloc(size, sizeof(DIOBufferType));
    if ( !tmp->buffer ) {
        free( tmp );
        return NULL;
    }
    tmp->strbuf_size = _determine_strbuf_size( size );
    tmp->strbuf = (char *)malloc(sizeof(char) * tmp->strbuf_size );
    if ( !tmp->strbuf ) {
        free(tmp->buffer);
        free(tmp);
        return NULL;
    }
    tmp->size = size;
    return tmp;
}
/*----------------------------------------------------------------------------*/
void _copy_to_buf( DIOBuf *tmp, const char *ary, int size_array ) {
    int tot_bit_size = tmp->size;
    int i; 
    for ( i = 0 ; i < tot_bit_size ; i ++ ) { 
        int curindex = i / 8;
        tmp->buffer[ i ] = (( ary[curindex] >> (( 8-1 ) - (i%8)) ) & 1 ? 1 : 0 );
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
 * @brief Constructor from a string argument like "101011011";
 */
DIOBuf *NewDIOBufFromBinStr( const char *ary ) {
    int tot_bit_size = strlen(ary);
    DIOBuf *tmp = NewDIOBuf( tot_bit_size );
    int i;
    if( ! tmp ) 
        return tmp;
    for ( i = tot_bit_size - 1; i >= 0 ; i -- ) { 
        DIOBufSetIndex( tmp, tot_bit_size - 1 - i  , (ary[i] == '0' ? 0 : 1 ) );
    }
    return tmp;
}
/*----------------------------------------------------------------------------*/
DIOBuf *DIOBufReplaceString( DIOBuf *buf, char *ary, int size_array ) 
{ 
    if ( buf  )
        if( DIOBufResize( buf, size_array*8 ) )
            _copy_to_buf( buf, ary, size_array );
    return buf;
}
/*----------------------------------------------------------------------------*/
DIOBuf *DIOBufReplaceBinString( DIOBuf *buf, char *bitstr ) 
{ 
    if ( buf  ) {
        if ( strlen(bitstr) / BITS_PER_BYTE > DIOBufSize( buf ) ) {
            return NULL;
        }
        for ( int i = strlen(bitstr) ; i >= 0 ; i -- ) {
            DIOBufSetIndex( buf, i, (bitstr[i] == '0' ? 0 : 1)  );
        }
    }
  return buf;
}
/*----------------------------------------------------------------------------*/
void DeleteDIOBuf( DIOBuf *buf ) 
{
    buf->size = 0;
    free( buf->buffer );
    free( buf->strbuf );
    free( buf );
}
/*----------------------------------------------------------------------------*/
DIOBuf *DIOBufResize( DIOBuf *buf , unsigned newsize ) 
{
    buf->buffer = (unsigned char *)realloc( buf->buffer, newsize*sizeof(unsigned char));
    if ( !buf->buffer ) {
        buf->size = 0;
        buf->strbuf_size = _determine_strbuf_size( newsize );
        buf->strbuf = (char *)realloc( buf->strbuf, buf->strbuf_size * sizeof(char) );
        buf->strbuf[0] = '\0';
        return NULL;
    }
    if( newsize > buf->size )
        memset( &buf->buffer[buf->size], 0, ( newsize - buf->size ));

    buf->strbuf_size = _determine_strbuf_size( newsize );
    buf->strbuf = (char *)realloc( buf->strbuf, buf->strbuf_size * sizeof(char));

    if ( !buf->strbuf )
        return NULL;
    buf->size = newsize;
    return buf;
}
/*----------------------------------------------------------------------------*/
unsigned  DIOBufSize( DIOBuf *buf ) {
  return buf->size;
}

/*----------------------------------------------------------------------------*/
unsigned DIOBufByteSize( DIOBuf *buf ) {
  return buf->size / BITS_PER_BYTE;
}

/*----------------------------------------------------------------------------*/
char *DIOBufToString( DIOBuf *buf ) {
  unsigned i;
  memset(buf->strbuf,0, buf->strbuf_size );
  for( i = 0; i < buf->size ; i ++ )
      buf->strbuf[i] = ( buf->buffer[i] == 0 ? '0' : '1' );
  buf->strbuf[buf->size] = '\0';
  return buf->strbuf;
}

/*----------------------------------------------------------------------------*/
char *DIOBufToHex( DIOBuf *buf ) {

    char *tmp = (char *)malloc( DIOBufSize(buf) / BITS_PER_BYTE );
    int size = DIOBufSize(buf) / BITS_PER_BYTE;

    memset( buf->strbuf, 0, buf->strbuf_size );
    memcpy( tmp, DIOBufToBinary( buf ), size );


    strcpy(&buf->strbuf[0], "0x" );
    int j = strlen(buf->strbuf);

    for ( int i = 0 ; i <  size ; i ++ , j = strlen(buf->strbuf)) {
        sprintf(&buf->strbuf[j], "%.2x", (unsigned char)tmp[i] );
    }
    buf->strbuf[j] = 0;
    free(tmp);
    return buf->strbuf;
}

/*----------------------------------------------------------------------------*/
char *DIOBufToBinary( DIOBuf *buf ) {
    int i, j;
    memset(buf->strbuf, 0, buf->strbuf_size );
    for ( i = 0, j = 0 ; i < (int)DIOBufSize(buf) ; i ++ , j = ( (j+1) % 8 )) { 
        buf->strbuf[i / BITS_PER_BYTE] |= buf->buffer[i] << ( 7 - (j % BITS_PER_BYTE) );
    }
    buf->strbuf[ ((i/ BITS_PER_BYTE)+1)*BITS_PER_BYTE ] = '\0';
    return buf->strbuf;
}

/*----------------------------------------------------------------------------*/
char *DIOBufToInvertedBinary( DIOBuf *buf ) {
    int i;
    char *orig = DIOBufToBinary(buf);
    int size = DIOBufSize(buf);
    int size_to_alloc = ((size / BITS_PER_BYTE)+1);
    char *tmp  = (char *)malloc( size_to_alloc );
    memcpy(tmp, orig, size_to_alloc ) ;
    for ( i = 0; i < size_to_alloc - 1; i ++ ) { 
        tmp[i] = ~tmp[i];
    }
    return tmp;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE DIOBufSetIndex( DIOBuf *buf, int index, unsigned value )
{
    AIO_ASSERT_RET( -AIOUSB_ERROR_INVALID_INDEX, index < (int)buf->size && index >= 0 );
    AIO_ASSERT_RET( -AIOUSB_ERROR_INVALID_PARAMETER, value == 0 || value == 1 );

    buf->buffer[buf->size - 1 - index] = ( value == AIOUSB_TRUE ? 1 : AIOUSB_FALSE );
    return 0;
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE DIOBufGetIndex( DIOBuf *buf, int index ) {
    AIO_ASSERT_RET( -AIOUSB_ERROR_INVALID_INDEX, index < (int)buf->size && index >= 0 );
  
    return buf->buffer[buf->size - 1 - index ];
}

/*----------------------------------------------------------------------------*/
AIORET_TYPE DIOBufGetByteAtIndex( DIOBuf *buf, unsigned index , char *value ) {
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if ( index >= buf->size / BITS_PER_BYTE )   
        return -AIOUSB_ERROR_INVALID_INDEX;
    *value = 0;
    int actindex = index * BITS_PER_BYTE;
    for ( int i = actindex ; i < actindex + BITS_PER_BYTE ; i ++ ) {
        *value |= ( DIOBufGetIndex( buf, i ) == 1 ? 1 << ( i % BITS_PER_BYTE ) : 0 );
    }

    return retval;
}
/*----------------------------------------------------------------------------*/
AIORET_TYPE DIOBufSetByteAtIndex( DIOBuf *buf, unsigned index, char  value ) {
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if ( index >= buf->size / BITS_PER_BYTE )   
        return -AIOUSB_ERROR_INVALID_INDEX;
    int actindex = index * BITS_PER_BYTE;
    for ( int i = actindex ; i < actindex + BITS_PER_BYTE ; i ++ ) {
        DIOBufSetIndex( buf, i,  (( (1 << i % BITS_PER_BYTE ) & value ) ? 1 : 0 ));
    }

    return retval;
}

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

TEST(DIOBuf, CharStrIncomplete ) {
    DIOBuf *buf = NewDIOBufFromBinStr("0100011001101111011011110100001001100001011100");
    EXPECT_STREQ( DIOBufToBinary(buf), "FooBap" );
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

TEST(DIOBuf, Binary_Output2 ) {
    DIOBuf *buf = NewDIOBufFromBinStr("0000000001100011");
    EXPECT_STREQ( DIOBufToBinary(buf), "\000c" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Inverted_Binary ) { 
    DIOBuf *buf = NewDIOBufFromBinStr("0000000001100011");
    char *tmp = DIOBufToInvertedBinary(buf);
    EXPECT_STREQ( tmp, "\xFF\x9C" );
    free(tmp);
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Resize_Test ) {
    DIOBuf *buf = NewDIOBuf(0);
    DIOBufResize(buf, 10 ); 
    EXPECT_STREQ( DIOBufToString(buf), "0000000000" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Hex_Output ) {
    DIOBuf *buf = NewDIOBufFromChar("Test",4 );
    EXPECT_STREQ( "0x54657374", DIOBufToHex(buf) );
    DIOBufReplaceString( buf, (char *)"This is a very long string to convert", 37);
    EXPECT_STREQ(  "0x5468697320697320612076657279206c6f6e6720737472696e6720746f20636f6e76657274", DIOBufToHex(buf) );
    DeleteDIOBuf( buf );
    
    buf =  NewDIOBufFromBinStr("00000000000000000000000011111111" );
    char *tmp = DIOBufToHex( buf );
    EXPECT_STREQ( tmp, "0x000000ff" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Indexing_is_correct ) {
    DIOBuf *buf = NewDIOBufFromBinStr("0010101101010101111011110111011011" );
    EXPECT_EQ( 1, DIOBufGetIndex(buf, 0 ) );
    EXPECT_EQ( 1, DIOBufGetIndex(buf, 1 ) );
    EXPECT_EQ( 0, DIOBufGetIndex(buf, 2 ) );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Correct_Null_Output ) {
    DIOBuf *buf = NewDIOBuf(16);
    EXPECT_STREQ( DIOBufToString(buf), "0000000000000000");
    EXPECT_STREQ( DIOBufToBinary(buf), "" );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Correct_Index_Reading ) {
    DIOBuf *buf = NewDIOBufFromBinStr("10101010001100111111000011111111" );
    char val;
    DIOBufGetByteAtIndex(buf, 0, &val );
    EXPECT_EQ( (unsigned char)val, 0xff );
    DIOBufGetByteAtIndex(buf, 1, &val );
    EXPECT_EQ( (unsigned char)val, 0xf0 );
    DIOBufGetByteAtIndex(buf, 2, &val );
    EXPECT_EQ( (unsigned char)val, 0x33 );
    DIOBufGetByteAtIndex(buf, 3, &val );
    EXPECT_EQ( (unsigned char)val, 0xaa );
    DeleteDIOBuf( buf );
}

TEST(DIOBuf, Correct_Index_Writing ) {
    DIOBuf *buf = NewDIOBufFromBinStr("10101010001100111111000011111111" );
    char val = 0xff;
    DIOBufSetByteAtIndex( buf, 1, 0xff );
    EXPECT_STREQ( DIOBufToString(buf), "10101010001100111111111111111111" );
    DIOBufSetByteAtIndex( buf, 2, 0xff );
    EXPECT_STREQ( DIOBufToString(buf), "10101010111111111111111111111111" );
    DIOBufSetByteAtIndex( buf, 2, 0x0f );
    EXPECT_STREQ( DIOBufToString(buf), "10101010000011111111111111111111" );
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
        tmp[DIOBufSize(buf) - 1 - k*k] = '1';
    }
    tmp[100] = '\0';
    EXPECT_STREQ( DIOBufToString(buf), tmp );
    DeleteDIOBuf(buf);
    free(tmp);
}


int main( int argc , char *argv[] ) 
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();

    return RUN_ALL_TESTS();  

}


#endif



