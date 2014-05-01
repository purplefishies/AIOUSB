
%module AIOUSB
%include "cpointer.i"
%include "carrays.i"

%pointer_functions( unsigned long,  ulp );
%pointer_functions( unsigned short, usp );
%pointer_functions( double , dp );
/* %pointer_functions( char, cstring ); */
%array_functions( char , cstring );

%include typemaps.i
%apply unsigned long *INOUT { unsigned long *result };
%{
/*   extern unsigned long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex, unsigned long *BlockSize ); */
  extern unsigned long ADC_BulkPoll( unsigned long DeviceIndex, unsigned long *INOUT );
%}


%{
  #include "aiousb.h"
  #include "AIOUSB_Core.h"
  #include "libusb.h"
  #include "AIOContinuousBuffer.h"
  #include "AIOTypes.h"
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
%include "AIOTypes.h"



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




