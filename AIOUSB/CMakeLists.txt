#
# CMAKE file for building aiousb. Please note that this
# version is required to be able to build static and 
# shared library versions.

MESSAGE( STATUS "${CMAKE_SOURCE_DIR}" )


SET( THIS_PROJECT "OFF" )

OPTION(BUILD_SAMPLES "Build the AIOUSB Samples" ON)
OPTION(BUILD_PERL "Build the Perl Interfaces" OFF)
OPTION(BUILD_PYTHON "Build the Python Interfaces" OFF)
OPTION(BUILD_JAVA "Build the Java Interfaces" OFF)
OPTION(BUILD_R "Build the R Interfaces" OFF)
OPTION(BUILD_OCTAVE "Build the Octave Interfaces" OFF)
OPTION(BUILD_PHP "Build the PHP Interfaces" OFF)
OPTION(BUILD_DOCUMENTATION "Build the Doxygen documentation" OFF )
OPTION(INSTALL_WRAPPERS_RELATIVE "Install Scripting languages in Paths relative to CMAKE_INSTALL_PREFIX" ON )
OPTION(GUESS_WRAPPER_INSTALL_DIRECTORIES "Guess the location of Wrapper installation paths" ON )



#set(CPACK_DEB_COMPONENT_INSTALL ON)
#set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
#set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
#set(CPACK_INSTALL_DEFAULT_COMPONENT_NAME aiousblib )


SET(CPACK_COMPONENT_AIOUSBLIB_DISPLAY_NAME "ACCES I/O USB libraries")
SET(CPACK_COMPONENT_AIOUSBLIB_DESCRIPTION "USB drivers for the ACCES I/O Products' line of USB data acquisition boards")
SET(CPACK_COMPONENT_AIOUSBLIB_GROUP "Foo")

ENABLE_TESTING()


if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a 'build' directory outside of the source code and call 'cmake' or 'ccmake' from there")
endif()

set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.12)


project( AIOUSBLIBS )


# ROOT DIRECTORY FOR INSTALLATION
#SET( AIOUSB_INSTALL_PREFIX /usr/share/aiousb )


# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# SET(CMAKE_INSTALL_PREFIX "${AIOUSB_INSTALL_PREFIX}" CACHE INTERNAL "Prefix for installation directories" FORCE)


set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts)
set(C_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/classlib)
set(SWIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/wrappers )

