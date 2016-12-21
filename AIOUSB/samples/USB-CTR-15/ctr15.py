#!/usr/bin/python


from __future__ import print_function
from AIOUSB import *
from string import split

import math
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def print_usage(args):
    print("%s :  -c CHANNEL_NUMBER=CLOCK_HZ [ -c CHANNEL_NUMBER=CLOCK_HZ ...]\n", args[0] );


def frequency_to_counter( frequency ):
    return int(math.sqrt( 10000000 / frequency ))

def find_idio(obj):
    if obj.PID == USB_CTR_15:
        return True



def main(args):
    channel_frequencies = [0.0,0.0,0.0,0.0,0.0]

    AIOUSB_ListDevices();
    indices = AIOUSB_FindDevices( find_idio )
    num_devices = 1;
    if len(indices) <= 0:
        eprint("Unable to find a USB-CTR-16 device..exiting\n")
        sys.exit(1)
    else:
        print("Using device at index %d\n" % ( indices[0] ))

        
    # Adjust the parmeters
    for i in args["--channel"]:
        [channel,hz] = split(i,"=")
        channel = int(channel)
        hz = float(hz)

        if channel < 0 or channel > 5:
           eprint("Error: only Channels between 0 and 4" )
           sys.exit(1)
        channel_frequencies[channel] = float(hz)



    for i,val in enumerate(channel_frequencies):
        print("Channel %i is set to %.2f" % (i, channel_frequencies[i] ))

        
    for i in range(0,5):
        if channel_frequencies[i] == 0:
            counter_value = 62071
        else:
            counter_value = frequency_to_counter( channel_frequencies[i] )

        print("Setting channel %d counter to %d" % (i , counter_value ))
        CTR_8254ModeLoad( indices[0], i, 1, 2, counter_value )
        CTR_8254ModeLoad( indices[0], i, 2, 3, counter_value )

              

if __name__ == "__main__":
    
    usage = """
Usage: counted_example.py --help
       counted_example.py (--channel=<path>)...

Options:
    -h, --help  Show the help screen

Try: 
     counted_example.py --channel ./here --channel ./there
     counted_example.py this.txt that.txt
    """
    from docopt import *
    arguments = docopt(usage,  )
    main(arguments)


# main( args )
#     ctr15.py --channel 0=1000 --channel 1=10000
#    ctr15.py (--channel=<CHANNELNUM=HZ>)... 
