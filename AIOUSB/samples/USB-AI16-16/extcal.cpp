/*
 * $RCSfile: extcal.cpp,v $
 * $Revision: 1.18 $
 * $Date: 2009/12/25 19:17:26 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * program to externally calibrate USB-AI16-16
 */


// {{{ build instructions
/*
 * g++ extcal.cpp -lclassaiousb -laiousbcpp -lusb-1.0 -o extcal
 *   or
 * g++ -ggdb extcal.cpp -lclassaiousbdbg -laiousbcppdbg -lusb-1.0 -o extcal
 */
// }}}

// {{{ includes
#include <iostream>
#include <iterator>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aiousb.h>
#include <AIOUSB_Core.h>
#include <USBDeviceManager.hpp>
#include <USB_AI16_Family.hpp>
// }}}

using namespace AIOUSB;
using namespace std;

int main( int argc, char *argv[] ) {
/*API*/	USBDeviceManager deviceManager;
	try {
		const int CAL_CHANNEL = 0;
/*API*/	deviceManager.open();
		cout <<
			"USB-AI16-16 sample program version 1.18, 25 December 2009\n"
/*API*/		"  AIOUSB C++ class library version " << deviceManager.VERSION_NUMBER << ", " << deviceManager.VERSION_DATE << "\n"
/*API*/		"  AIOUSB library version " << deviceManager.getAIOUSBVersion() << ", " << deviceManager.getAIOUSBVersionDate() << "\n"
			"\n"
			"  This program demonstrates external calibration of a USB-AI16-16 device\n"
			"  on the USB bus. For simplicity, it uses the first such device found on\n"
			"  the bus and supports these product IDs: ";
/*API*/	const StringArray supportedProductNames = USB_AI16_Family::getSupportedProductNames();
		for( int index = 0; index < ( int ) supportedProductNames.size(); index++ ) {
			if( index > 0 )
				cout << ", ";
			cout << supportedProductNames.at( index );
		}	// for( int index ...
		cout <<
			".\n\n"
			"  This external calibration procedure allows you to inject a sequence of\n"
			"  precise voltages into the USB-AI16-16 that will be used to calibrate its\n"
			"  A/D. This procedure calibrates the A/D using channel " << CAL_CHANNEL << " on the 0-10V range.\n"
			"  A minimum of 2 calibration points are required. The procedure is as follows:\n"
			"\n"
			"  1) Adjust a precision voltage source to a desired target voltage. These\n"
			"     target voltages do not have to be evenly spaced or provided in any\n"
			"     particular order.\n"
			"\n"
			"  2) When you have adjusted the precision voltage source to your desired\n"
			"     target, type in the exact voltage being fed into the USB-AI16-16 and\n"
			"     press <Enter>. This program will read the A/D and display the reading,\n"
			"     asking you to accept it or not. If it looks acceptable, press 'y'.\n"
			"     Otherwise, press any other key and the program will retake the reading.\n"
			"\n"
			"  3) When you are finished calibrating, press <Enter> without entering a\n"
			"     voltage, and the A/D will be calibrated using the data entered, and\n"
			"     the calibration table will be saved to a file in a format that can be\n"
			"     subsequently loaded into the A/D.\n"
			"\n";
/*API*/	deviceManager.printDevices();
/*API*/	USBDeviceArray devices = deviceManager.getDeviceByProductID( USB_AI16_Family::getSupportedProductIDs() );
		if( devices.size() > 0 ) {
			USB_AI16_Family &device = *( USB_AI16_Family * ) devices.at( 0 );	// get first device found
			try {
				/*
				 * set up A/D in proper mode for calibrating, including a default calibration table
				 */
				cout << "Calibrating A/D, may take a few seconds ... " << flush;
/*API*/			device.reset()
/*API*/				.setCommTimeout( 1000 );
/*API*/			device.adc()
/*API*/				.setCalMode( AnalogInputSubsystem::CAL_MODE_NORMAL )
/*API*/				.setDiscardFirstSample( true )
/*API*/				.setTriggerMode( AnalogInputSubsystem::TRIG_MODE_SCAN )
/*API*/				.setRangeAndDiffMode( AnalogInputSubsystem::RANGE_0_10V, false )
/*API*/				.setOverSample( 100 )
/*API*/				.calibrate( false, false, "" );
				cout << "successful" << endl;
				DoubleArray points;				// voltage-count pairs
				while( true ) {
					std::string commandLine;
					unsigned short counts;
					cout
						<< "Measuring calibration point " << points.size() / 2 + 1 << ':' << endl
						<< "  Feed a voltage into channel " << CAL_CHANNEL << " and enter voltage here" << endl
						<< "  (enter nothing to finish and calibrate A/D): ";
					getline( cin, commandLine );
					if( commandLine.empty() )
						break;					// from while()
					points.insert( points.end(), strtod( commandLine.c_str(), 0 ) );
#if defined( SIMULATE_AD )
					cout << "  Enter A/D counts: ";
					getline( cin, commandLine );
					if( commandLine.empty() )
						break;					// from while()
					counts = strtol( commandLine.c_str(), 0, 0 );
					points.insert( points.end(), counts );
#else
					try {
/*API*/					counts = device.adc().readCounts( CAL_CHANNEL );
						cout << "  Read " << counts << " A/D counts ("
/*API*/						<< device.adc().countsToVolts( CAL_CHANNEL, counts )
							<< " volts), accept (y/n)? ";
						getline( cin, commandLine );
						if( commandLine.compare( "y" ) == 0 )
							points.insert( points.end(), counts );
					} catch( exception &ex ) {
						cerr << "Error \'" << ex.what() << "\' occurred while reading A/D input" << endl;
					}	// catch( ...
#endif
				}	// while( true )
				if( points.size() >= 4 ) {
					try {
						char fileName[ 100 ];
/*API*/					sprintf( fileName, "ADC-Ext-Cal-Table-%llx", ( long long ) device.getSerialNumber() );
/*API*/					device.adc().calibrate( points, false, fileName );
						cout << "External calibration of A/D successful, table saved in " << fileName << endl;
					} catch( exception &ex ) {
						cerr << "Error \'" << ex.what() << "\' occurred while externally calibrating A/D."
							<< " This usually occurs because the input voltages or measured counts are not unique and ascending." << endl;
					}	// catch( ...
				} else
					cerr << "Error: you must provide at least two points" << endl;
			} catch( exception &ex ) {
				cerr << "Error \'" << ex.what() << "\' occurred while configuring device" << endl;
			}	// catch( ...
		} else
			cout << "No USB-AI16-16 devices found on USB bus" << endl;
/*API*/	deviceManager.close();
	} catch( exception &ex ) {
		cerr << "Error \'" << ex.what() << "\' occurred while initializing device manager" << endl;
/*API*/	if( deviceManager.isOpen() )
/*API*/		deviceManager.close();
	}	// catch( ...
}	// main()


/* end of file */
