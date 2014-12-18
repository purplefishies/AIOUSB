#!/usr/bin/python

import usb.core
import usb.util
import sys
import struct
import time
import math
import itertools
from threading import Thread, Event
import time

#productID=0x8140  
productID=0x8140
vendorID=0x1605
DELAY=10000

# find our device

def find_device():
    global dev
    dev = usb.core.find(idVendor=vendorID, idProduct=productID)
    return dev

    
def init_device(dev):
    # dev.set_configuration()
    cfg = dev.get_active_configuration()
    intf = cfg[(0,0)]

def reload_device(dev):
    thisep = get_configuration(dev)
    return thisep

def get_write_endpoint(dev):
    for cfg in dev:
        sys.stdout.write(str(cfg.bConfigurationValue) + '\n')
        for intf in cfg:
            sys.stdout.write('\t' + \
                             str(intf.bInterfaceNumber) + \
                             ',' + \
                             str(intf.bAlternateSetting) + \
                             '\n')
            for ep in intf:
                if ep.bEndpointAddress == 0x2:
                    thisep = ep
                sys.stdout.write('\t\t' + \
                                 str(ep.bEndpointAddress) + \
                                 '\n')
    return thisep


def get_read_endpoint(dev):
    for cfg in dev:
        # sys.stdout.write(str(cfg.bConfigurationValue) + '\n')
        for intf in cfg:
            # sys.stdout.write('\t' + \
            #                  str(intf.bInterfaceNumber) + \
            #                  ',' + \
            #                  str(intf.bAlternateSetting) + \
            #                  '\n')
            for ep in intf:
                if ep.bEndpointAddress == 0x86:
                    thisep = ep
                # sys.stdout.write('\t\t' + \
                #                  str(ep.bEndpointAddress) + \
                #                  '\n')
    return thisep


def get_configuration(dev):
    for cfg in dev:
        sys.stdout.write(str(cfg.bConfigurationValue) + '\n')
        for intf in cfg:
            sys.stdout.write('\t' + \
                             str(intf.bInterfaceNumber) + \
                             ',' + \
                             str(intf.bAlternateSetting) + \
                             '\n')
            for ep in intf:
                if ep.bEndpointAddress == 0x86:
                    thisep = ep
                sys.stdout.write('\t\t' + \
                                 str(ep.bEndpointAddress) + \
                                 '\n')
    return thisep


def send_control_block( data ):
    packet = "\x40\xBE\x00\x00\x00\x00\x14\x00"
    [bmRequestType, bRequest, wValue, wIndex, wLength ] = struct.unpack("<BBHHH", packet[0:8] )
    tmpdat = struct.pack("<%dB" % len(data) , *data )
    tmpdat = struct.pack("<BBBBBBBBBBBBBBBBBBBB" , *data )
    data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , tmpdat )  
    return data



def init_board():
    sendPacket( "\x80\x06\x00\x01\x00\x00\x12\x00" )


def continuous_setup(clock=1000):
    # THE BB Command
    if False:

        # First ask for the config object
        # sendPacket( " " )
        sendPacket( "\xC0\xBA\x00\x00\x00\x00\x01\x00", [], True )
        tmpdat = [ 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x15, 0xF0, 0x00 ]
        sendPacket( "\x40\xBE\x00\x00\x00\x00\x14\x00", tmpdat )
        sendPacket( "\x40\x21\x00\x74\x00\x00\x00\x00" )
        sendPacket( "\x40\x21\x00\xB6\x00\x00\x00\x00" )
        sendPacket( "\x40\xC5\x64\x00\x64\x00\x00\x00" )
        sendPacket( "\x40\xBC\x00\x00\x00\x00\x04\x00" , data ) # Starts counters
    else:
        sendPacket( "\xC0\xBA\x00\x00\x00\x00\x01\x00" ,[] , True )
        tmpdat = [ 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x15, 0xF0, 0x00 ]
        sendPacket( "\x40\xBE\x00\x00\x00\x00\x14\x00", tmpdat )
        sendPacket( "\x40\x21\x00\x74\x00\x00\x00\x00" )
        sendPacket( "\x40\x21\x00\xB6\x00\x00\x00\x00" )
        # sendPacket( "\x40\x23\x00\x74\x64\x00\x00\x00" )
        # sendPacket( "\x40\x23\x00\xB6\x09\x00\x00\x00" )
        tmpdat = [0x07,0x00,0x00,0x01]
        sendPacket( "\x40\xBC\x00\x00\x00\x00\x04\x00" , tmpdat )
        sendPacket( "\x40\x23\x00\x74\x64\x00\x00\x00" )
        sendPacket( "\x40\x23\x00\xB6\x64\x00\x00\x00" )

