/**
 * @file   Sample.java
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief  
 * @note to compile:
 *   make Sample.java
 */

import AIOUSB.*;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;

class Sample {
    public static final String VERSION = "$Format: %t$";
    /*
     * if you want to test writing to the EEPROM, set DEMO_EEPROM_WRITE to 'true'; after writing
     * to the EEPROM this sample program will attempt to restore the original EEPROM contents,
     * but you never know ...
     */
    static class Device {
        public int deviceMask;
        public SWIGTYPE_p_unsigned_long productID  ;
        public SWIGTYPE_p_unsigned_long nameSize   ;
        public SWIGTYPE_p_unsigned_long numDIOBytes;
        public SWIGTYPE_p_unsigned_long numCounters;            
        public AIOChannelMask outputMask;
        int MAX_DIO_BYTES = 32;
        public DIOBuf readBuffer;
        public DIOBuf writeBuffer;
        public long index;
        public Device( long nindex, 
                       SWIGTYPE_p_unsigned_long nproductID,  
                       SWIGTYPE_p_unsigned_long nnameSize,  
                       SWIGTYPE_p_unsigned_long nnumDIOBytes, 
                       SWIGTYPE_p_unsigned_long nnumCounters
                       ) { 
            index = nindex;
            // System.out.println("Used device " + index );
            productID = nproductID;
            nameSize = nnameSize;
            numDIOBytes = nnumDIOBytes;
            numCounters = nnumCounters;
            outputMask = new AIOChannelMask(0);
            readBuffer = new DIOBuf(MAX_DIO_BYTES );
            writeBuffer = new DIOBuf( MAX_DIO_BYTES );
        }
    }

    protected static final boolean DEMO_EEPROM_WRITE = false;
    
    public static void main( String args[] ) {

        System.out.println(
                           "USB-DIO-32 sample program version: " + VERSION + "\n" + 
                           "  AIOUSB Java library version: " + 
                           "  AIOUSB library version: " + 
                           AIOUSB.AIOUSB_GetVersion() + ", " + 
                           AIOUSB.AIOUSB_GetVersionDate() + "\n" + 
                           "  JRE version: " + System.getProperty( "java.version" ) + "\n" +
                           "  OS version: " + System.getProperty( "os.name" ) + 
                           " " + System.getProperty( "os.arch" ) + 
                           " " + System.getProperty( "os.version" ) + "\n" + 
                           "  This program demonstrates controlling a USB-DIO-32 device on\n" + 
                           "  the USB bus. For simplicity, it uses the first such device found\n" + 
                           "  on the bus."
                           );
        
        AIOUSB.AIOUSB_Init();
        AIOUSB.AIOUSB_ListDevices();
        int number_devices = 1;
        long deviceMask = AIOUSB.GetDevices();
        SWIGTYPE_p_unsigned_long productID    = AIOUSB.new_ulp();
        SWIGTYPE_p_unsigned_long nameSize     = AIOUSB.new_ulp();
        SWIGTYPE_p_unsigned_long numDIOBytes  = AIOUSB.new_ulp();
        SWIGTYPE_p_unsigned_long numCounters  = AIOUSB.new_ulp();
        String name = "";
        long index = 0;
        ArrayList<Device> devices = new ArrayList<Device>();
        while( deviceMask > 0 && (devices.size() < number_devices  ) ) {
            if ( (deviceMask & 1 ) != 0 ) { 
                AIOUSB.QueryDeviceInfo(index , productID, nameSize, name, numDIOBytes, numCounters);
                // This is a hack, only because Java has some issue
                // retrieving the value of ProductIDS.USB_DIO_32
                if( AIOUSB.ulp_value(productID) == ProductIDS.USB_DIO_32.swigValue() ) { 
                    System.out.println("Foud device at index:" + index );
                    devices.add( new Device( index, productID, nameSize, numDIOBytes, numCounters ) );
                }
                index += 1;
                deviceMask >>= 1;
            }        
        }
        if( devices.size() != number_devices ) {
            System.out.println("Error; not enough devices found=" + number_devices + "\n" );
            System.out.println("Error; not enough devices found=" + devices.size() + "\n" );
            System.exit(1);
        } else {
            System.out.println(devices.size() + " devices were found\n");
        }

        Device device = devices.get(0);
        // System.out.println("Using device index:" + device.index  );
        // System.exit(0);
        AIOUSB.AIOUSB_SetCommTimeout( device.index, 1000 );
        AIOUSB.AIOChannelMaskSetMaskFromStr( device.outputMask, "1111" );
        System.out.println("Mask is " + device.outputMask.toFoo() );
        
            // for port in range(0x20):
        System.out.println("Using device index:" + device.index  );
        for( int port = 0; port < 0x20; port ++ ) { 
            System.out.println( "Using value " + port );
            AIOUSB.DIOBufSetIndex( device.writeBuffer, port, 1 );
            long result = AIOUSB.DIO_Configure( device.index, (short)1 , device.outputMask , device.writeBuffer );
            try {
                TimeUnit.MILLISECONDS.sleep(100);// Thread.sleep(0,10000);
            } catch ( java.lang.InterruptedException e ) {

            }
        }
        //     System.out.println( DIOBufToString( device.writeBuffer )
        //         result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer )
        //         #System.out.println( "Result was %d" % (result)
        //         time.sleep(1/10.0)
        //         }
        //     # ONLY Port D
        //     AIOChannelMaskSetMaskFromStr(device.outputMask, "1000" );
        // result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
        // time.sleep(1/10.0)
        //     # ONLY Port C
        //     AIOChannelMaskSetMaskFromStr(device.outputMask, "0100" );
        // result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
        // time.sleep(1/10.0);
        // # ONLY Port B
        //     AIOChannelMaskSetMaskFromStr(device.outputMask, "0010" );
        // result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
        // time.sleep(1/10.0);
        // # ONLY Port A
        //     AIOChannelMaskSetMaskFromStr(device.outputMask, "0001" );
        // result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );
        // time.sleep(1/10.0);
        // # All Channels
        //     AIOChannelMaskSetMaskFromStr(device.outputMask, "1111" );
        // result = DIO_Configure( device.index, AIOUSB_FALSE, device.outputMask , device.writeBuffer );


    }
}

