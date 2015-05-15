
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
  extern unsigned long ADC_BulkPoll( unsigned long DeviceIndex, unsigned long *INOUT );
%}


%{
  #include "AIOUSB_Core.h"
  #include "ADCConfigBlock.h"
  #include "AIOContinuousBuffer.h"
  #include "AIOChannelMask.h"
  #include "AIODeviceTable.h"    
  #include "AIOUSBDevice.h"
  #include "AIODeviceInfo.h"
  #include "AIOUSB_Properties.h"
  // #include "AIOUSB_ADC.h"
  #include "AIOUSB_DAC.h"
  #include "AIOUSB_CTR.h"
  #include "AIOTypes.h"
  #include "cJSON.h"
  #include "AIOUSB_DIO.h"
  #include "DIOBuf.h"
  #include "libusb.h"
%}

/* Needed to allow inclusion into Scala */
%pragma(java) modulecode=%{
    static {
        System.loadLibrary("AIOUSB");
    }
%}

%newobject CreateSmartBuffer;
%newobject NewAIOBuf;
%delobject AIOBuf::DeleteAIOBuf;

#if defined(SWIGPYTHON)
%typemap(in) unsigned char *gainCodes {
    int i;
    static unsigned char temp[16];

    if (!PySequence_Check($input)) {
        PyErr_SetString(PyExc_ValueError,"Expected a sequence");
        return NULL;
    }
    if (PySequence_Length($input) != 16 ) {
        PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected 16 elements");
        return NULL;
    }
    for (i = 0; i < 16; i++) {
        PyObject *o = PySequence_GetItem($input,i);
        if (PyNumber_Check(o)) {
            temp[i] = (unsigned char) PyFloat_AsDouble(o);
        } else {
            PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
            return NULL;
        }
    }
    $1 = temp;
}

//
// 
%typemap(in)  double *voltages
{
    double temp[256];
    $1 = temp;
}

%typemap(argout) (unsigned long DeviceIndex, double *voltages)  {
    int i;
    AIORESULT result = AIOUSB_SUCCESS;
    AIOUSBDevice *deviceDesc = AIODeviceTableGetDeviceAtIndex( $1, &result );
    if ( result != AIOUSB_SUCCESS ) {
        PyErr_SetString(PyExc_ValueError,"Invalid DeviceIndex");
        return NULL;
    }
    int tmpsize = deviceDesc->ADCMUXChannels;
    $result = PyList_New(tmpsize);
    for (i = 0; i < tmpsize; i++) {
        PyObject *o = PyFloat_FromDouble((double) $2[i]);
        PyList_SetItem($result,i,o);
    }
}

%typemap(argout) (unsigned long DeviceIndex, unsigned long ChannelIndex, double *voltages )
{
    // printf("debugging");
    if ( result != AIOUSB_SUCCESS ) {
        PyErr_SetString(PyExc_ValueError,"Invalid DeviceIndex");
        return NULL;
    }

    $result = PyList_New(1);

    PyObject *o = PyFloat_FromDouble((double) $3[$2]);
    PyList_SetItem($result,0,o);
}

%typemap(in)  double *ctrClockHz {
    double tmp = PyFloat_AsDouble($input);
    $1 = &tmp;
}

#elif defined(SWIGPERL)

%typemap(in)  double *voltages {
    unsigned short temp[256];
    $1 = temp;
}

%typemap(argout) double *voltages {
    AV *myav;
    SV **svs;
    int i = 0,len = 16;
    /* Figure out how many elements we have */
    svs = (SV **) malloc(len*sizeof(SV *));
    for (i = 0; i < len ; i++) {
        svs[i] = sv_newmortal();

        sv_setnv( (SV*)svs[i],(double)$1[i] );
    };
    myav = av_make(len,svs);
    free(svs);
    $result = newRV_noinc((SV*)myav);
    sv_2mortal($result);
    argvi++;
}