def continuous_end():
    global dev
    data = [0x02,0x00,0x02,0x00 ]
    if False:
        sendPacket("\x40\xBC\x00\x00\x00\x00\x04\x00" , data )
        #Possible
        sendPacket("\x40\x35\x00\x00\x00\x00\x00\x00"  )
    else:
        sendPacket("\x40\xBC\x00\x00\x00\x00\x04\x00" , data )
        sendPacket("\x40\x35\x00\x00\x00\x00\x00\x00" )
        sendPacket("\x40\x21\x00\x74\x00\x00\x00\x00" )
        sendPacket("\x40\x21\x00\xB6\x00\x00\x00\x00")

    usb.util.dispose_resources(dev)

def send_bb(dev, wValue):
    bmRequestType = 0x40
    bRequest = 0xbb
    wIndex   = 0x0400
    wLength  = 0x00
    tmpdata  = []
    data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , tmpdata ) 
    data

def read_config(dev):
    bmRequestType = 0xC0
    bRequest = 0xd2
    wValue = 0x00
    wIndex = 0x00
    wLength = 0x14
    data = dev.ctrl_transfer( 0xC0, bRequest , wValue, wIndex, wLength )
    return data



def write_config(dev, config ):
    bmRequestType = 0x40
    bRequest = 0xbe
    wValue = 0x00
    wIndex = 0x00
    dev.ctrl_transfer( bmRequestType, bRequest, wValue, wIndex, config )


def get_caltable_config():
    config = [ 0 for x in  range(0,20)]
    config[17] = 0x05
    config[18] = 0xF0
    config[19] = 0x0e
    return config

def bulk_read(dev):
    # 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 05 F0 00
    data =[]
    config = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x05,0xf0,0x0e]
    write_config(dev, config )
    dev.ctrl_transfer( 0x40, 0x21, 0x7400,0x0000,data)
    dev.ctrl_transfer( 0x40, 0x21, 0xb600,0x0000,data)
    write_config(dev, config )
    dev.ctrl_transfer( 0x40, 0x23, 0x7400,0x64,data)
    dev.ctrl_transfer( 0x40, 0x23, 0xb600,0x64,data)
    data = [0x05,0x00,0x00,0x00 ]
    numHigh = 0x2
    numLow = 0x0
    print "High: %x" % ( numHigh )
    print "Low: %x" % ( numLow )
    dev.ctrl_transfer( 0x40, 0xbc, numHigh, numLow, data )
    thisep = get_read_endpoint(dev)
    total = 0
    for i in range(0,512):
        tmp = thisep.read(512)
        total += len(tmp)
    data = []
    dev.ctrl_transfer( 0x40, 0x21, 0x7400,0x0000,data)
    dev.ctrl_transfer( 0x40, 0x21, 0xb600,0x0000,data)
    return total

#
#
#
# for i in range(0,n):
#        print "Input thisep=%s , n=%d, data=%s" % ( thisep, n, data )
def read_data(thisep, n, end_event, data,readsize=4096 ):
    try:
        while data[0] < (n):
            tmp = thisep.read(size=readsize,timeout=4000)
            data[0] += len(tmp)
            data[1] += 1
    except:
        print "Continuing"
    end_event.set()
#
# @param n number of samples to capture
#
def bulk_read2(dev,num_bytes=256,readsize=4096):
    data =[]
    config = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x05,0xf0,0x0e]
    write_config(dev, config )
    dev.ctrl_transfer( 0x40, 0x21, 0x7400,0x0000,data)
    dev.ctrl_transfer( 0x40, 0x21, 0xb600,0x0000,data)
    write_config(dev, config )

    numHigh = ((num_bytes) >> 17) & 0xffff
    numLow = ((num_bytes) >> 1 ) & 0xffff
    # print "High: %x" % ( numHigh )
    # print "Low: %x" % ( numLow )
    thisep = get_read_endpoint(dev)
    total = 0
    counter = 0
    tmpdata = [0,0]
    event = Event()
    t = Thread(target=read_data, args=(thisep, num_bytes , event, tmpdata,readsize))
    t.start()
    # print "sending ctrl"

    data = [0x05,0x00,0x00,0x00 ]
    dev.ctrl_transfer( 0x40, 0xbc, numHigh, numLow, data )
    data = []
    # print "Send start"
    dev.ctrl_transfer( 0x40, 0x23, 0x7400,0x64,data)
    dev.ctrl_transfer( 0x40, 0x23, 0xb600,0x64,data)

    event.wait()
    t.join()
    dev.ctrl_transfer( 0x40, 0x21, 0x7400,0x0000,data)
    dev.ctrl_transfer( 0x40, 0x21, 0xb600,0x0000,data)
    return (tmpdata[0],tmpdata[1])

  

