#include "AIOChannelMask.h"
#include "AIOTypes.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
namespace AIOUSB {
#endif

#define BIT_LENGTH(x) ( sizeof(x) * 8 )


AIOChannelMask * NewAIOChannelMask( unsigned int number_channels )
{
  AIOChannelMask *tmp = (AIOChannelMask *)malloc(sizeof(AIOChannelMask ));
  if( !tmp ) {
    goto out_NewAIOChannelMask;
  }
  tmp->signals = (aio_channel_obj *) malloc(number_channels / (sizeof(aio_channel_obj)*8) );
  if( !tmp->signals ) 
    goto out_cleansignals;

  tmp->number_signals = number_channels;
  tmp->active_signals = 0;
out_NewAIOChannelMask:
  return tmp;
out_cleansignals:
  free(tmp);
  tmp = NULL;
  return tmp;
}

void  DeleteAIOChannelMask( AIOChannelMask *mask )
{
  free(mask->signals);
  free(mask);
}


/**
 * @desc Sets the AIOChannelMask using the regular notion of or'ing of shifted bytes, 
 * 
 */
AIORET_TYPE AIOChannelMask_SetMaskFromInt( AIOChannelMask *obj, unsigned field , unsigned index )
{
  unsigned i;
  AIORET_TYPE ret = AIOUSB_SUCCESS;  
  int curindex = (index ) * sizeof(field) / sizeof(aio_channel_obj);
  int shift_val = index % ( sizeof(aio_channel_obj) / sizeof(field) );
  int total_shift = (shift_val * sizeof(field)*8);
  aio_channel_obj mask = (((aio_channel_obj)-1) & ~((  (aio_channel_obj)((unsigned)-1) << total_shift )));

  if ( index > obj->number_signals % sizeof(aio_channel_obj) ) { 
    ret = -AIOUSB_ERROR_INVALID_PARAMETER;
  }
  for ( i = 0 ; i < BIT_LENGTH(field) ; i ++ )
    obj->active_signals += (( 1 << i ) & field ? 1 : 0 );

  obj->signals[ curindex ] = ((aio_channel_obj)field << total_shift) | (obj->signals[curindex] & mask );

  return ret;
}

/**
 * @desc
 * @param channels
 **/
AIORET_TYPE AIOChannelMask_NumberChannels( AIOChannelMask *obj )
{
  return (AIORET_TYPE)obj->active_signals;
}


/**
 * @desc Rely on the base type to determine the sizes
 * @param obj 
 * @param bitfields a character string that contains 0s and 1s. 
 *
 */
AIORET_TYPE AIOChannelMask_SetMask( AIOChannelMask *obj, const char *bitfields )
{
  int i;
  assert( strlen(bitfields) == obj->number_signals );

  AIORET_TYPE ret = AIOUSB_SUCCESS;
  unsigned j;
  aio_channel_obj tmpval = 0;
  int index = 0;
  for( i = strlen(bitfields) ; i > 0 ; i -- ) {
    j = strlen(bitfields)-i;
    if( bitfields[i-1] == '1' ) {
      tmpval |= 1 << j ;
      obj->active_signals ++;
    }
    /* tmpval |= (( bitfields[i-1] == '1' ? 1 : 0 ) << j); */
    if ( j >= obj->number_signals-1 ) {
      obj->signals[index++] = tmpval;
      tmpval = 0;
    }
  }
  return ret;
}

/**
 * @desc Creates a new AIOChannelMask object from a character string of 1's and 0's
 * @param bitfields
 * @return a new AIOChannelMask object 
 * @todo Add smarter error checking
 */
AIOChannelMask *NewAIOChannelMaskFromStr( const char *bitfields ) 
{
  AIOChannelMask *tmp = NewAIOChannelMask( strlen(bitfields) );
  AIOChannelMask_SetMask( tmp, bitfields );
  return tmp;
}

/**
 * @param obj 
 * @param bitfields a string of bitfields that we will apply
 * @param index 
 * 
 * @return 
 * @todo Implement this function
 */
AIORET_TYPE AIOChannelMask_SetMaskWithIndex( AIOChannelMask *obj, const char *bitfields , unsigned index )
{
  assert(0);
  return (AIORET_TYPE)-1;
}

AIORET_TYPE AIOChannelMask_GetMaskWithIndex( AIOChannelMask *obj, unsigned index )
{
  unsigned retvalue = -1;
  return retvalue;
}

/**
 * @desc Returns a mask for the index in question
 * @param obj 
 * @param index
 */
const char * AIOChannelMask_GetMask( AIOChannelMask *obj, unsigned index )
{
  static char retval[BIT_LENGTH(aio_channel_obj)+1];
  memset(retval,'\0',BIT_LENGTH(aio_channel_obj)+1 );
  unsigned i;
  /* for ( i = 0 ; i <  BIT_LENGTH( aio_channel_obj ) ; i ++ ) { */
  for ( i = 0 ; i <  obj->number_signals ; i ++ ) {
    /* int j = (BIT_LENGTH(aio_channel_obj) - i - 1 ); */
    int j = obj->number_signals - i-1;
    /* retval[BIT_LENGTH(aio_channel_obj)-i-1] = ( ((1 << j ) & obj->signals[index]) ? '1' : '0' ); */
    retval[i] = ( ( ( 1 << j ) & obj->signals[index] ) ? '1' : '0' );
    /* retval[i] = ( ((1 << j ) & obj->signals[index]) ? '1' : '0' ); */
  }

  return retval;
}


#ifdef __cplusplus
}
#endif