%typemap(in) unsigned char *gainCodes {
    AV *tempav;
    I32 len;
    int i;
    SV **tv;

    static unsigned char temp[16];
    if (!SvROK($input))
        croak("Argument $argnum is not a reference.");
    if (SvTYPE(SvRV($input)) != SVt_PVAV)
        croak("Argument $argnum is not an array.");

    tempav = (AV*)SvRV($input);
    len = av_len(tempav);
    if ( (int)len != 16-1 )  {
        croak("Bad stuff: length was %d\n", (int)len);
    }
    for (i = 0; i <= len; i++) {
        tv = av_fetch(tempav, i, 0);
        temp[i] = (unsigned char) SvNV(*tv );
        // printf("Setting value %d\n", (int)SvNV(*tv ));
    }
    
    $1 = temp;
}

%typemap(in)  double *ctrClockHz {
    double tmp = SvIV($input);
    $1 = &tmp;
}

#elif defined(SWIGRUBY)

%typemap(in)  double *ctrClockHz {
    double tmp = NUM2DBL($input);
    // printf("Type was %d\n", (int)tmp );
    $1 = &tmp;
}

%typemap(in) unsigned char *gainCodes {
    int i;
    static unsigned char temp[16];

    if ( RARRAY_LEN($input) != 16 ) {
        rb_raise(rb_eIndexError, "Length is not valid ");
    }
    for (i = 0; i < 16; i++) {
        temp[i] = (unsigned char)NUM2INT(rb_ary_entry($input, i)); 
        /* printf("Setting temp[%d] to %d\n", i, (int)temp[i] ); */
    }
    $1 = temp;
}

%typemap(in)  double *voltages {
    double temp[256];
    $1 = temp;
}

%typemap(argout) double *voltages {
    int i;
    // printf("Debugging ruby voltages\n");
    $result = rb_ary_new2(16);
    // printf("Allocated buffer\n");
    for (i = 0; i < 16; i++) {
        rb_ary_store( $result, i, rb_float_new((double)$1[i]));
    }
}

#endif

unsigned long ADC_RangeAll( unsigned long DeviceIndex, unsigned char *gainCodes ,unsigned long bSingleEnded );
unsigned long ADC_GetScanV( unsigned long DeviceIndex, double *voltages );
unsigned long ADC_GetChannelV(unsigned long DeviceIndex, unsigned long ChannelIndex, double *voltages );
unsigned long CTR_StartOutputFreq( unsigned long DeviceIndex,  unsigned long BlockIndex, double *ctrClockHz );


%include "AIOUSB_Core.h"
%include "ADCConfigBlock.h"
%include "AIOContinuousBuffer.h"
%include "AIOUSB_Properties.h"
%include "AIOChannelMask.h"
%include "AIODeviceTable.h"    
%include "AIOUSB_ADC.h"
%include "AIOUSB_DAC.h"
%include "AIOUSB_CTR.h"
%include "AIOUSBDevice.h"
%include "AIODeviceInfo.h"
%include "AIOUSB_DIO.h"
%include "cJSON.h"
%include "AIOTypes.h"
%include "DIOBuf.h"

%array_functions(unsigned short, counts )
%array_functions(double, volts )

%inline %{
    /* For handling counts */
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

    void print_array(double x[10]) {
       int i;
       for (i = 0; i < 10; i++) {
          printf("[%d] = %g\n", i, x[i]);
       }
    }

    // Ushort_Array new_Ushort_Array(int nelements) {
    //     Ushort_Array tmp;
    //     tmp._ary = (unsigned short *)calloc(nelements,sizeof(unsigned short));
    //     tmp.size = nelements;
    //     return tmp;
    // }

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
    return (AIOBuf *)NewAIOBuf( bufsize );
  }
  ~AIOBuf()  {
    DeleteAIOBuf$self);
  }

}
%extend diobuf_type {

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
    return DIOBufSetIndex( $self, index, value );
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




