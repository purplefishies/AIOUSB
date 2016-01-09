/**
 * @file   extcal.cpp
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @author Jimi Damon <jdamon@accesio.com>
 * @version $Format: %h$
 * 
 * @page sample_usb_ai16_16_extcal extcal.c
 *
 * @par Extcal
 * Extcal.cpp is simple program that demonstrates using
 * the AIOUSB C library and C++ Classlib to perform an
 * external calibration of an ACCES I/O model USB-AI16-16A analog
 * input board. The program is not intended to be a comprehensive
 * demonstration and is limited to demonstrating the following
 * features of the AIOUSB API:
 * 
 * - Initializing and shutting down the API – USBDeviceManager::open(), USBDeviceManager::close()
 * - Finding devices on the USB bus – USBDeviceManager::getDeviceByProductID()
 * - Configuring the board – USBDevice::setCommTimeout(), AnalogInputSubsystem::setCalMode(), AnalogInputSubsystem::setDiscardFirstSample(), AnalogInputSubsystem::setTriggerMode(), AnalogInputSubsystem::setGainCodeAndDiffMode(), AnalogInputSubsystem::setOversample()
 * - Installing a default calibration table – AnalogInputSubsystem::calibrate(bool,...)
 * - Reading the analog inputs in counts – AnalogInputSubsystem::read()
 * - Generating an external calibration table – AnalogInputSubsystem::calibrate(double[],...)
 *
 * @todo Setup BUILDING Tag
 * 
 */

/*
 * g++ extcal.cpp -lclassaiousb -laiousbcpp -lusb-1.0 -o extcal
 *   or
 * g++ -ggdb extcal.cpp -lclassaiousbdbg -laiousbcppdbg -lusb-1.0 -o extcal
 */

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


using namespace AIOUSB;
using namespace std;

int main( int argc, char *argv[] ) {
       	USBDeviceManager deviceManager;
	try {
		const int CAL_CHANNEL = 0;
       	deviceManager.open();
		cout <<
			"USB-AI16-16 sample program version 1.18, 25 December 2009\n"
       		"  AIOUSB C++ class library version " << deviceManager.VERSION_NUMBER << ", " << deviceManager.VERSION_DATE << "\n"
       		"  AIOUSB library version " << deviceManager.getAIOUSBVersion() << ", " << deviceManager.getAIOUSBVersionDate() << "\n"
			"\n"
			"  This program demonstrates external calibration of a USB-AI16-16 device\n"
			"  on the USB bus. For simplicity, it uses the first such device found on\n"
			"  the bus and supports these product IDs: ";
       	const StringArray supportedProductNames = USB_AI16_Family::getSupportedProductNames();
		for( int index = 0; index < ( int ) supportedProductNames.size(); index++ ) {
			if( index > 0 )
				cout << ", ";
			cout << supportedProductNames.at( index );
		}
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
       	deviceManager.printDevices();
       	USBDeviceArray devices = deviceManager.getDeviceByProductID( USB_AI16_Family::getSupportedProductIDs() );
		if( devices.size() > 0 ) {
			USB_AI16_Family &device = *( USB_AI16_Family * ) devices.at( 0 );	// get first device found
			try {
				/*
				 * set up A/D in proper mode for
				 * calibrating, including a default
				 * calibration table
				 */
				cout << "Calibrating A/D, may take a few seconds ... " << flush;
       			device.reset()
       				.setCommTimeout( 1000 );
       			device.adc()
       				.setCalMode( AnalogInputSubsystem::CAL_MODE_NORMAL )
       				.setDiscardFirstSample( true )
       				.setTriggerMode( AnalogInputSubsystem::TRIG_MODE_SCAN )
       				.setRangeAndDiffMode( AnalogInputSubsystem::RANGE_0_10V, false )
       				.setOverSample( 100 )
       				.calibrate( false, false, "" );
				cout << "successful" << endl;
				DoubleArray points;
				while( true ) {
					std::string commandLine;
					unsigned short counts;
					cout
						<< "Measuring calibration point " << points.size() / 2 + 1 << ':' << endl
						<< "  Feed a voltage into channel " << CAL_CHANNEL << " and enter voltage here" << endl
						<< "  (enter nothing to finish and calibrate A/D): ";
					getline( cin, commandLine );
					if( commandLine.empty() )
						break;
					points.insert( points.end(), strtod( commandLine.c_str(), 0 ) );
#if defined( SIMULATE_AD )
					cout << "  Enter A/D counts: ";
					getline( cin, commandLine );
					if( commandLine.empty() )
						break;
					counts = strtol( commandLine.c_str(), 0, 0 );
					points.insert( points.end(), counts );
#else
					try {
       					counts = device.adc().readCounts( CAL_CHANNEL );
						cout << "  Read " << counts << " A/D counts ("
       						<< device.adc().countsToVolts( CAL_CHANNEL, counts )
							<< " volts), accept (y/n)? ";
						getline( cin, commandLine );
						if( commandLine.compare( "y" ) == 0 )
							points.insert( points.end(), counts );
					} catch( exception &ex ) {
						cerr << "Error \'" << ex.what() << "\' occurred while reading A/D input" << endl;
					}
#endif
				}
				if( points.size() >= 4 ) {
					try {
						char fileName[ 100 ];
       					sprintf( fileName, "ADC-Ext-Cal-Table-%llx", ( long long ) device.getSerialNumber() );
       					device.adc().calibrate( points, false, fileName );
						cout << "External calibration of A/D successful, table saved in " << fileName << endl;
					} catch( exception &ex ) {
						cerr << "Error \'" << ex.what() << "\' occurred while externally calibrating A/D."
							<< " This usually occurs because the input voltages or measured counts are not unique and ascending." << endl;
					}
				} else
					cerr << "Error: you must provide at least two points" << endl;
			} catch( exception &ex ) {
				cerr << "Error \'" << ex.what() << "\' occurred while configuring device" << endl;
			}
		} else
			cout << "No USB-AI16-16 devices found on USB bus" << endl;
       	deviceManager.close();
	} catch( exception &ex ) {
		cerr << "Error \'" << ex.what() << "\' occurred while initializing device manager" << endl;
       	if( deviceManager.isOpen() )
       		deviceManager.close();
	}
}


/* end of file */
