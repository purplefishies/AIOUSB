/*
 * $RCSfile: Sample.java,v $
 * $Date: 2009/12/25 18:30:47 $
 * $Revision: 1.3 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 */

// {{{ build instructions
/*
 * to compile:
 *   javac -cp ../../java/aiousb.jar Sample.java
 * to run:
 *   java -cp ../../java/aiousb.jar:. Sample
 */
// }}}

// {{{ imports
import com.acces.aiousb.*;
import java.io.*;
import java.util.*;
// }}}

class Sample {
	public static final String VERSION = "1.3, 25 December 2009";
	/*
	 * if you want to test writing to the EEPROM, set DEMO_EEPROM_WRITE to 'true'; after writing
	 * to the EEPROM this sample program will attempt to restore the original EEPROM contents,
	 * but you never know ...
	 */
	protected static final boolean DEMO_EEPROM_WRITE = false;

	public static void main( String args[] ) {
		USBDeviceManager deviceManager = null;
		try {
/*API*/		deviceManager = new USBDeviceManager();
/*API*/		deviceManager.open();
			System.out.println(
				  "USB-DIO-32 sample program version: " + VERSION + "\n"
/*API*/			+ "  AIOUSB Java library version: " + deviceManager.VERSION_NUMBER + ", " + deviceManager.VERSION_DATE + "\n"
/*API*/			+ "  AIOUSB library version: " + deviceManager.getAIOUSBVersion() + ", " + deviceManager.getAIOUSBVersionDate() + "\n"
				+ "  JRE version: " + System.getProperty( "java.version" ) + "\n"
				+ "  OS version: " + System.getProperty( "os.name" )
					+ " " + System.getProperty( "os.arch" )
					+ " " + System.getProperty( "os.version" ) + "\n"
				+ "  This program demonstrates controlling a USB-DIO-32 device on\n"
				+ "  the USB bus. For simplicity, it uses the first such device found\n"
				+ "  on the bus."
			);
/*API*/		deviceManager.printDevices();
/*API*/		USBDevice[] devices = deviceManager.getDeviceByProductID( USBDeviceManager.USB_DIO_32 );
			if( devices.length > 0 ) {
				USB_DIO_32_Family device = ( USB_DIO_32_Family ) devices[ 0 ];	// get first device found
/*API*/			device.reset()
/*API*/				.setCommTimeout( 1000 );

				/*
				 * demonstrate writing EEPROM
				 */
				if( DEMO_EEPROM_WRITE ) {
					System.out.print( "Writing to EEPROM ... " );
					final int offset = 0x100;
					String message = "USB-DIO-32 sample program";
/*API*/				byte[] prevData = device.customEEPROMRead( offset, message.length() );	// preserve current EEPROM contents
/*API*/				device.setCommTimeout( 10000 )	// writing the entire EEPROM (which we're not doing here) can take a long time
/*API*/					.customEEPROMWrite( offset, message.getBytes() );
/*API*/				String readMessage = new String( device.customEEPROMRead( offset, message.length() ) );
/*API*/				device.customEEPROMWrite( offset, prevData )
/*API*/					.setCommTimeout( 1000 );
					System.out.println(
						readMessage.equals( message )
							? "successful"
							: "failed: data read back did not compare" );
				}	// if( DEMO_EEPROM_WRITE )

				/*
				 * demonstrate reading EEPROM
				 */
				System.out.println( "EEPROM contents:\n"
/*API*/				+ Arrays.toString( device.customEEPROMRead( 0, device.CUSTOM_EEPROM_SIZE ) ) );

				/*
				 * demonstrate DIO configuration
				 */
				System.out.print( "Configuring digital I/O ... " );
/*API*/			boolean[] outputs = new boolean[ device.dio().getNumPorts() ];
				Arrays.fill( outputs, true );	// all ports are outputs
/*API*/			boolean[] values = new boolean[ device.dio().getNumChannels() ];
				Arrays.fill( values, false );	// turn all outputs off
/*API*/			device.dio().configure( false, outputs, values );
				System.out.println( "successful" );

				/*
				 * demonstrate writing outputs individually
				 */
				System.out.print( "Turning all outputs on:" );
/*API*/			for( int channel = 0; channel < device.dio().getNumChannels(); channel++ ) {
					System.out.print( " " + channel );
/*API*/				device.dio().write( channel, true );
					Thread.sleep( 100 );
				}	// for( int channel ...
				System.out.print( "\nTurning all outputs off:" );
/*API*/			for( int channel = 0; channel < device.dio().getNumChannels(); channel++ ) {
					System.out.print( " " + channel );
/*API*/				device.dio().write( channel, false );
					Thread.sleep( 100 );
				}	// for( int channel ...
				System.out.println();

				/*
				 * demonstrate writing outputs as a group
				 */
				System.out.print( "Turning all outputs on ... " );
				Arrays.fill( values, true );
/*API*/			device.dio().write( 0, values );
				System.out.println( "successful" );
				Thread.sleep( 2000 );
				System.out.print( "Turning all outputs off ... " );
				Arrays.fill( values, false );
/*API*/			device.dio().write( 0, values );
				System.out.println( "successful" );
			} else
				System.out.println( "No USB-DIO-32 devices found on USB bus" );
/*API*/		deviceManager.close();
		} catch( Exception ex ) {
			System.err.println( "Error \'" + ex.toString() + "\' occurred while manipulating device" );
			if(
				deviceManager != null
/*API*/			&& deviceManager.isOpen()
			)
/*API*/			deviceManager.close();
		}	// catch( ...
	}	// main()

}	// class Sample

/* end of file */