def write_cal_table(dev):
    thisep = get_write_endpoint(dev)
    start = 0x00
    data = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x05,0xf0,0x0e]
    # write_config( dev, data )
    dev.ctrl_transfer( 0x40, 0xbe, 0x00,0x14,data)
    dev.ctrl_transfer( 0xc0, 0xba, 0x00,0x00,1 )

    config = read_config(dev)

    config[0x10] = 05
    config[0x11] = 0 
    config[0x12] = 0
    config[0x13] = 0

    write_config(dev,config)
    # Write config
    
    for n in range(0x00,64):
        data = b''.join([struct.pack("<H",x) for x in range(n*(1024),(n+1)*1024)])
        thisep.write(data,1000)
        [wValue] = struct.unpack(">H",struct.pack(">H", n*(1024)))
        send_bb(dev, wValue)

    
def end_setup_cal_table(dev):
    # nconfig = read_config(dev)
    nconfig = get_caltable_config()
    # Key thing are these settings
    nconfig[0x11] = 0x05
    nconfig[0x12] = 0xf0
    nconfig[0x13] = 0x0e
    write_config(dev,nconfig)
    read_config(dev)
    write_config(dev,nconfig)

    # # data = [5,0,0,0]
    # # thisep = get_read_endpoint(dev)
    # # dev.ctrl_transfer( 0x40,0xbc,0,0xf0,data )
    # # dev.ctrl_transfer( 0x40,0xbf,0,0,[] )

    # nddef old_end_setup_cal_table(dev):
    # # nconfig = read_config(dev)
    # nconfig = get_caltable_config()
    # # Key thing are these settings
    # nconfig[0x11] = 0x04
    # nconfig[0x12] = 0xf0
    # nconfig[0x13] = 0x0e
    # write_config(dev,nconfig)
    # data = [5,0,0,0]
    # thisep = get_read_endpoint(dev)
    # dev.ctrl_transfer( 0x40,0xbc,0,0xf0,data )
    # dev.ctrl_transfer( 0x40,0xbf,0,0,[] )

    # First read config
    # data = thisep.read(512*4)
    # return data

def get_first_config():
    config = [ 0 for x in  range(0,20)]
    config[17] = 0x04
    config[18] = 0xF0
    config[19] = 0x0a
    return config

# nconfig = get_caltable_config()
# nconfig[0x11] = 0x05
# nconfig[0x12] = 0xf0
def new_get_scan( dev ):
    # nconfig[0x13] = 0x0e
    nconfig = read_config( dev )
    nconfig[0x11] = 0x04
    nconfig[0x12] = 0xf0
    nconfig[0x13] = 0x0e
    write_config(dev,nconfig)
    thisep = get_read_endpoint(dev)
    tmpdata = [5,0,0,0]
    dev.ctrl_transfer( 0x40,0xbc,0,0xf0,tmpdata )
    dev.ctrl_transfer( 0x40,0xbf,0,0,[] )
    data = thisep.read(512)
    dev.ctrl_transfer( 0xc0, 0xd2, 0x0,0x0, 0x14 ) 
    return data

#
# Trying to write a get_scan utility
#
def get_scan(dev,data,n=100):
    thisep = get_read_endpoint(dev)
    tmpdata = [5,0,0,0]
    dev.ctrl_transfer( 0x40,0xbc,0,0xf0,tmpdata )
    dev.ctrl_transfer( 0x40,0xbf,0,0,[] )
    for i in range(0,n):
        config = get_first_config()
        write_config(dev,config)
        bRequest = 0xbc
        wValue = 0x0
        wIndex = 0xb0
        dev.ctrl_transfer(64, bRequest, wValue, wIndex , [] )
        bRequest = 0xbf # AUR_ADC_IMMEDIATE,
        wValue = 0x00
        wIndex = 0x00
        dev.ctrl_transfer(0xc0 ,0xbf , 0 , 0 , 2 )
        data.append(thisep.read(4*512))