#ifdef SELF_TEST
int main(int argc, char *argv[] )
{
  AIORET_TYPE retval;
  char signals[32] = {'\0'};
  AIOChannelMask *mask = NewAIOChannelMask( 32 );
  AIOChannelMask_SetMaskFromInt( mask , 1 | 2 | 1 << 3 | 1 << 7 , 0);
  printf("1..6\n");

  printf("%s - %s\n", (strcmp(AIOChannelMask_GetMask( mask, 0 ),  "00000000000000000000000010001011") == 0  ? "ok" : "not ok" ), "decode of  matches 00000000000000000000000010001011" );
  printf("%s - %s, exp=%d got=%d\n", (AIOChannelMask_NumberChannels( mask ) == 4 ? "ok" : "not ok" ) , "Got correct number of channels", 4, AIOChannelMask_NumberChannels( mask ) ); 


  DeleteAIOChannelMask( mask );
  strcpy(signals,"00000000000001000011000101011111" );
  mask = NewAIOChannelMask( strlen(signals) );
  AIOChannelMask_SetMask( mask, signals );
  printf("%s - %s%s\n", (strcmp(AIOChannelMask_GetMask( mask, 0 ),  signals) == 0  ? "ok" : "not ok" ), "decode of ",signals );

  printf("%s - %s, exp=%d got=%d\n", (AIOChannelMask_NumberChannels( mask ) == 10 ? "ok" : "not ok" ) , "Got correct number of channels", 10, AIOChannelMask_NumberChannels( mask ) ); 

  DeleteAIOChannelMask( mask );

  mask = NewAIOChannelMaskFromStr("01010101010101010" );
  printf("%s - %s%s%s\n", (strcmp(AIOChannelMask_GetMask( mask, 0 ),  "01010101010101010" ) == 0  ? "ok" : "not ok" ), "decode of ", "01010101010101010 was ", AIOChannelMask_GetMask( mask, 0 ));

  printf("%s - %s, exp=%d got=%d\n", (AIOChannelMask_NumberChannels( mask ) == 8 ? "ok" : "not ok" ) , "Got correct number of channels", 8, AIOChannelMask_NumberChannels( mask ) ); 
  DeleteAIOChannelMask( mask );

}

#endif
