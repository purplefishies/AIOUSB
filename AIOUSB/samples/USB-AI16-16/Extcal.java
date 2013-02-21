/*
 * $RCSfile: Extcal.java,v $
 * $Revision: 1.11 $
 * $Date: 2009/12/25 18:44:28 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * program to externally calibrate USB-AI16-16
 */

// {{{ build instructions
/*
 * to compile:
 *   javac -cp ../../java/aiousb.jar Extcal.java
 * to run:
 *   java -cp ../../java/aiousb.jar:. Extcal
 */
// }}}

// {{{ imports
import com.acces.aiousb.*;
import java.io.*;
import java.lang.*;
import java.util.*;
// }}}

class Extcal {
	public static final String VERSION = "1.11, 25 December 2009";
	protected static final boolean SIMULATE_AD = false;
	protected static final int CAL_CHANNEL = 0;

	public static void main( String args[] ) {
		USBDeviceManager deviceManager = null;
		try {
/*API*/		deviceManager = new USBDeviceManager();
/*API*/		deviceManager.open();
			System.out.println(
				  "USB-AI16-16A sample program version: " + VERSION + "\n"
/*API*/			+ "  AIOUSB Java library version: " + deviceManager.VERSION_NUMBER + ", " + deviceManager.VERSION_DATE + "\n"
/*API*/			+ "  AIOUSB library version: " + deviceManager.getAIOUSBVersion() + ", " + deviceManager.getAIOUSBVersionDate() + "\n"
				+ "  JRE version: " + System.getProperty( "java.version" ) + "\n"
				+ "  OS version: " + System.getProperty( "os.name" )
					+ " " + System.getProperty( "os.arch" )
					+ " " + System.getProperty( "os.version" ) + "\n"
				+ "\n"
				+ "  This program demonstrates external calibration of a USB-AI16-16 device\n"
				+ "  on the USB bus. For simplicity, it uses the first such device found on\n"
				+ "  the bus.\n"
				+ "\n"
				+ "  This external calibration procedure allows you to inject a sequence of\n"
				+ "  precise voltages into the USB-AI16-16 that will be used to calibrate its\n"
				+ "  A/D. This procedure calibrates the A/D using channel " + CAL_CHANNEL + " on the 0-10V range.\n"
				+ "  A minimum of 2 calibration points are required. The procedure is as follows:\n"
				+ "\n"
				+ "  1) Adjust a precision voltage source to a desired target voltage. These\n"
				+ "     target voltages do not have to be evenly spaced or provided in any\n"
				+ "     particular order.\n"
				+ "\n"
				+ "  2) When you have adjusted the precision voltage source to your desired\n"
				+ "     target, type in the exact voltage being fed into the USB-AI16-16 and\n"
				+ "     press <Enter>. This program will read the A/D and display the reading,\n"
				+ "     asking you to accept it or not. If it looks acceptable, press 'y'.\n"
				+ "     Otherwise, press any other key and the program will retake the reading.\n"
				+ "\n"
				+ "  3) When you are finished calibrating, press <Enter> without entering a\n"
				+ "     voltage, and the A/D will be calibrated using the data entered, and\n"
				+ "     the calibration table will be saved to a file in a format that can be\n"
				+ "     subsequently loaded into the A/D.\n"
			);
/*API*/		deviceManager.printDevices();
/*API*/		USBDevice[] devices = deviceManager.getDeviceByProductID( USB_AI16_Family.getSupportedProductIDs() );
			if( devices.length > 0 ) {
				USB_AI16_Family device = ( USB_AI16_Family ) devices[ 0 ];	// get first device found

				/*
				 * set up A/D in proper mode for calibrating, including a default calibration table
				 */
				System.out.print( "Calibrating A/D, may take a few seconds ... " );
/*API*/			device.reset()
/*API*/				.setCommTimeout( 1000 );
/*API*/			device.adc()
/*API*/				.setRangeAndDiffMode( AnalogInputSubsystem.RANGE_0_10V, false )
/*API*/				.setCalMode( AnalogInputSubsystem.CAL_MODE_NORMAL )
/*API*/				.setTriggerMode( AnalogInputSubsystem.TRIG_MODE_SCAN )
/*API*/				.setOverSample( 100 )
/*API*/				.setDiscardFirstSample( true )
/*API*/				.calibrate( false, false, null );
				System.out.println( "successful" );
				BufferedReader input = new BufferedReader( new InputStreamReader( System.in ) );
				final int MAX_POINTS = 100;
				double[] points = new double[ MAX_POINTS ];		// voltage-count pairs
				int numPoints = 0;
				while( numPoints < MAX_POINTS ) {
					String commandLine;
					System.out.print(
						  "Measuring calibration point " + ( numPoints + 1 ) + ":\n"
						+ "  Feed a voltage into channel " + CAL_CHANNEL + " and enter voltage here\n"
						+ "  (enter nothing to finish and calibrate A/D): "
					);
					commandLine = input.readLine();
					if( commandLine.isEmpty() )
						break;					// from while()
					try {
						points[ numPoints * 2 ] = Double.parseDouble( commandLine );
					} catch( NumberFormatException ex ) {
						System.err.println( "Error: \'" + commandLine + "\' is not a valid voltage" );
						continue;
					}	// catch( ...
					if( SIMULATE_AD ) {
						System.out.print( "  Enter A/D counts: " );
						commandLine = input.readLine();
						if( commandLine.isEmpty() )
							break;				// from while()
						try {
							points[ numPoints++ * 2 + 1 ] = Integer.parseInt( commandLine );
						} catch( NumberFormatException ex ) {
							System.err.println( "Error: \'" + commandLine + "\' is not a valid count value" );
							continue;
						}	// catch( ...
					} else {
						int counts;
						try {
/*API*/						counts = device.adc().readCounts( CAL_CHANNEL );
						} catch( Exception ex ) {
							System.err.println( "Error: \'" + ex.toString() + "\' occurred while reading A/D input" );
							continue;
						}	// catch( ...
						System.out.print( "  Read " + counts + " A/D counts ("
/*API*/						+ device.adc().countsToVolts( CAL_CHANNEL, ( char ) counts )
							+ " volts), accept (y/n)? " );
						commandLine = input.readLine();
						if( commandLine.compareToIgnoreCase( "y" ) == 0 )
							points[ numPoints++ * 2 + 1 ] = counts;
					}	// if( SIMULATE_AD )
				}	// while( true )
				if( numPoints >= 2 ) {
					try {
/*API*/					String fileName = "ADC-Ext-Cal-Table-" + Long.toHexString( device.getSerialNumber() );
/*API*/					device.adc().calibrate( Arrays.copyOf( points, numPoints * 2 ), false, fileName );
						System.out.println( "External calibration of A/D successful, table saved in " + fileName );
					} catch( Exception ex ) {
						System.err.println(
							  "Error \'" + ex.toString() + "\' occurred while externally calibrating A/D."
							+ " This usually occurs because the input voltages or measured counts are not unique and ascending."
						);
					}	// catch( ...
				} else
					System.err.println( "Error: you must provide at least two points" );
			} else
				System.out.println( "No USB-AI16-16 devices found on USB bus" );
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
}	// class Extcal

/* end of file */
