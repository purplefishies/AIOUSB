/**
 * @file   AIOUSB_Properties.h
 * @author $Author$
 * @date   $Date$
 * @copy
 * @brief
 */
#ifndef _AIOUSB_PROPERTIES_H
#define _AIOUSB_PROPERTIES_H

#include "AIOUSB_Core.h"
#include "AIOTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#ifdef __aiousb_cplusplus       /* Required for header file inclusion and SWIG */
namespace AIOUSB
{
#endif

PUBLIC_EXTERN unsigned long AIOUSB_GetDeviceByProductID(int minProductID,
                                                        int maxProductID,
                                                        int maxDevices,
                                                        int *deviceList
                                          );
PUBLIC_EXTERN unsigned long GetDeviceBySerialNumber(unsigned long *pSerialNumber);
PUBLIC_EXTERN unsigned long AIOUSB_GetDeviceProperties(
                                                       unsigned long DeviceIndex,
                                                       DeviceProperties *properties
                                                       );
PUBLIC_EXTERN const char *AIOUSB_GetResultCodeAsString(unsigned long result_value);

PUBLIC_EXTERN void AIOUSB_ListDevices();
PUBLIC_EXTERN AIORESULT FindDevices( int **indices, int *length , unsigned minProductID, unsigned maxProductID  );
PUBLIC_EXTERN AIORESULT AIOUSB_GetAllDevices();




#ifdef __aiousb_cplusplus       /* Required for header file inclusion and SWIG */
}
#endif

#endif
