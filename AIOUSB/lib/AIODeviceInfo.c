#include "AIODeviceInfo.h"
#include "AIOUSB_Core.h"

#ifdef __cplusplus
namespace AIOUSB {
#endif

PUBLIC_EXTERN AIODeviceInfo *NewAIODeviceInfo() {
    AIODeviceInfo *tmp = (AIODeviceInfo *)malloc(sizeof(AIODeviceInfo));
    return tmp;
}

PUBLIC_EXTERN void DeleteAIODeviceInfo( AIODeviceInfo *di ) { 
    if ( di->Name ) 
        free(di->Name);
    free(di);
}

PUBLIC_EXTERN const char *AIODeviceInfoGetName( AIODeviceInfo *di )
{
    return di->Name;
}

PUBLIC_EXTERN unsigned AIODeviceInfoGetCounters( AIODeviceInfo *di ) 
{
    return di->Counters;
}

PUBLIC_EXTERN unsigned AIODeviceInfoGetDIOBytes( AIODeviceInfo *di ) 
{
    return di->DIOBytes;
}

PUBLIC_EXTERN AIODeviceInfo *GetDeviceInfo( unsigned long DeviceIndex )
{
    if ( !AIOUSB_Lock() ) { 
        aio_errno = -AIOUSB_ERROR_INVALID_MUTEX;
        return NULL;
    }
    AIODeviceInfo *tmp = NewAIODeviceInfo();
    if ( !tmp ) {
        aio_errno = -AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    memset(tmp,0,sizeof(AIODeviceInfo));
    AIOUSB_UnLock();
    unsigned long result = AIOUSB_Validate(&DeviceIndex);
    if ( result != AIOUSB_SUCCESS ) {
        AIOUSB_UnLock();
        aio_errno = -result;
        return NULL;
    }

    DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];

    tmp->PID       = deviceDesc->ProductID;
    tmp->DIOBytes  = deviceDesc->DIOBytes;
    tmp->Counters  = deviceDesc->Counters;

    AIOUSB_UnLock();  /* unlock while communicating with device */
    
    const char *deviceName = ProductIDToName( tmp->PID );
    if ( deviceName ) {
        tmp->Name = strdup( deviceName );
    }

    return tmp;

}

#ifdef __cplusplus
}
#endif

#ifdef SELF_TEST

#include <math.h>
#include "gtest/gtest.h"
#include "tap.h"
#include "AIOUSB_Core.h"

#ifdef __cplusplus
using namespace AIOUSB;
#endif


TEST(AIODeviceInfo,Get_Defaults) {
    /* Mock object that looks like a DeviceTable */
    unsigned long products[] = {USB_AIO16_16A, USB_DIO_32};
    PopulateDeviceTableTest( products, 2 );
    AIODeviceInfo *tmp = GetDeviceInfo( 0 );
    EXPECT_STREQ(AIODeviceInfoGetName(tmp), "USB-AIO16-16A" );
    EXPECT_EQ(AIODeviceInfoGetCounters(tmp), 1 );
    EXPECT_EQ(AIODeviceInfoGetDIOBytes(tmp), 2 );
    DeleteAIODeviceInfo( tmp );
    
}

int main( int argc , char *argv[] ) 
{
    testing::InitGoogleTest(&argc, argv);
    testing::TestEventListeners & listeners = testing::UnitTest::GetInstance()->listeners();
#ifdef GTEST_TAP_PRINT_TO_STDOUT
    delete listeners.Release(listeners.default_result_printer());
#endif
    listeners.Append( new tap::TapListener() );
   
    return RUN_ALL_TESTS();  
}


#endif
