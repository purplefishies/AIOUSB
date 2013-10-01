#
# CMAKE file for building aiousb. Please note that this
# version is required to be able to build static and 
# shared library versions.

MESSAGE( STATUS "${CMAKE_SOURCE_DIR}" )

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a 'build' directory outside of the source code and call 'cmake' or 'ccmake' from there")
endif()


cmake_minimum_required(VERSION 2.8)

# project( aiousblibs )

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
#include(compiler_info)

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
add_subdirectory(lib/wrappers)
add_subdirectory(samples/TestLib)
SET( AIOUSB_TESTCASELIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/samples/TestLib )

add_subdirectory(samples/USB-AI16-16)
