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
format long;

if strcmp(getenv('AIO_LIB_DIR'),'') 
    error('You must first source the sourceme file in ../../ ');
    exit();
end

addpath(getenv('AIO_LIB_DIR'));
loadlibrary('libaiousb','libaiousb.h', 'includepath','/usr/include/libusb-1.0', 'includepath',getenv('AIO_LIB_DIR'));

% To see the other functions offered, you can run this
% 
% libfunctions libaiousb -full
%
%

calllib('libaiousb','AIOUSB_GetVersion');
calllib('libaiousb','AIOUSB_GetVersionDate');
result = calllib('libaiousb','AIOUSB_Init' );
deviceMask = calllib('libaiousb','GetDevices');
deviceIndex = 0;
calllib('libaiousb','AIOUSB_ListDevices');      
productId = libpointer('ulongPtr',0);
calllib('libaiousb','QueryDeviceInfo',0,productId,[],libpointer('cstring'),[],[]);

if productId.Value == 32792
    stopval = 16;
elseif productId.Value == 32796
    stopval = 8;
else
    tmp = sprintf('Card with board id "0x%x" is not supported by this sample', productId.Value );
    disp(tmp);
    exit();
end

timeout = 1000;
result = calllib('libaiousb','AIOUSB_Reset', deviceIndex )
result = calllib('libaiousb','AIOUSB_SetCommTimeout',deviceIndex, timeout );


get(productId);
%    Value: 32796
% DataType: 'ulongPtr'


outData = libpointer('uint16Ptr',16) ;
result = calllib('libaiousb','DIO_WriteAll', 0,  outData );
readData = libpointer('uint16Ptr',0 );



result = calllib('libaiousb','DIO_WriteAll', deviceIndex,  outData );
% for j=1:1000
start = tic;
val=sprintf('Starting at %d',start);
disp(val);
for i=0:2^stopval-1
    % val=sprintf('Sending %d',i);
    result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',i) );
    % disp(val)                          
    % pause(0.04);                        
end
stop = toc(start);
val=sprintf('Completed in seconds %d\nCount per is %f',stop,stop/(2^stopval-1)  );
disp(val);
% end
% for i=0:stopval
%     val=sprintf('Sending %d',2^i);
%     result = calllib('libaiousb','DIO_WriteAll', 0, libpointer('uint16',2^i) );
%     disp(val);
%     pause(1);    
% end

result = calllib('libaiousb','DIO_ReadAll', deviceIndex, readData );

exit();
