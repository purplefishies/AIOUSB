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




if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a 'build' directory outside of the source code and call 'cmake' or 'ccmake' from there")
endif()

set(CMAKE_LEGACY_CYGWIN_WIN32 0)
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
      	  set(CMAKE_C_FLAGS "-std=gnu99")
      endif( USE_GCC )
      if (USE_GCC AND NOT CYGWIN )
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fPIC" )
      endif(USE_GCC AND NOT CYGWIN )
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
IF(DOXYGEN_FOUND)
  MESSAGE(STATUS "Will generate Doxygen") 
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(doc
    ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )

IF( PERL_FOUND )
SET(PAR "(" )
SET(CPAR ")" )
SET(REPLACE_COMMAND ${PERL_EXECUTABLE} -i -pne 's/^\\s*${PAR}References|Referenced\\s+by${CPAR}.*//g' * )
  ADD_CUSTOM_COMMAND( TARGET doc
    POST_BUILD
    COMMAND ${REPLACE_COMMAND}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man/man3
    COMMENT "Cleaning up man pages"
    )
ENDIF( PERL_FOUND )
INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man/man3 DESTINATION man )
ENDIF(DOXYGEN_FOUND)

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CPack stuff
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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
ELSE( UNIX )                    # LINUX / UNIX 
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