# Threads are required in both Win and Unix 
# Acquisition libraries
find_package(Threads REQUIRED )
find_package(SWIG )
find_package(Doxygen)
find_package(Perl)

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Linux / Mac Build 
# 1. Put this into an included file
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
if(  UNIX OR APPLE ) 
  find_package(libusb-1.0 REQUIRED )
  include_directories( ${LIBUSB_1_INCLUDE_DIRS} )
  MESSAGE( STATUS "Trying Apple / Unix" )

  set(EXTRA_LIBS m)
  if( CMAKE_COMPILER_IS_GNUCC )
    set( USE_GCC TRUE )
  elseif( CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(USE_CLANG TRUE )
  endif( CMAKE_COMPILER_IS_GNUCC ) 


  if("$ENV{CFLAGS}" STREQUAL "")
    if(USE_GCC OR USE_CLANG)
      MESSAGE(STATUS "GCC compiler?:${USE_GCC}")
      if( USE_GCC ) 
      	  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
      endif( USE_GCC )

      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__aiousb_cplusplus ")
      if (USE_GCC AND NOT CYGWIN )
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fPIC" )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )	
      endif(USE_GCC AND NOT CYGWIN )

      set(CORELIBS pthread usb-1.0 )
    endif(USE_GCC OR USE_CLANG)
  else("$ENV{CFLAGS}" STREQUAL "")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")
    list(APPEND EXTRA_CFLAGS "$ENV{CFLAGS}")
  endif("$ENV{CFLAGS}" STREQUAL "")

endif( UNIX OR APPLE)


add_subdirectory(lib)
include_directories(lib)
add_subdirectory(classlib)
include_directories(classlib)


if ( SWIG_FOUND ) 
  if ( BUILD_PYTHON )
    MESSAGE(STATUS "Adding wrapper for python" )
    add_subdirectory("lib/wrappers/python")
  endif (BUILD_PYTHON)
  if ( BUILD_PERL )
    MESSAGE(STATUS "Adding wrapper for Perl" )
    add_subdirectory("lib/wrappers/perl")
  endif (BUILD_PERL)
  if ( BUILD_JAVA )
    MESSAGE(STATUS "Adding wrapper for Java" )
    add_subdirectory("lib/wrappers/java")
  endif (BUILD_JAVA)
  if ( BUILD_RUBY )
    MESSAGE(STATUS "Adding wrapper for Ruby" )
    add_subdirectory("lib/wrappers/ruby")
  endif (BUILD_RUBY)
  if ( BUILD_OCTAVE )
    MESSAGE(STATUS "Adding wrapper for Octave" )
    add_subdirectory("lib/wrappers/octave")
  endif (BUILD_OCTAVE )
  if ( BUILD_R )
    MESSAGE(STATUS "Adding wrapper for R" )
    add_subdirectory("lib/wrappers/R")
  endif (BUILD_R)
  if ( BUILD_PHP )
    MESSAGE(STATUS "Adding wrapper for PHP" )
    add_subdirectory("lib/wrappers/php")
  endif (BUILD_PHP)
endif( SWIG_FOUND )


add_subdirectory(samples/TestLib)
SET( AIOUSB_TESTCASELIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/samples/TestLib )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Auto build the samples directory
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

IF( BUILD_SAMPLES ) 
  FOREACH( SAMPLE_DIR USB-AO16-16 USB-AI16-16 USB-DA12-8A USB-DIO-16 USB-DIO-32 USB-IIRO-16 USB-IDIO-16 USB-IDIO-8 )
    MESSAGE(STATUS "Including samples for samples/${SAMPLE_DIR}")
    STRING(REGEX REPLACE  "-" "_" THIS_PROJECT ${SAMPLE_DIR} )
    ADD_SUBDIRECTORY("samples/${SAMPLE_DIR}")
  ENDFOREACH( SAMPLE_DIR  USB-AO16-16 USB-AI16-16 USB-DA12-8A USB-DIO-16 USB-DIO-32  USB-IIRO-16 USB-IDIO-16 USB-IDIO-8 )
endif( BUILD_SAMPLES )


#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Documentation and man pages
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
IF( DOXYGEN_FOUND AND BUILD_DOCUMENTATION )
  MESSAGE(STATUS "Generating Doxygen Documentation ") 
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  ADD_CUSTOM_COMMAND(OUTPUT BuildDoxygen.txt 
    COMMAND 
    ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND
    ${CMAKE_COMMAND} -E touch BuildDoxygen.txt 
    COMMENT "Generating API documentation with Doxygen"
    DEPENDS aiousb
    )

  IF( PERL_FOUND )
    SET(PAR "(" )
    SET(CPAR ")" )
    SET(REPLACE_COMMAND ${PERL_EXECUTABLE} -i -pne 's/^\\s*${PAR}References|Referenced\\s+by${CPAR}.*//g' * )

    ADD_CUSTOM_COMMAND( TARGET BuildDoxygen.txt
      POST_BUILD
      COMMAND ${REPLACE_COMMAND}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man/man3
      COMMENT "Cleaning up man pages"
      )
    
    ADD_CUSTOM_TARGET( doc ALL DEPENDS BuildDoxygen.txt
      COMMENT "Building Documentation"
      )

  ENDIF( PERL_FOUND )
  
  SET(CPACK_COMPONENT_MANPAGES_DISPLAY_NAME "Man pages")
  SET(CPACK_COMPONENT_MANPAGES_DESCRIPTION "Man pages for the API")
  SET(CPACK_COMPONENT_MANPAGES_GROUP "docs")

  INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man/man3 DESTINATION share/accesio/man COMPONENT manpages )
ENDIF( DOXYGEN_FOUND AND BUILD_DOCUMENTATION )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CPack stuff
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#set(CPACK_RPM_COMPONENT_INSTALL ON)


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ACCES I/O Products USB Drivers")
SET(CPACK_PACKAGE_VENDOR "ACCES")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "111")
SET(CPACK_PACKAGE_VERSION_PATCH "0" )
SET(CPACK_PACKAGE_NAME "libaiousb" )
SET(CPACK_PACKAGE_ARCHITECTURE "i386" )
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "This project contains USB userspace drivers based off of libusb" )
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\n and samples for ACCES I/O Product's line of USB based " )
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\n data acquisition modules." )

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "accesio ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF( WIN32 AND NOT UNIX )        # WIN32
  SET(CPACK_NSIS_CONTACT "support@accesio.com")
ELSE( UNIX )                    # LINUX / UNIX 
  SET(CPACK_SOURCE_STRIP_FILES "")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jimi Damon <jdamon@accesio.com>") #required
  SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
  SET(CPACK_RPM_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
  SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386" )
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 ( >= 1.0.0 ), libusb-dev ( >= 0.1.0 )")
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, cmake ( >= 2.8 ), cmake-curses-gui ( >= 2.8 ) , cmake-data ( >= 2.8 )")
  SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/accesio/AIOUSB" )
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, gcc , binutils")

  # RPM Stuff
  SET(CPACK_RPM_PACKAGE_LICENSE "${CPACK_RESOURCE_FILE_LICENSE}")
  SET(CPACK_RPM_PACKAGE_REQUIRES "libusb-1.0 >= 1.0.0, cmake >= 2.8 , cmake-data >= 2.8")
  SET(CPACK_RPM_PACKAGE_SUMMARY "ACCES I/O Products' libusb based userspace drivers" )
  SET(CPACK_GENERATOR "DEB")

ENDIF(WIN32 AND NOT UNIX )

SET(CPACK_COMPONENT_MANPAGES_GROUP "docs")

SET(CPACK_COMPONENTS_ALL Foo manpages )


INCLUDE(CPack)


