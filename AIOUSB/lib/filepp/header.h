#ifndef LIBAIOUSB_H
#define LIBAIOUSB_H


#include <stdint.h>
typedef unsigned long AIORESULT;
typedef int64_t AIORET_TYPE;

/* only needed because of mexFunction below and mexPrintf */
#include "AIOUSB_Core.h"
#include "AIOChannelMask.h"
#include "AIOContinuousBuffer.h"
#include "AIODataTypes.h"
#include "AIOTypes.h"

/* #include <pthread.h> */

#include <mex.h>  

