/**
 * @file   AIOUSB_Core.h
 * @author $Format: %an <%ae>$
 * @date   $Format: %ad$
 * @release $Format: %t$
 * @brief 
 *
 * @note build instructions
 * @verbatim
 * g++ sample.cpp -lclassaiousb -laiousbcpp -lusb-1.0 -o sample
 *   or
 * g++ -ggdb sample.cpp -lclassaiousbdbg -laiousbcppdbg -lusb-1.0 -o sample
 * @endverbatim
 */

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <USBDeviceManager.hpp>
#include <USB_DIO_Family.hpp>


using namespace AIOUSB;
using namespace std;

int main( int argc, char *argv[] ) {
	const string VERSION = "1.6, 25 December 2013";
	/*
	 * if you want to test writing to the EEPROM, set DEMO_EEPROM_WRITE to 'true'; after writing
	 * to the EEPROM this sample program will attempt to restore the original EEPROM contents,
	 * but you never know ...
	 */
	const bool DEMO_EEPROM_WRITE = false;

       	USBDeviceManager deviceManager;
	try {
       	deviceManager.open();
		cout
			<< "USB-IDIO-8 sample program version: " << VERSION << "\n"
       		<< "  AIOUSB C++ library version: " << deviceManager.VERSION_NUMBER << ", " << deviceManager.VERSION_DATE << "\n"
       		<< "  AIOUSB library version: " << deviceManager.getAIOUSBVersion() << ", " << deviceManager.getAIOUSBVersionDate() << "\n"
			<< "  This program demonstrates controlling a USB-IDIO-8 device on\n"
			<< "  the USB bus. For simplicity, it uses the first such device found\n"
			<< "  on the bus.\n";
       	deviceManager.printDevices();
       	USBDeviceArray devices = deviceManager.getDeviceByProductID( USB_IDIO_8 );
		if( devices.size() > 0 ) {
			USB_DIO_Family &device = *( USB_DIO_Family * ) devices.at( 0 );	// get first device found
       		device.reset()
       			.setCommTimeout( 1000 );

			/*
			 * demonstrate DIO configuration
			 */
			cout << "Configuring digital I/O ... " << flush;
       		const int numPorts = device.dio().getNumPorts();
			const int numOutputPorts = numPorts / 2;
       		const int numChannels = device.dio().getNumChannels();
			const int numOutputChannels = numChannels / 2;
			const int numInputChannels = numChannels - numOutputChannels;
			BoolArray outputs( numPorts );
			for( int port = 0; port < numOutputPorts; port++ )
				outputs.at( port ) = true;			// relays are outputs
			for( int port = numOutputPorts; port < numPorts; port++ )
				outputs.at( port ) = false;			// other ports are inputs
			BoolArray values( numOutputChannels );
			values.assign( values.size(), true );	// open all relays
       		device.dio().configure( false, outputs, values );
			cout << "successful" << endl;

			/*
			 * demonstrate writing outputs individually
			 */
			cout
				<< "In the following demonstrations, you should hear the relays click\n"
				<< "and see the LED on the device blink.\n";
			cout << "  CLosing relays:" << flush << dec;
			for( int channel = 0; channel < numOutputChannels; channel++ ) {
				cout << " " << channel << flush;
       			device.dio().write( channel, false );	// close relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl << "  Opening relays:" << flush;
			for( int channel = 0; channel < numOutputChannels; channel++ ) {
				cout << " " << channel << flush;
       			device.dio().write( channel, true );	// open relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl;

			/*
			 * demonstrate writing outputs as a group
			 */
			cout << "Closing all relays ... " << flush;
			values.assign( values.size(), false );	// close all relays
       		device.dio().write( 0, values );
			cout << "successful" << endl;
			sleep( 2 );
			cout << "Opening all relays ... " << flush;
			values.assign( values.size(), true );	// open all relays
       		device.dio().write( 0, values );
			cout << "successful" << endl;

			/*
			 * demonstrate reading inputs individually
			 */
			sleep( 1 );
			cout << "CLosing every other relay:";
			for( int channel = 0; channel < numOutputChannels; channel += 2 ) {
				cout << " " << channel << flush;
       			device.dio().write( channel, false );	// close relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl << "Reading back relays:" << endl;
			for( int channel = 0; channel < numOutputChannels; channel++ ) {
       			cout << "  Channel " << channel << " = " << device.dio().read( channel ) << endl;
			}	// for( int channel ...
			cout << "Reading isolated inputs:" << endl;
			for( int channel = 0; channel < numInputChannels; channel++ ) {
       			cout << "  Channel " <<  ( numOutputChannels + channel ) << " = " << device.dio().read( numOutputChannels + channel ) << endl;
			}	// for( int channel ...

			/*
			 * open all relays
			 */
			values.assign( values.size(), true );
       		device.dio().write( 0, values );
		} else
			cout << "No USB-IDIO-8 devices found on USB bus" << endl;
       	deviceManager.close();
	} catch( exception &ex ) {
		cerr << "Error \'" << ex.what() << "\' occurred while manipulating device" << endl;
       	if( deviceManager.isOpen() )
       		deviceManager.close();
	}	// catch( ...
}

/* end of file */