def reinit(dev):
    config = get_first_config()
    write_config(dev,config)
    bRequest = 0xbc
    wValue = 0x0
    wIndex = 0xb0
    dev.ctrl_transfer(64, bRequest, wValue, wIndex , [] )
    bRequest = 0xbf # AUR_ADC_IMMEDIATE,
    wValue = 0x00
    wIndex = 0x00
    dev.ctrl_transfer(0xc0 ,0xbf , 0 , 0 , 2 )

def continuous_test( size=4096, clock=1000,readsize=512):
    dev = find_device()
    thisep = reload_device(dev)
    count = 0
    continuous_setup(clock)
    while count < size:
        tmp = thisep.read(readsize)
        count += len(tmp)
        #print tmp
        print count

    continuous_end()

def continuous_test_print( size=4096, clock=1000,readsize=512):
    dev = find_device()
    thisep = reload_device(dev)
    count = 0
    continuous_setup(clock)
    while count < size:
        tmp = thisep.read(readsize)
        count += len(tmp)
        print tmp
    continuous_end()

def end_transaction():
    print("Ending transaction")
    data = [0x02,0x00,0x02,0x00]
    sendPacket( "\x40\xBC\x00\x00\x00\x00\x04\x00" , data )
    sendPacket( "\xC0\xBC\x00\x00\x00\x00\x04\x00" , [0 for x in range(0,4)], True )
    sendPacket( "\x40\x21\x00\x74\x00\x00\x00\x00" )
    sendPacket( "\x40\x35\x00\x00\x00\x00\x00\x00" )



def calculate_clocks(hz):
    ROOTCLOCK = 10000000;
    if hz == 0 :
        return -1

    if hz * 4 >= ROOTCLOCK:
        divisora = 2
        divisorb = 2
    else:
        divisorab = ROOTCLOCK / hz
        l = int(math.sqrt( divisorab ))
        if  l > 0xffff:
            divisora = 0xffff
            divisorb = 0xffff
            min_err  = abs(round(((ROOTCLOCK / hz) - (divisora * l))));
        else:
            divisora  = round( divisorab / l );
            l         = round( math.sqrt( divisorab ));
            divisorb  = l;

        min_err = abs(((ROOTCLOCK / hz) - (divisora * l)));
      
        # for( unsigned lv = l ; lv >= 2 ; lv -- ) {
        for lv in range( int(l) ,2 , -1 ):
            olddivisora = round(divisorab / lv);
            if olddivisora > 0xffff:
                print "Found value > 0xff..resetting"
                break;
            else:
                divisora = olddivisora
                
            err = abs((ROOTCLOCK / hz) - (divisora * lv));
            if err <= 0:
                min_err = 0;
                print "Found zero error: %d\n" % ( lv )
                divisorb = lv;
                break;

            if err < min_err:
                print( "Found new error: using lv=%d\n" % lv)
                divisorb = lv;
                min_err = err;
        
            divisora = round(divisorab / divisorb);
        return (divisora,divisorb)


def reset_device():
    global dev
    dev = find_device()
    print "Resetting the board "
    dev.ctrl_transfer( 0x40, 0xA0, 0xE600, 0 , [0x01] )
    dev.ctrl_transfer( 0x40, 0xA0, 0xE600, 0 , [0x00] )
    dev = find_device()
    # print "Got value %s" % (dev )
    init_device(dev)


def continuous_read_data(count = 2048):
    global DELAY
    actualcount = 0
    error = 0
    while actualcount  < count:
        try:
            tmp = thisep.read(512, DELAY)
            print "Received %s " % tmp
            actualcount += len(tmp)
            error = 0
        except:
            if error > 20:
                print "Timeout issue"
                break
            print "Trying again" 
            error += 1


def sendPacket(packet,data = [], indata = False ):
    global dev
    [bmRequestType, bRequest, wValue, wIndex, wLength ] = struct.unpack("<BBHHH", packet[0:8] )
    if wLength == 0:
        if data != []:
            data = []
        if indata:
            data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , wLength )  
        else:
            data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , data )  
    else:
        if indata:
            data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , wLength )  
        else:
            if data == []:
                data =  [0 for x in range(0,wLength) ]
            data = dev.ctrl_transfer( bmRequestType , bRequest, wValue, wIndex , data )  

    return data


class Packet():
    def __init__(self,a, b):
        self.a = a
        self.b = b
        print "a: %s , b: %s\n" % ( a, b )

        
    def __str__(self):
        return "a: %s , b: %s\n" % ( self.a, self.b )





