
/* #include "AIODeviceTable.h" */
PUBLIC_EXTERN AIORESULT AIODeviceTableAddDeviceToDeviceTable( int *numAccesDevices, unsigned long productID ) ;
PUBLIC_EXTERN AIORESULT AIODeviceTableAddDeviceToDeviceTableWithUSBDevice( int *numAccesDevices, unsigned long productID , USBDevice *usb_dev );
PUBLIC_EXTERN AIORET_TYPE AIODeviceTablePopulateTable(void);
PUBLIC_EXTERN AIORET_TYPE AIODeviceTablePopulateTableTest(unsigned long *products, int length );
PUBLIC_EXTERN AIORESULT AIODeviceTableClearDevices( void );
PUBLIC_EXTERN AIORESULT ClearDevices( void );
PUBLIC_EXTERN AIOUSBDevice *AIODeviceTableGetDeviceAtIndex( unsigned long index , AIORESULT *res );
PUBLIC_EXTERN USBDevice *AIODeviceTableGetUSBDeviceAtIndex( unsigned long DeviceIndex, AIORESULT *res );
void _setup_device_parameters( AIOUSBDevice *device , unsigned long productID );

PUBLIC_EXTERN unsigned long QueryDeviceInfo( unsigned long DeviceIndex, unsigned long *pPID, unsigned long *pNameSize, char *pName, unsigned long *pDIOBytes, unsigned long *pCounters );
PUBLIC_EXTERN AIORET_TYPE GetDevices(void);
PUBLIC_EXTERN char *GetSafeDeviceName( unsigned long DeviceIndex );
PUBLIC_EXTERN char *ProductIDToName( unsigned int productID );
PUBLIC_EXTERN AIORET_TYPE ProductNameToID(const char *name);
PUBLIC_EXTERN AIORET_TYPE AIOUSB_Init(void);
PUBLIC_EXTERN AIORET_TYPE AIOUSB_EnsureOpen(unsigned long DeviceIndex);
PUBLIC_EXTERN AIOUSB_BOOL AIOUSB_IsInit();
PUBLIC_EXTERN AIORET_TYPE AIOUSB_Exit();
PUBLIC_EXTERN AIORET_TYPE AIOUSB_Reset( unsigned long DeviceIndex );
PUBLIC_EXTERN void AIODeviceTableInit(void);

PUBLIC_EXTERN void CloseAllDevices(void);
PUBLIC_EXTERN AIORESULT AIOUSB_GetAllDevices();

PUBLIC_EXTERN unsigned long AIOUSB_INIT_PATTERN;

/* #include "AIOUSB_CTR.h" */

PUBLIC_EXTERN AIORET_TYPE CTR_8254Mode(
                                         unsigned long DeviceIndex,
                                         unsigned long BlockIndex,
                                         unsigned long CounterIndex,
                                         unsigned long Mode );

PUBLIC_EXTERN AIORET_TYPE CTR_8254Load(
                                         unsigned long DeviceIndex,
                                         unsigned long BlockIndex,
                                         unsigned long CounterIndex,
                                         unsigned short LoadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ModeLoad(
                                             unsigned long DeviceIndex,
                                             unsigned long BlockIndex,
                                             unsigned long CounterIndex,
                                             unsigned long Mode,
                                             unsigned short LoadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadModeLoad(
                                                 unsigned long DeviceIndex,
                                                 unsigned long BlockIndex,
                                                 unsigned long CounterIndex,
                                                 unsigned long Mode,
                                                 unsigned short LoadValue,
                                                 unsigned short *pReadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254Read( unsigned long DeviceIndex,
                                        unsigned long BlockIndex,
                                        unsigned long CounterIndex,
                                        unsigned short *pReadValue );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadAll( unsigned long DeviceIndex,
                                           unsigned short *pData );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadStatus( unsigned long DeviceIndex,
                                              unsigned long BlockIndex,
                                              unsigned long CounterIndex,
                                              unsigned short *pReadValue,
                                              unsigned char *pStatus );

PUBLIC_EXTERN AIORET_TYPE CTR_StartOutputFreq( unsigned long DeviceIndex,
                                               unsigned long BlockIndex,
                                               double *pHz );

PUBLIC_EXTERN AIORET_TYPE CTR_8254SelectGate( unsigned long DeviceIndex,
                                              unsigned long GateIndex );

PUBLIC_EXTERN AIORET_TYPE CTR_8254ReadLatched( unsigned long DeviceIndex,
                                               unsigned short *pData );




#endif
