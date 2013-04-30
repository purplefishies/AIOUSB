%module AIOUSB
%include "cpointer.i"


/* %inline { */
/*   extern unsigned long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex, unsigned long *BlockSize ); */
/* } */
/* %pointer_functions( unsigned long, ulp ); */


/* %include typemaps.i */
/* %apply unsigned long *INOUT { unsigned long *result }; */
/* %{ */
/*   extern unsigned long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex, unsigned long *BlockSize ); */
/* %} */
/* extern unsigned long AIOUSB_GetStreamingBlockSize(unsigned long DeviceIndex, unsigned long *INOUT ); */
 /* ALternative using pointer wrappers */


%{
  #include "AIOUSB_Core.h"
  #include "aiousb.h"
  #include "libusb.h"
%}

%include "AIOUSB_Core.h"
%include "aiousb.h"



