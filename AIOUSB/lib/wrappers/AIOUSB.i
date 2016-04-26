
%module AIOUSB
%include "cpointer.i"
%include "carrays.i"
%include "typemaps.i"
%include "aioarrays.i"
#if defined(SWIGJAVA)
%include "enums.swg"
#else

#endif

%feature("autodoc", "1");


%pointer_functions( unsigned long,  ulp );
%pointer_functions( long long, ullp );
%pointer_functions( double,  udp );
%pointer_functions( int,  ip );
%pointer_functions( unsigned short, usp );
%pointer_functions( double , dp );
%pointer_functions( char , cp );
%pointer_functions( unsigned char , ucp );
%array_functions( char , cstring );


%apply unsigned long *INOUT { unsigned long *result };
%apply long long { int64_t };
%apply unsigned long long { uint64_t };


%apply AIORESULT *OUTPUT { unsigned long *result };

%{
  extern unsigned long ADC_BulkPoll( unsigned long DeviceIndex, unsigned long *INOUT );
%}

%{
  #include "AIOTypes.h"
  #include "AIOUSB_Core.h"
  #include "AIOCommandLine.h"
  #include "ADCConfigBlock.h"
  #include "AIOContinuousBuffer.h"
  #include "AIOChannelMask.h"
  #include "AIODeviceTable.h"    
  #include "AIODeviceQuery.h"
  #include "AIOUSBDevice.h"
  #include "AIODeviceInfo.h"
  #include "AIOUSB_Properties.h"
  #include "AIOUSB_DAC.h"
  #include "AIOUSB_ADC.h"
  #include "AIOUSB_CTR.h"
  #include "cJSON.h"
  #include "AIOUSB_DIO.h"
  #include "AIOBuf.h"
  #include "DIOBuf.h"
  #include "libusb.h"
  #include <pthread.h>

%}

#if defined(SWIGPYTHON)
%typemap(in) unsigned char *pGainCodes {
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

%typemap(in) (int *argc, char **argv) {
    int i;
    $2 = NULL;
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a list");
        return NULL;
    }
    $1 = (int *)malloc(sizeof(int));
    if (!$1 ) 
        return NULL;
    *$1 = PyList_Size($input);
    $2 = (char **) malloc((*$1+1)*sizeof(char *));
    for (i = 0; i < *$1; i++) {
        PyObject *s = PyList_GetItem($input,i);
        if (!PyString_Check(s)) {
            free($2);
            PyErr_SetString(PyExc_ValueError, "List items must be strings");
            return NULL;
        }
        $2[i] = PyString_AsString(s);
    }
    $2[i] = 0;
}
/* After arguments have been processed....
 * we set the argv to be this new value */
%typemap(argout) (int *argc, char **argv) {
    PyList_SetSlice($input, 0, PyList_Size($input), NULL);
    {
        int i; 
        for ( i = 0; i < *$1 ; i ++ ) { 
            /* printf("Adding %s\n", $2[i]); */
            PyObject *ofmt = SWIG_Python_str_FromChar( $2[i] );
            PyList_Append( $input, ofmt );
        }
    } 
}

%typemap(freearg) (int *argc, char **argv) {
    if ($1) free($1);
    if ($2) free($2);
}


#endif

/* Needed to allow inclusion into Scala */
%pragma(java) modulecode=%{
    static {
        System.loadLibrary("AIOUSB");
    }
%}

%newobject CreateSmartBuffer;
%newobject NewAIOBuf;
%newobject NewAIODeviceQuery;
%delobject AIOBuf::DeleteAIOBuf;


AIOUSBDevice *AIODeviceTableGetDeviceAtIndex( unsigned long DeviceIndex , unsigned long *OUTPUT );



#if defined(SWIGPYTHON)

%typemap(in)  double *pBuf
{
    double temp[256];
    $1 = temp;
}

%typemap(in)  double *ctrClockHz {
    double tmp = PyFloat_AsDouble($input);
    $1 = &tmp;
}

#elif defined(SWIGPERL)

