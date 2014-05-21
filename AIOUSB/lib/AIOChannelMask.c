#include "AIOChannelMask.h"
#include "AIOTypes.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

/*----------------------------------------------------------------------------*/
    PUBLIC_EXTERN AIOChannelMask * NewAIOChannelMask( unsigned int number_channels ) {
        AIOChannelMask *tmp = (AIOChannelMask *)malloc(sizeof(AIOChannelMask ));
        unsigned i;
        if( !tmp ) {
            goto out_NewAIOChannelMask;
        }
        tmp->size = ((number_channels+BITS_PER_BYTE-1)/BITS_PER_BYTE); /* Ceil function */

        tmp->signals = (aio_channel_obj *)calloc(sizeof(aio_channel_obj), ( tmp->size + 1) );
        if( !tmp->signals ) 
            goto out_cleansignals;
        tmp->strrep          = 0;
        tmp->strrepsmall     = 0;
        tmp->number_signals  = number_channels;
        tmp->active_signals  = 0;

        tmp->signal_indices = (int*)malloc(sizeof(int)*(number_channels+1) );
        tmp->signal_index = 0;
        for ( i = 0; i < number_channels + 1 ; i ++ ) {
            tmp->signal_indices[i] = -1;
        }
    out_NewAIOChannelMask:
        return tmp;
    out_cleansignals:
        free(tmp);
        tmp = NULL;
        return tmp;
    }
/*----------------------------------------------------------------------------*/
/**
 * @desc Deletes the AIOChannelMask object
 * @param mask 
 */
