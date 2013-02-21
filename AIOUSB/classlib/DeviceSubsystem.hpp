/*
 * $RCSfile: DeviceSubsystem.hpp,v $
 * $Revision: 1.8 $
 * $Date: 2009/12/19 00:27:36 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * class DeviceSubsystem declarations
 */


#if ! defined( DeviceSubsystem_hpp )
#define DeviceSubsystem_hpp

// {{{ includes
#include <USBDevice.hpp>
// }}}

namespace AIOUSB {

// {{{ class DeviceSubsystem declarations

/**
 * Class DeviceSubsystem is the abstract super class for all device subsystems.
 */

class DeviceSubsystem {
protected:

	// {{{ protected members
	USBDevice *parent;							// parent device that this subsystem is part of
	// }}}

	// {{{ protected methods
	DeviceSubsystem( USBDevice &parent );
	virtual ~DeviceSubsystem();
	int getDeviceIndex() const;
	// }}}

public:

	/*
	 * properties
	 */

	virtual std::ostream &print( std::ostream &out ) = 0;

	/**
	 * Gets the parent device that this subsystem is part of.
	 * @return The parent device that this subsystem is part of.
	 */

	USBDevice &getParent() {
		return *parent;
	}	// getParent()

};	// class DeviceSubsystem

// }}}

}	// namespace AIOUSB

#endif


/* end of file */
