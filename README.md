AIOUSB
======

[ACCES I/O Products](http://accesio.com/)' USB driver library 


This project contains USB drivers and APIs for ACCES I/O Product's line of USB based data acquisition modules. This driver represents a large API collection for communicating with one or more of ACCES I/O Product's line of USB based data acquisition products. All of the core functionality that exists and is supported by the Windows software is implemented in this library for non-Windows based operating systems.  This code base compiles using either GCC and Clang compilers to both shared and static libraries that be can used in applications that need to perform highspeed USB data acquisition.

The entire set of drivers are rely on functionality provided by the [libusb-1.0](http://www.libusb.org/) library. Please see the [prequisites](#prereqs) section to find out about required software for building the driver.

Currently, this project aims at providing 
full support to the following platforms:

* Linux
* Mac OS X 
* Free / Net BSD
* POSIX Compliant Operating Systemsthat can successfully compile and use libusb.


**NOTE**: At this moment using these drivers under Windows is **not** supported although several customers have successfully been able to build and deploy solutions based on this system. We expect to have Windows libusb support available in the near future.


### <a href="prereqs"></a>Prerequisites 
The functionality in this driver depends on the following installed packages.

1. [libusb-1.0](http://www.libusb.org/)
2. [cmake]( http://www.cmake.org/cmake/resources/software.html )
3. [swig](http://swig.org/)




#### Ubuntu / Debian
```bash
sudo apt-get install libusb-1.0 libusb-1.0-0-dev cmake swig
```

#### Fedora / Red Hat
```bash
sudo yum install libusb-1.0 cmake swig
```

#### Open SUSE
```bash
sudo zypper install libusb-1.0 cmake swig
```


-------------------------------------


How to Build
------------
Building ACCES I/O Products' Driver library amounts to compiling C source files to produce C and C++ based shared ( .so ) or static (.a) libraries.  The build process relies on either GNU make or Cmake.  The first method of building ( see [non-cmake users](#noncmake)is a little more involved but will give you the ability to build wrapper language packs.  Currently ,the simplified cmake system is easier to build and install the general libraries but we have been unable to use it to deploy the Swig based wrappers as we would have liked. If you don't require any other languages besides C/C++, I suggest you use the Cmake based approach.


## <a href="#noncmake"></a>Non-CMake users

You will need to do the following

```bash
cd AIOUSB
source sourceme.sh
cd lib && make && cd -
cd classlib && make && cd -
cd sample/USB_SAMPLE
make sample AIOUSBLIBDIR=${AIO_LIB_DIR} AIOUSBCLASSLIBDIR=${AIO_CLASSLIB_DIR} DEBUG=1
```

## Build with CMake

```bash
cd AIOUSB
mkdir build
cd build
cmake ..
make
sudo make install
```



## Extra Language Support
In addition, to providing fully functional C Shared and Static libraries, this project also provides
wrapper language support for the following languages:

* Java
* Perl
* Python
* Ruby
* PHP
* Octave
* R

Users who wish to build web applications around the ACCES I/O Product line might consider one of these
for faster development cycles. Suggestions for additional languages and features are well received and can 
be made to suggestions _AT_  accesio _DOT_ com


Sincerely,

The ACCES I/O Development team.
