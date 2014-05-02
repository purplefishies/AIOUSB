% 
% $Date $Format: %ad$$
% $Author $Format: %an <%ae>$$
% $Release $Format: %t$$
% @desc Sample program to run the USB-DIO-16 from within Matlab
% @note This sample Triggers the relay switches. Note that the
% output is inverted such that if you write a '1' to a bit location
% then any LED connected to that pin will switch OFF, not ON
%
%
%

if strcmp(getenv('AIO_LIB_DIR'),'') 
    error('You must first source the sourceme file in ../../ ');
    exit();
end

addpath(getenv('AIO_LIB_DIR')) 

more off;

AIOUSB;
AIOUSB_GetVersion()
AIOUSB_GetVersionDate()
AIOUSB_Init();
deviceMask = GetDevices()
deviceIndex = 0;
AIOUSB_ListDevices()
productId = new_ulp()  
ulp_assign(productId,0)
QueryDeviceInfo(0, productId,new_ulp(),"",new_ulp(), new_ulp())

if ulp_value( productId ) == 32792
    stopval = 15
elseif ulp_value( productId ) == 32796
    stopval = 7
else
    tmp = sprintf('Card with board id "0x%x" is not supported by this sample', ulp_value( productId ) );
    disp(tmp);
    exit();
end

timeout = 1000;
AIOUSB_Reset(  deviceIndex );
AIOUSB_SetCommTimeout( deviceIndex, timeout );

outData = new_ulp()
ulp_assign(outData, 15 );

DIO_WriteAll( deviceIndex, outData );
readData = new_ulp();
ulp_assign(readData ,0);


for i=0:255
    val=sprintf('Sending %d\n',i);
    ulp_assign(outData, i );
    result = DIO_WriteAll( deviceIndex, outData );
    ## result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',i) );
    ## printf(val);
    disp(val);
    pause(0.04);
end

for i=0:stopval
    val=sprintf('Sending %d',2^i);
    ulp_assign(outData, 2^i );
    DIO_WriteAll(deviceIndex, outData );
    ## result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',2^i) );
    disp(val);
    pause(1);    
end

ulp_assign(outData, 0x5465 );
DIO_WriteAll(deviceIndex, outData );
disp('Reading back data');
buf = DIOBuf(10)
DIO_ReadAll( deviceIndex, buf );
val = sprintf("Binary was: %s", DIOBufToString(buf) );
disp(val);
val = sprintf("Hex was: %s", DIOBufToHex(buf) );
disp(val)
## exit();
