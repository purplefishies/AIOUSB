/*
 * $RCSfile: sample.cpp,v $
 * $Date: 2009/12/25 19:11:55 $
 * $Revision: 1.6 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 */

// {{{ build instructions
/*
 * g++ sample.cpp -lclassaiousb -laiousbcpp -lusb-1.0 -o sample
 *   or
 * g++ -ggdb sample.cpp -lclassaiousbdbg -laiousbcppdbg -lusb-1.0 -o sample
 */
// }}}

// {{{ includes
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <USBDeviceManager.hpp>
#include <USB_DIO_Family.hpp>
// }}}

using namespace AIOUSB;
using namespace std;

int main( int argc, char *argv[] ) {
	const string VERSION = "1.6, 25 December 2009";
	/*
	 * if you want to test writing to the EEPROM, set DEMO_EEPROM_WRITE to 'true'; after writing
	 * to the EEPROM this sample program will attempt to restore the original EEPROM contents,
	 * but you never know ...
	 */
	const bool DEMO_EEPROM_WRITE = false;

/*API*/	USBDeviceManager deviceManager;
	try {
/*API*/	deviceManager.open();
		cout
			<< "USB-IIRO-16 sample program version: " << VERSION << "\n"
/*API*/		<< "  AIOUSB C++ library version: " << deviceManager.VERSION_NUMBER << ", " << deviceManager.VERSION_DATE << "\n"
/*API*/		<< "  AIOUSB library version: " << deviceManager.getAIOUSBVersion() << ", " << deviceManager.getAIOUSBVersionDate() << "\n"
			<< "  This program demonstrates controlling a USB-IIRO-16 device on\n"
			<< "  the USB bus. For simplicity, it uses the first such device found\n"
			<< "  on the bus.\n";
/*API*/	deviceManager.printDevices();
/*API*/	USBDeviceArray devices = deviceManager.getDeviceByProductID( USB_IIRO_16 );
		if( devices.size() > 0 ) {
			USB_DIO_Family &device = *( USB_DIO_Family * ) devices.at( 0 );	// get first device found
/*API*/		device.reset()
/*API*/			.setCommTimeout( 1000 );

			/*
			 * demonstrate writing EEPROM
			 */
			if( DEMO_EEPROM_WRITE ) {
				cout << "Writing to EEPROM ... " << flush;
				const int offset = 0x100;
				string message = "USB-IIRO-16 sample program";
/*API*/			UCharArray prevData = device.customEEPROMRead( offset, message.length() );	// preserve current EEPROM contents
				UCharArray data( message.length() );
				for( int index = 0; index < ( int ) message.length(); index++ )
					data.at( index ) = message.at( index );
/*API*/			device.setCommTimeout( 10000 )	// writing the entire EEPROM (which we're not doing here) can take a long time
/*API*/				.customEEPROMWrite( offset, data );
/*API*/			UCharArray readData = device.customEEPROMRead( offset, message.length() );
				string readMessage( readData.size(), ' ' );
				for( int index = 0; index < ( int ) readData.size(); index++ )
					readMessage.at( index ) = readData.at( index );
/*API*/			device.customEEPROMWrite( offset, prevData )
/*API*/				.setCommTimeout( 1000 );
				cout
					<< ( ( readMessage.compare( message ) == 0 )
						? "successful"
						: "failed: data read back did not compare" )
					<< endl;
			}	// if( DEMO_EEPROM_WRITE )

			/*
			 * demonstrate reading EEPROM
			 */
			cout
				<< "EEPROM contents:\n"
				<< '['
				<< flush << hex << setw( 2 ) << setfill( '0' );
/*API*/		UCharArray data = device.customEEPROMRead( 0, device.CUSTOM_EEPROM_SIZE );
			for( int index = 0; index < ( int ) data.size() - 1; index++ )
				cout << ( unsigned ) data.at( index ) << ", ";
			cout << ( unsigned ) data.at( data.size() - 1 ) << "]\n";

			/*
			 * demonstrate DIO configuration
			 */
			cout << "Configuring digital I/O ... " << flush;
/*API*/		const int numPorts = device.dio().getNumPorts();
			const int numOutputPorts = numPorts / 2;
/*API*/		const int numChannels = device.dio().getNumChannels();
			const int numOutputChannels = numChannels / 2;
			const int numInputChannels = numChannels - numOutputChannels;
			BoolArray outputs( numPorts );
			for( int port = 0; port < numOutputPorts; port++ )
				outputs.at( port ) = true;			// relays are outputs
			for( int port = numOutputPorts; port < numPorts; port++ )
				outputs.at( port ) = false;			// other ports are inputs
			BoolArray values( numOutputChannels );
			values.assign( values.size(), true );	// open all relays
/*API*/		device.dio().configure( false, outputs, values );
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
/*API*/			device.dio().write( channel, false );	// close relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl << "  Opening relays:" << flush;
			for( int channel = 0; channel < numOutputChannels; channel++ ) {
				cout << " " << channel << flush;
/*API*/			device.dio().write( channel, true );	// open relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl;

			/*
			 * demonstrate writing outputs as a group
			 */
			cout << "Closing all relays ... " << flush;
			values.assign( values.size(), false );	// close all relays
/*API*/		device.dio().write( 0, values );
			cout << "successful" << endl;
			sleep( 2 );
			cout << "Opening all relays ... " << flush;
			values.assign( values.size(), true );	// open all relays
/*API*/		device.dio().write( 0, values );
			cout << "successful" << endl;

			/*
			 * demonstrate reading inputs individually
			 */
			sleep( 1 );
			cout << "CLosing every other relay:";
			for( int channel = 0; channel < numOutputChannels; channel += 2 ) {
				cout << " " << channel << flush;
/*API*/			device.dio().write( channel, false );	// close relay
				usleep( 100000 );
			}	// for( int channel ...
			cout << endl << "Reading back relays:" << endl;
			for( int channel = 0; channel < numOutputChannels; channel++ ) {
/*API*/			cout << "  Channel " << channel << " = " << device.dio().read( channel ) << endl;
			}	// for( int channel ...
			cout << "Reading isolated inputs:" << endl;
			for( int channel = 0; channel < numInputChannels; channel++ ) {
/*API*/			cout << "  Channel " <<  ( numOutputChannels + channel ) << " = " << device.dio().read( numOutputChannels + channel ) << endl;
			}	// for( int channel ...

			/*
			 * open all relays
			 */
			values.assign( values.size(), true );
/*API*/		device.dio().write( 0, values );
		} else
			cout << "No USB-IIRO-16 devices found on USB bus" << endl;
/*API*/	deviceManager.close();
	} catch( exception &ex ) {
		cerr << "Error \'" << ex.what() << "\' occurred while manipulating device" << endl;
/*API*/	if( deviceManager.isOpen() )
/*API*/		deviceManager.close();
	}	// catch( ...
}	// main()

/* end of file */