%typemap(in) unsigned char *pGainCodes {
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

#endif

/* Typemaps should remain before the includes */
%include "AIOTypes.h"
%include "AIOUSB_Core.h"
%include "ADCConfigBlock.h"
%include "AIOCommandLine.h"
%include "AIOContinuousBuffer.h"
%include "AIOUSB_Properties.h"
%include "AIOChannelMask.h"
%include "AIODeviceTable.h"    
%include "AIODeviceQuery.h"
%include "AIOUSB_ADC.h"
%include "AIOUSB_DAC.h"
%include "AIOUSB_CTR.h"
%include "AIOUSBDevice.h"
%include "AIODeviceInfo.h"
%include "AIOUSB_DIO.h"
%include "cJSON.h"
%include "AIOBuf.h"
%include "DIOBuf.h"

%aioarray_class(unsigned short,ushortarray)
%aioarray_class(double,doublearray)

%inline %{
AIORET_TYPE ADC_GetScanVToDoubleArray( unsigned long DeviceIndex, doublearray *ary ) {
    return ADC_GetScanV( DeviceIndex, ary->el );
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

%extend AIODeviceQuery { 

    AIODeviceQuery( unsigned long DeviceIndex ) {
        return (AIODeviceQuery*)NewAIODeviceQuery( DeviceIndex );
    }

    ~AIODeviceQuery() { 
        DeleteAIODeviceQuery( $self );
    }

    char *__str__() { 
        return AIODeviceQueryToStr( $self );
    }    

    char *__repr__() { 
        return AIODeviceQueryToRepr( $self );
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
   AIOBuf(int bufsize, int refsize)  {
       return (AIOBuf *)NewAIOBuf( (AIOBufType)bufsize, refsize );
   }  
  ~AIOBuf()  {
       DeleteAIOBuf($self);
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
  
  int get(int index) { 
    return DIOBufGetIndex( $self, index );
  }

  int set(int index, int value ) {
    return DIOBufSetIndex( $self, index , value );
  }

  char *hex() {
    return DIOBufToHex($self);
  }

  char *to_hex() {
    return DIOBufToHex($self);
  }

  DIOBuf *resize( unsigned size ) {
    return DIOBufResize( $self, size );
  } 

  const char *__str__() {
    return DIOBufToString( $self );
  }
  
  const char *__repr__() { 
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





#if defined(SWIGPYTHON)
%pythoncode %{
from AIOUSB import doublearray as volts
from AIOUSB import ushortarray as counts

def new_ushortarray(n):
    """Creates a new ushortarray of size n"""
    import AIOUSB
    return AIOUSB.ushortarray(n)

def delete_ushortarray(n):
    del n

def new_volts(n):
    """Creates a new double array of size n"""
    import AIOUSB
    return AIOUSB.volts(n)

def delete_volts(n):
    """Creates a new double array of size n"""
    del n


%}


/* Special conversion so that binary string with multiple zeros aren't truncated */
%extend DIOBuf {
    
    %typemap(out) char * {
         $result = PyString_FromStringAndSize( $1, MAX(strlen($1),DIOBufByteSize( arg1 )) );
    }

    char *to_bin() {
        return DIOBufToBinary($self);
    }

    %typemap(out) char *; /* Remove this for other char * returning methods */
}


%exception DIOBuf::__getitem__ { 
    $action 
    if ( result < 0 ) {
          PyErr_SetString(PyExc_IndexError,"Index out of range");
          return NULL;
    }
}

%exception DIOBuf::__setitem__ { 
    $action 
    if ( result < 0 ) { 
        if( result == -AIOUSB_ERROR_INVALID_INDEX ) {
          PyErr_SetString(PyExc_IndexError,"Index out of range");
          return NULL;
        } else if ( result == -AIOUSB_ERROR_INVALID_PARAMETER ) { 
          PyErr_SetString(PyExc_TypeError,"Invalid value");
          return NULL;
        }
    }
}




%extend DIOBuf {
  int __getitem__( int index ) {
    return DIOBufGetIndex( $self, index );
  }
  int __setitem__(int index, int value ) {
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
  %extend DIOBuf {
      const char *toString() {
          return DIOBufToString( $self );
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




