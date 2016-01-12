![ACCES I/O Products Inc. Logo](http://accesio.com/images/weblogo2.png)


[TOC]

# AIOUSB driver library {#AIOUSBLibrary}

This project contains USB drivers and APIs for ACCES I/O Product's line of USB based data acquisition modules. This driver represents a large API collection for communicating with one or more of ACCES I/O Product's line of USB based data acquisition products. All of the core functionality that exists and is supported by the Windows software is implemented in this library for non-Windows based operating systems.  This code base compiles using either GCC and Clang compilers to both shared and static libraries that be can used in applications that need to perform highspeed USB data acquisition.

The entire set of drivers are rely on functionality provided by the [libusb-1.0](http://www.libusb.org/) library. Please see the [prequisites](#Prerequisites) section to find out about required software for building the driver.

Currently, this project provides full support to the following platforms:

* Linux (  )
* Mac OS X (  ) 
* Free / Net BSD
* [Rasberry Pi](https://www.raspberrypi.org/) 
* [Beagle Board](http://beagleboard.org/bone)
* POSIX compliant operating systems that can successfully compile and use libusb.


**NOTE**: At this moment using these drivers under Windows is **not** supported although several customers have successfully been able to build and deploy solutions based on this system. We expect to have Windows libusb support available in the near future.


## Prerequisites  {#Prerequisites}
The functionality in this driver depends on the following installed packages.

1. [libusb-1.0](http://www.libusb.org/)
2. [cmake]( http://www.cmake.org/cmake/resources/software.html )
3. [swig](http://swig.org/)
4. (Optionally for python bindings )[python-dev](http://python.org)

#### Ubuntu / Debian
<<<<<<< HEAD
```bash
sudo apt-get install libusb-1.0 libusb-1.0-0-dev cmake swig python-dev
```
=======
~~~bash
sudo apt-get install libusb-1.0 libusb-1.0-0-dev cmake swig
~~~
>>>>>>> IBT: Fixes for doxygen documentation

#### Fedora / Red Hat
~~~bash
sudo yum install libusb-1.0 cmake swig
~~~

#### Open SUSE
~~~bash
sudo zypper install libusb-1.0 cmake swig
~~~

#### Mac OS X

##### Homebrew

~~~bash
brew install libusb  cmake
~~~

##### Darwin Ports

~~~bash
sudo port install libusb cmake
~~~

##### Rasberry Pi

~~~bash
sudo apt-get install libusb-1.0-0 libusb-1.0-0-dev  cmake 
<<<<<<< HEAD
sudo apt-get install python-dev # If you want Python bindings
```
=======
~~~
>>>>>>> IBT: Fixes for doxygen documentation

##### Beagle Board
~~~bash
sudo apt-get install libusb-1.0-0 libusb-1.0-0-dev cmake 
<<<<<<< HEAD
sudo apt-get install python-dev # If you want Python bindings
```
=======
~~~
>>>>>>> IBT: Fixes for doxygen documentation

# <a name="UnixBuild"></a>Building on Linux/ MacOS / BSD / *NIX systems

Building ACCES I/O Products' Driver library amounts to compiling C source files to produce C and C++ based shared ( .so ) or static (.a) libraries.  The build process relies on either GNU make or Cmake.  The first method of building ( see [non-cmake users](#NonCmakeBuild) is a little more involved but will give you the ability to build wrapper language packs.  Currently ,the simplified cmake system is easier to build and install the general libraries but we have been unable to use it to deploy the Swig based wrappers as we would have liked. 


## <a id="NonCmakeBuild"></a>Build with Make

You will need to do the following

~~~bash
cd AIOUSB
source sourceme.sh
cd lib && make && cd -
cd classlib && make && cd -
cd samples/USB_SAMPLE_OF_CHOICE
make sample AIOUSBLIBDIR=${AIO_LIB_DIR} AIOUSBCLASSLIBDIR=${AIO_CLASSLIB_DIR} DEBUG=1
~~~

## Build with CMake {#CMakeBuild}

~~~bash
cd AIOUSB
mkdir build
cd build
cmake ..
make
sudo make install
~~~

## <a name="Installation"></a>Installation


### Linux Installation

1. Install fxload either using the appropriate installation tool for
your platform or by installing from
https://github.com/accesio/fxload.  Copy fxload to a standard location
in your $PATH.

2. sudo cp AIOUSB/Firmware/*.hex /usr/share/usb/

3. sudo cp AIOUSB/Firmware/10-acces*.rules /etc/udev/rules.d


### Mac Installation (work in progress!!)

1. Build and Install fxload from https://github.com/accesio/fxload and
copy fxload to a standard location in your $PATH.


2. Determine the raw USB Device ID for your card by looking for the
Vendor ID 1605 in your System Profiler. Set the variable PRODUCTID to be
this value.


3. Manually upload your corresponding firmware to your device by
running the following:

fxload -t fx2lp -I AIOUSB/Firmware/CORRESPONDING_HEXFILE.hex -D 1605:${PRODUCTID}




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

## How to build Wrapper languages

### CMake {#cmake_build}

This is the easiest way to build the wrapper languages. Perform the following

~~~bash
cmake  -DCMAKE_INSTALL_PREFIX=/some/path/Dir  -DBUILD_PERL=ON -DBUILD_JAVA=ON ..
~~~

This will build the languages for Perl and Java. The remaining languages that can be built are
Python ( -DBUILD_PYTHON=ON ) , Ruby (-DBUILD_RUBY=ON), PHP (-DBUILD_PHP=ON) and R (-DBUILD_R=ON) 
while Octave is currently not ready yet. The installation of these wrapper scripts will default be written
to the CMAKE_INSTALL_PREFIX. To better customize the installation, you should use 

~~~bash
ccmake -DCMAKE_INSTALL_PREFIX=/some/path/Dir ..
~~~

or if you have installed cmake-gui, then

~~~bash
cmake-gui -DCMAKE_INSTALL_PREFIX=/some/path/Dir ..
~~~



### Regular Make system

Perform this step *AFTER* you have already followed the instructions
for building the aiousb libraries.  

#### Perl
~~~bash
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_perl
cd perl
sudo make install

~~~

#### Java

You must make sure that you have the Java Development Kit installed (
JDK ). 
~~~bash
export CPATH=$CPATH:$JAVA_HOME/include # example /usr/lib/jvm/java-7-openjdk-i386/include
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_java
sudo cp java/{AIOUSB.jar,libaiousb.jar} $JAR_FOLDER

~~~

#### Python
~~~bash
pyver=$(python  -c 'import platform; print platform.python_version()')
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_python
sudo cp python/build/lib.linux-$(uname -m)-${pyver}/* /usr/lib/python${pyver}/

~~~

#### Ruby
~~~bash
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_ruby

~~~

#### Octave
~~~bash
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_octave

~~~

#### R
~~~bash
cd AIOUSB/lib/wrappers
make -f GNUMakefile inplace_R
~~~




Users who wish to build web applications around the ACCES I/O Product line might consider one of these
for faster development cycles. Suggestions for additional languages and features are well received and can 
be made to suggestions _AT_  accesio _DOT_ com


Sincerely,

The ACCES I/O Development team.