PUBLIC_EXTERN void DeleteAIOChannelMask( AIOChannelMask *mask ) {
    if ( mask->strrep ) 
        free(mask->strrep);
    if( mask->strrepsmall )
        free( mask->strrepsmall );
    free(mask->signal_indices );
    free(mask->signals);
    free(mask);
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskIndices( AIOChannelMask *mask , int *pos ) {
    AIORET_TYPE retval;
    if ( !mask || !pos )
        return -AIOUSB_ERROR_INVALID_DATA;
    *pos = 0;
    retval = mask->signal_indices[*pos];
    *pos +=1;
    return retval;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskNextIndex( AIOChannelMask *mask , int *pos ) {
    int retval;
    if ( *pos >= (int)mask->number_signals ) {
        retval = *pos = -1;
    } else {
        retval = mask->signal_indices[*pos];
        *pos +=1 ;
    }
    return retval;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Sets the AIOChannelMask using the regular notion of or'ing of shifted bytes, 
 * 
 */
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskSetMaskFromInt( AIOChannelMask *obj, unsigned field ) {
    int i;
    AIORET_TYPE ret = AIOUSB_SUCCESS;  
    if ( obj->size <  (int)( sizeof(field) / sizeof(aio_channel_obj )) ) {
        return -AIOUSB_ERROR_INVALID_PARAMETER;
    }
    obj->signal_index = 0;
    for ( i = 0; i < obj->size ; i ++ ) {
      char curfield = *(((char *)&field)+i);
      obj->signals[(obj->size-1-i)] = curfield;
      for( int j = 0; j < BITS_PER_BYTE; j ++ ) {
         if( ( 1 << j ) & curfield ? 1 : 0 ) { 
           obj->active_signals ++;
           obj->signal_indices[obj->signal_index++] = (i*BITS_PER_BYTE) + j;
         }
      }
    }
    return ret;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskSetMaskAtIndex( AIOChannelMask *obj, char field, unsigned index  )
{
    if ( index >= (unsigned)obj->size )
        return -AIOUSB_ERROR_INVALID_INDEX;
    

    obj->signals[obj->size-index-1] = field;
    return AIOUSB_SUCCESS;
}

/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskGetSize( AIOChannelMask *obj ) {
     return (AIORET_TYPE)obj->size;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Returns channels that are set to High ( not low )
 * @param channels
 **/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskNumberChannels( AIOChannelMask *obj ) {
    return (AIORET_TYPE)obj->active_signals;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskNumberSignals( AIOChannelMask *obj ) {
    return (AIORET_TYPE)obj->number_signals;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Rely on the base type to determine the sizes
 * @param obj 
 * @param bitfields a character string that contains 0s and 1s. 
 *
 */
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskSetMaskFromStr( AIOChannelMask *obj, 
                                                         const char *bitfields 
                                                         ) {
    if ( strlen(bitfields) != obj->number_signals )
        return -AIOUSB_ERROR_INVALID_PARAMETER;

    AIORET_TYPE ret = AIOUSB_SUCCESS;
    unsigned j;
    aio_channel_obj tmpval = 0;
    obj->signal_index = 0;
    obj->signals = (char*)realloc( obj->signals, strlen(bitfields)*sizeof(char));
    obj->signal_indices = (int*)realloc( obj->signal_indices, sizeof(int)*(strlen(bitfields)) );
    obj->size = (strlen(bitfields) + BITS_PER_BYTE-1 ) / BITS_PER_BYTE ;
    memset(obj->signals,0,obj->size);
    int index = obj->size-1;
    for( int i = strlen(bitfields) , shiftval = 0; i > 0 ; i -- , shiftval = (shiftval + 1)%BITS_PER_BYTE ) {
        j = strlen(bitfields)-i;
        if( bitfields[i-1] == '1' ) {
            tmpval |= 1 << shiftval;
            obj->active_signals ++;
            obj->signal_indices[obj->signal_index++] = j;
        }
        if ( j > 0 && (( (j+1) % BITS_PER_BYTE) == 0)  ) {
            obj->signals[index--] = tmpval;
            tmpval = 0;
        }
    }
    obj->signals[index] = tmpval;

    return ret;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Creates a new AIOChannelMask object from a character string of 1's and 0's
 * @param bitfields
 * @return a new AIOChannelMask object 
 * @todo Add smarter error checking
 */
PUBLIC_EXTERN AIOChannelMask *NewAIOChannelMaskFromStr( const char *bitfields ) {
    AIOChannelMask *tmp = NewAIOChannelMask( strlen(bitfields) );
    AIOChannelMaskSetMaskFromStr( tmp, bitfields );
    return tmp;
}
/*----------------------------------------------------------------------------*/
/**
 * @desc Returns a mask for the index in question
 * @param obj 
 * @param index
 */
 PUBLIC_EXTERN const char *AIOChannelMaskToString( AIOChannelMask *obj ) {
     if( obj->strrep ) {
         free(obj->strrep);
     }
     obj->strrep = (char *)malloc( obj->number_signals+1*sizeof(char) );
     memset(obj->strrep,0,obj->number_signals+1);
     char *retval = obj->strrep;
     int i, j, pos, startpos;
     pos = 0;
     for ( i = 0;  i < obj->size; i ++ ) {
         /** @note Check for the case where we have say 17 signals( non-integer multiple of 
          * BITS_PER_BYTE 
          */
         if ( i == 0 && (obj->number_signals % BITS_PER_BYTE != 0) ) {
             startpos = (( obj->number_signals % BITS_PER_BYTE ) - 1);
         } else {
             startpos = BITS_PER_BYTE-1;
         }
         for ( j = startpos ; j >= 0 && pos < (int)obj->number_signals ; j -- ) { 
             retval[pos] = ((( 1 << j ) & ( obj->signals[i] )) ? '1' : '0');
             pos ++;
         }
     }

     return retval;
 }
/*----------------------------------------------------------------------------*/
/**
 * @desc Returns a mask for the index in question
 * @param obj 
 * @param index
 */
PUBLIC_EXTERN const char *AIOChannelMaskToStringAtIndex( AIOChannelMask *obj, unsigned index ) {
    if ( index >= (unsigned)obj->size ) {
        return NULL;
    }
    obj->strrepsmall = (char *)realloc( obj->strrepsmall, obj->number_signals+1 );
    memset(obj->strrepsmall,0,obj->number_signals+1);
    char *retval = obj->strrepsmall;
    int i, j, pos, startpos;
    pos = 0;
    /* for ( i = 0;  i < obj->size; i ++ ) { */
    /* for ( i = index ; i < obj->size ; i++ ) {  */
    /* i = index; */
    i = ( obj->size - index - 1 );
        /** @note Check for the case where we have say 17 signals( non-integer multiple of 
         * BITS_PER_BYTE 
         */
    if ( i == 0 && (obj->number_signals % BITS_PER_BYTE != 0) ) {
        startpos = (( obj->number_signals % BITS_PER_BYTE ) - 1);
    } else {
        startpos = BITS_PER_BYTE-1;
    }
    for ( j = startpos ; j >= 0 ; j -- ) { 
        retval[pos] = ((( 1 << j ) & ( obj->signals[i] )) ? '1' : '0');
        pos ++;
    }
    return retval;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN AIORET_TYPE AIOChannelMaskGetMaskAtIndex( AIOChannelMask *obj , char *tmp , unsigned index ) {
    AIORET_TYPE retval = AIOUSB_SUCCESS;
    if( index >= (unsigned)obj->size )
        return -AIOUSB_ERROR_INVALID_INDEX;
    *tmp = obj->signals[obj->size-1-index];
    return retval;
}
/*----------------------------------------------------------------------------*/
PUBLIC_EXTERN char *AIOChannelMaskGetMask( AIOChannelMask *obj ) {
    char *tmp = (char *)malloc(obj->size+1);
    if ( tmp ) {
        memset(tmp,0,obj->size+1);
        /* memcpy(tmp,obj->signals,obj->size); */
        for ( int i = 0; i < obj->size ; i ++ ) 
            tmp[i] = obj->signals[i];
    }
    return tmp;
}


#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#ifdef __cplusplus
using namespace AIOUSB;
#endif

#define TEST_MASK_STRING_AT_INDEX( expr, index, expected )                \
    { \
    int tval = strcmp(AIOChannelMaskToStringAtIndex( expr, index ),  expected ); \
    printf("%s - %s%s%s%s\n", ( tval == 0  ? "ok" : "not ok" ), "decode was ", AIOChannelMaskToStringAtIndex( expr, index ), " expected was " , expected ); \
    } 

#define TEST_VALID_SIGNAL_LENGTH(mask, expected ) \
    printf("%s - %s, exp=%d got=%d\n", (AIOChannelMaskNumberChannels( mask ) == expected ? "ok" : "not ok" ) , "Got correct number of channels", expected, (int)AIOChannelMaskNumberChannels( mask ) ); 

#define TEST_MASK_STRING( mask, expected ) \
    { \
      int tval = strncmp(AIOChannelMaskToString( mask ),  expected ,strlen(expected)); \
      printf("%s - %s%s%s%s\n", ( tval == 0  ? "ok" : "not ok" ), "decode was ", AIOChannelMaskToString( mask ), " expected was " , expected ); \
    }

#define TEST_VALUE_EQUAL( expr, value ) \
    printf("%s - %s%d%s%d\n", ( expr == value ? "ok" : "not ok" ), "value ", expr, " and expected ", value );


int main(int argc, char *argv[] )
{
  AIORET_TYPE retval;
  int i,j,pos;
  char signals[32] = {'\0'};
  char tmpmask;
  int expected[] = {0,1,3,7,30};
  int expected_long[] = {0,1,3,7,30,32,33,35,39,62};
  int received[4] = {0};
  AIOChannelMask *mask = NewAIOChannelMask( 32 );
  AIOChannelMaskSetMaskFromInt( mask , 1 | 2 | 1 << 3 | 1 << 7 | 1 << 30 );
  printf("1..32\n");

  /* AIOChannelMaskToString( mask ); */
  /* AIOChannelMaskToString( mask ); */
  TEST_MASK_STRING( mask, "01000000000000000000000010001011");
  TEST_MASK_STRING_AT_INDEX( mask, 0,  "10001011");
  TEST_VALID_SIGNAL_LENGTH( mask, 5 );

  pos = 0;

  j = 0;
  for ( i = AIOChannelMaskIndices(mask, &j ) ; i >= 0 ; i = AIOChannelMaskNextIndex(mask, &j ) ) {
      received[pos] = i;
      printf("%s - received: %d matches expected: %d\n", (received[pos] == expected[pos] ? "ok" : "not ok" ), 
             received[pos], expected[pos] );
      pos ++;
  }
  DeleteAIOChannelMask( mask );

  mask = NewAIOChannelMaskFromStr( "0100000000000000000000001000101101000000000000000000000010001011" );
  /* twice as long, verify that index works and that couning still works */
  TEST_MASK_STRING( mask, "0100000000000000000000001000101101000000000000000000000010001011" );
  TEST_MASK_STRING_AT_INDEX( mask, 7, "01000000" );
  TEST_VALID_SIGNAL_LENGTH( mask, 10 );
  TEST_MASK_STRING_AT_INDEX( mask, 1, "00000000" );
  TEST_MASK_STRING_AT_INDEX( mask, 3, "01000000" );
  AIOChannelMaskSetMaskAtIndex( mask, 0xff, 2 );
  TEST_MASK_STRING_AT_INDEX( mask, 2, "11111111" );
  AIOChannelMaskGetMaskAtIndex( mask, &tmpmask, 2 );
  TEST_VALUE_EQUAL( (unsigned char)tmpmask , 0xff);
  AIOChannelMaskSetMaskAtIndex( mask, 0xf0, 2 );
  TEST_MASK_STRING_AT_INDEX( mask, 2, "11110000" );

  pos = 0;
  j = 0;
  for ( i = AIOChannelMaskIndices( mask, &j ); i >= 0 ; i = AIOChannelMaskNextIndex( mask, &j )) { 
    received[pos] = i;
    printf("%s - received: %d matches expected: %d\n", (received[pos] == expected_long[pos] ? "ok" : "not ok" ), 
           received[pos], expected_long[pos] );
    pos ++;
  }
  DeleteAIOChannelMask( mask );


  strcpy(signals,"00000000000001000011000101011111" );
  mask = NewAIOChannelMask( strlen(signals) );
  AIOChannelMaskSetMaskFromStr( mask, signals );
  TEST_MASK_STRING_AT_INDEX( mask, 0, "01011111" );
  TEST_MASK_STRING_AT_INDEX( mask, 2, "00000100" );
  TEST_VALID_SIGNAL_LENGTH( mask, 10 );
  DeleteAIOChannelMask( mask );

  mask = NewAIOChannelMaskFromStr( "010101010101010100" );
  TEST_MASK_STRING_AT_INDEX( mask, 0,  "01010100" );
  TEST_MASK_STRING_AT_INDEX( mask, 1,  "01010101" );
  TEST_MASK_STRING_AT_INDEX( mask, 2,  "01" );
  TEST_MASK_STRING( mask, "010101010101010100" );
  TEST_VALID_SIGNAL_LENGTH( mask, 8 );
  DeleteAIOChannelMask( mask );

  mask = NewAIOChannelMaskFromStr("01010100011001010111001101110100");
  char *tmp = AIOChannelMaskGetMask( mask );
  printf("%s - %s, exp=%s got=%s\n", 
         (strncmp(tmp,"Test", AIOChannelMaskGetSize(mask)) == 0 ? "ok" : "not ok" ),
         "Got correct binary representation",
         "Test",
         tmp
         );
  DeleteAIOChannelMask( mask );

  mask = NewAIOChannelMaskFromStr("1111");
  TEST_MASK_STRING_AT_INDEX( mask , 0,  "1111" );

  AIOChannelMaskGetMaskAtIndex( mask, &tmpmask, 0 );
  TEST_VALUE_EQUAL( tmpmask , 15);
  free(tmp);
  DeleteAIOChannelMask( mask );


}

#endif
