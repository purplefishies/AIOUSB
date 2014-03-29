#
# CMAKE file for building aiousb. Please note that this
# version is required to be able to build static and 
# shared library versions.

MESSAGE( STATUS "${CMAKE_SOURCE_DIR}" )


SET( THIS_PROJECT "OFF" )

OPTION(BUILD_SAMPLES "Build the AIOUSB Samples" ON)
OPTION(BUILD_WRAPPERS "Build the Wrapper Interfaces" ON)



if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a 'build' directory outside of the source code and call 'cmake' or 'ccmake' from there")
endif()


cmake_minimum_required(VERSION 2.8)

project( AIOUSBLIBS )

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")



set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts)
set(C_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/classlib)
set(SWIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/wrappers )

# Threads are required in both Win and Unix 
# Acquisition libraries
find_package(Threads REQUIRED )
find_package(SWIG )


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
      # MESSAGE( STATUS "Setting CFLAGS" )
      set(CMAKE_C_FLAGS   "-std=gnu99 -fPIC" )
      set(CMAKE_CXX_FLAGS "-D__aiousb_cplusplus -fPIC" )
      set(CORELIBS pthread usb-1.0 )
    endif(USE_GCC OR USE_CLANG)
  else("$ENV{CFLAGS}" STREQUAL "")
    set(CMAKE_C_FLAGS "$ENV{CFLAGS}")
    list(APPEND EXTRA_CFLAGS "$ENV{CFLAGS}")
  endif("$ENV{CFLAGS}" STREQUAL "")
endif( UNIX OR APPLE)

add_subdirectory(lib)
include_directories(lib)
add_subdirectory(classlib)
include_directories(classlib)


if ( SWIG_FOUND AND BUILD_WRAPPERS ) 
  # FOREACH( language python )
  FOREACH( language perl java )
    MESSAGE(STATUS "Adding wrapper for ${language}" )
    add_subdirectory("lib/wrappers/${language}")
  ENDFOREACH( language )
endif( SWIG_FOUND AND BUILD_WRAPPERS )

# add_subdirectory(lib/wrappers/perl)
# add_subdirectory(lib/wrappers/ruby)

add_subdirectory(samples/TestLib)
SET( AIOUSB_TESTCASELIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/samples/TestLib )

#
# Auto build the samples directory
#

if( BUILD_SAMPLES ) 
  
  FOREACH( SAMPLE_DIR USB-AO16-16 USB-AI16-16 USB-DA12-8A USB-DIO-16 USB-DIO-32  USB-IIRO-16)
    MESSAGE(STATUS "Including samples for samples/${SAMPLE_DIR}")
    STRING(REGEX REPLACE  "-" "_" THIS_PROJECT ${SAMPLE_DIR} )
    ADD_SUBDIRECTORY("samples/${SAMPLE_DIR}")
  ENDFOREACH( SAMPLE_DIR )
endif( BUILD_SAMPLES )

#
# CPack stuff
#

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ACCES I/O Products AIOUSB Drivers")
SET(CPACK_PACKAGE_VENDOR "accesio")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "98")
# SET(CPACK_PACKAGE_VERSION_PATCH "8")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "accesio ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF( WIN32 AND NOT UNIX )        # WIN32
  SET(CPACK_NSIS_CONTACT "support@accesio.com")
ELSE( WIN32 AND NOT UNIX )      # LINUX / UNIX
  SET(CPACK_DEBIAN_PACKAGE_NAME "libaiousb" )
  SET(CPACK_DEBIAN_PACKAGE_VERSION "1.98" )
  SET(CPACK_SOURCE_STRIP_FILES "")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jimi Damon") #required
  SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "" )
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 ( >= 1.0.0 ), libusb-dev ( >= 0.1.0 ) ")

  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")

  # Install everything into /usr/share/accesio
  # bin
  # lib
  # man
  # doc
  # Include the files for 
  #
  #
  #
  #

  SET(CPACK_GENERATOR "DEB")

ENDIF(WIN32 AND NOT UNIX )
#SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")
INCLUDE(CPack)
