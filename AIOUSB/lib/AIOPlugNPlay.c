#include "AIOPlugNPlay.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif


#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member)*__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })

/**
 * @param pnpentry 
 * 
 * @return 
 */
AIOUSB_BOOL DeviceHasPNPByte(const AIOPlugNPlay *pnpentry )
{

    return AIOUSB_TRUE;
}
/* sort of a container of thingy */
/* pnpentry - pnpentry->dev[0];a */
/* I := Cardinal(@PNPEntry) - Cardinal(@Dev[0]); */
/* I := I div SizeOf(Dev[0]); */
/* Result := Dev[I].PNPData.PNPSize > (Cardinal(@PNPEntry) - Cardinal(@Dev[I].PNPData)); */
/* container_of( pnpentry, AIOPlugNPlay  , struct  */



#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#include "AIOUSBDevice.h"
#include "gtest/gtest.h"


#endif
