
%module AIOUSB
%include "cpointer.i"
%include "carrays.i"

%pointer_functions( unsigned long,  ulp );
%pointer_functions( int,  ip );
%pointer_functions( unsigned short, usp );
%pointer_functions( double , dp );
%pointer_functions( char , cp );
%array_functions( char , cstring );

%include typemaps.i
%apply unsigned long *INOUT { unsigned long *result };
%{
/*   extern unsigned long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex, unsigned long *BlockSize ); */
  extern unsigned long ADC_BulkPoll( unsigned long DeviceIndex, unsigned long *INOUT );
%}


%{
  #include "AIOUSB_Core.h"
  #include "AIOContinuousBuffer.h"
  #include "AIOChannelMask.h"
  #include "AIOTypes.h"
  #include "AIOUSB_DIO.h"
  #include "DIOBuf.h"
  #include "libusb.h"
  #include "aiousb.h"
%}

/* Needed to allow inclusion into Scala */
%pragma(java) modulecode=%{
    static {
        System.loadLibrary("AIOUSB");
    }
%}

%newobject CreateSmartBuffer;
%newobject NewBuffer;
%delobject AIOBuf::DeleteBuffer;

%include "aiousb.h"
%include "AIOUSB_Core.h"
%include "AIOContinuousBuffer.h"
%include "AIOChannelMask.h"
%include "AIOUSB_DIO.h"
%include "AIOTypes.h"
%include "DIOBuf.h"

%inline %{
  unsigned short *new_ushortarray(int size) {
      return (unsigned short *)malloc(size*sizeof(unsigned short));
  }

  void delete_ushortarray( unsigned short *ary ) {
    free(ary);
  }

  int ushort_getitem(unsigned short *ary, int index) {
    return (int)ary[index];
  }

  void ushort_setitem( unsigned short *ary, int index, int value ) {
    ary[index] = (unsigned short)value;
  }
%}

%extend AIOChannelMask { 
    AIOChannelMask( unsigned size ) { 
        return (AIOChannelMask *)NewAIOChannelMask( size );
    }
    ~AIOChannelMask() { 
        DeleteAIOChannelMask($self);
    }

    const char *__str__() {
        return AIOChannelMaskToString( $self );
    }

 }

%extend AIOContinuousBuf {

  AIOContinuousBuf( unsigned long deviceIndex, unsigned numScans, unsigned numChannels ) {
    return (AIOContinuousBuf *)NewAIOContinuousBufForCounts( deviceIndex, numScans, numChannels );
  }

  ~AIOContinuousBuf() {
    DeleteAIOContinuousBuf($self);
  }
}

%extend AIOBuf {
  AIOBuf(int bufsize)  {
    return (AIOBuf *)NewBuffer( bufsize );
  }
  ~AIOBuf()  {
    DeleteBuffer($self);
  }

}
%extend DIOBuf {

  DIOBuf( int size ) {
    return (DIOBuf *)NewDIOBuf( size );
  }

  DIOBuf( char *ary, int size_array ) {
    return (DIOBuf *)NewDIOBufFromChar(ary, size_array );
  } 

  DIOBuf( char *ary ) {
    return (DIOBuf*)NewDIOBufFromBinStr( ary );
  }

  ~DIOBuf() {
    DeleteDIOBuf( $self );
  }
  
  int get(unsigned index) { 
    return DIOBufGetIndex( $self, index );
  }

  int set(unsigned index, int value ) {
    return DIOBufSetIndex( $self, index , value );
  }

  char *hex() {
    return DIOBufToHex($self);
  }

  unsigned size() { 
     return DIOBufSize( $self );
  }
  
  DIOBuf *resize( unsigned size ) {
    return DIOBufResize( $self, size );
  } 

  const char *__str__() {
    return DIOBufToString( $self );
  }
}
#ifdef __cplusplus
   %extend DIOBuf {
     bool operator==( DIOBuf *b ) {
       int i;
       int equiv = 1;
       if ( b->_size != $self->_size )
         return 0;
       for ( int i = 0; i < b->_size; i ++ )
         equiv &= ( $self->_buffer[i] == b->_buffer[i] );
       
       return equiv == 1;
     }
     
     bool operator!=( DIOBuf *b ) {
    return !($self == b);
     }
   }
#endif


#if defined(SWIGPYTHON) | defined(SWIGLUA) 
%extend DIOBuf {
  int __getitem__( unsigned index ) {
    return DIOBufGetIndex( $self, index );
  }
  int __setitem__(unsigned index, int value ) {
    DIOBufSetIndex( $self, index, value );
  }
 }
#elif defined(SWIGOCTAVE)
%extend DIOBuf {

  int __brace__( unsigned index ) {
    return DIOBufGetIndex( $self, index );
  }
  int __brace_asgn__( unsigned index , int value ) {
    return DIOBufSetIndex( $self, index, value );
  }
  int __paren__( unsigned index ) {
    return DIOBufGetIndex( $self, index );
  }
  int __paren_asgn__( unsigned index , int value) {
    return DIOBufSetIndex( $self, index , value );
  }

 }
#elif defined(SWIGJAVA)
%extend AIOChannelMask {
  char *toFoo() {
      return AIOChannelMaskToString( $self );
  }
}

#elif defined(SWIGRUBY)
%extend DIOBuf {
int at( unsigned index ) {
return DIOBufGetIndex( $self, index );
}
}
#elif defined(SWIGPERL)
%perlcode %{
package  AIOUSB::DIOBuf;
sub newDESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    #my $tmp = $self;
    my $tmp = "" . $self;
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        AIOUSBc::delete_DIOBuf($self);
        delete $OWNER{$tmp};
    }
}
*AIOUSB::DIOBuf::DESTROY = *AIOUSB::DIOBuf::newDESTROY;
%}
#endif




