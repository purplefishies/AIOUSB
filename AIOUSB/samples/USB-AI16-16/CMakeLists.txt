#
# Build samples for USB-AI16-16
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

if( NOT AIOUSB_INCLUDE_DIR)
  set(AIOUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib )
  MESSAGE(STATUS "Setting AIOUSB_INCLUDE_DIR to be : ${AIOUSB_INCLUDE_DIR}" )
  LINK_DIRECTORIES( ${AIOUSB_INCLUDE_DIR} )
  INCLUDE_DIRECTORIES( ${AIOUSB_INCLUDE_DIR} )
  SET( CMAKE_SHARED_LINKER_FLAGS "-L${AIOUSB_INCLUDE_DIR}" )
endif(NOT AIOUSB_INCLUDE_DIR )

if( NOT LIBUSB_FOUND )
  SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeScripts )
  INCLUDE_DIRECTORIES( ${AIOUSB_INCLUDE_DIR} )
  FIND_PACKAGE(libusb-1.0 REQUIRED )
  INCLUDE_DIRECTORIES( ${LIBUSB_1_INCLUDE_DIRS} )
endif( NOT LIBUSB_FOUND )

if( NOT AIOUSB_TESTCASELIB_DIR ) 
  SET( AIOUSB_TESTCASELIB_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../TestLib )
endif( NOT AIOUSB_TESTCASELIB_DIR )
INCLUDE_DIRECTORIES( ${AIOUSB_TESTCASELIB_DIR} )
LINK_DIRECTORIES( ${AIOUSB_TESTCASELIB_DIR} )  


SET(CMAKE_CXX_FLAGS "-D__aiousb_cplusplus -fPIC" )
SET(CMAKE_C_FLAGS   "-std=gnu99 " )


MESSAGE(STATUS "Using INCLUDE DIR ${AIOUSB_TESTCASELIB_DIR}")
INCLUDE_DIRECTORIES( ${AIOUSB_TESTCASELIB_DIR} )


file( GLOB C_FILES ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/*.c" )
file( GLOB CXX_FILES ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" )

macro ( build_sample_c_file c_file ) 
# foreach( c_file ${C_FILES} )
  GET_FILENAME_COMPONENT( tmp_c_file ${c_file} NAME )
  STRING(REGEX REPLACE "\\.c$" "" file_name ${tmp_c_file})
  ADD_EXECUTABLE( ${file_name} ${c_file} )
  TARGET_LINK_LIBRARIES( ${file_name} TestCaseSetup aiousb aiousbcpp)
# endforeach( c_file )
endmacro ( build_sample_c_file) 

macro ( build_sample_cpp_file cpp_file )
  GET_FILENAME_COMPONENT( tmp_cpp_file ${cpp_file} NAME )
  STRING(REGEX REPLACE "\\.cpp$" "" file_name ${tmp_cpp_file})
  ADD_EXECUTABLE( ${file_name} ${cpp_file} )
  TARGET_LINK_LIBRARIES( ${file_name} TestCaseSetup aiousbdbg aiousbcppdbg classaiousbdbg)
endmacro ( build_sample_cpp_file )

foreach( c_file ${C_FILES} )
  build_sample_c_file( ${c_file} )
endforeach( c_file )

foreach( cpp_file ${CXX_FILES} )
  build_sample_cpp_file( ${cpp_file} )
  # GET_FILENAME_COMPONENT( tmp_cpp_file ${cpp_file} NAME )
  # STRING(REGEX REPLACE "\\.cpp$" "" file_name ${tmp_cpp_file})
  # ADD_EXECUTABLE( ${file_name} ${cpp_file} )
  # TARGET_LINK_LIBRARIES( ${file_name} TestCaseSetup aiousbdbg aiousbcppdbg classaiousbdbg)
endforeach( cpp_file )


