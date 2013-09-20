#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CMAKE file for building libaiousb. Please note that this
# version is required to be able to build static and 
# shared library versions.
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

MESSAGE(STATUS "Loading rules for build libaiousb")
# MESSAGE( STATUS "${CMAKE_SOURCE_DIR}" )
# MESSAGE( STATUS "${CMAKE_SOURCE_DIR}" )
# if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
#   message(FATAL_ERROR "Prevented in-tree built. Please create a 'build' directory outside of the source code and call 'cmake' or 'ccmake' from there")
# endif()
# cmake_minimum_required(VERSION 2.8.8)

project( aiousblibs )

#
# Options for building shared vs. Static libraries
#
OPTION(BUILD_AIOUSB_SHARED "Build the AIOUSB libraries shared." ON)
OPTION(BUILD_AIOUSBDBG_SHARED "Build the AIOUSB Debug libraries shared." ON)
OPTION(BUILD_AIOUSBCPP_SHARED "Build the AIOUSB C++ libraries shared." ON)
OPTION(BUILD_AIOUSBCPPDBG_SHARED "Build the AIOUSB C++ Debug libraries shared." ON)


#
# Options for building shared vs. Static libraries
#
OPTION(BUILD_AIOUSB_SHARED "Build the AIOUSB libraries shared." ON)
OPTION(BUILD_AIOUSBDBG_SHARED "Build the AIOUSB Debug libraries shared." ON)
OPTION(BUILD_AIOUSBCPP_SHARED "Build the AIOUSB C++ libraries shared." ON)
OPTION(BUILD_AIOUSBCPPDBG_SHARED "Build the AIOUSB C++ Debug libraries shared." ON)

IF(BUILD_AIOUSB_SHARED)
  SET(AIOUSB_LIBRARY_TYPE SHARED)
ELSE(BUILD_AIOUSB_SHARED)
  SET(AIOUSB_LIBRARY_TYPE STATIC)
ENDIF(BUILD_AIOUSB_SHARED)

IF(BUILD_AIOUSBDBG_SHARED)
  SET(AIOUSBDBG_LIBRARY_TYPE SHARED)
ELSE(BUILD_AIOUSBDBG_SHARED)
  SET(AIOUSBDBG_LIBRARY_TYPE STATIC)
ENDIF(BUILD_AIOUSBDBG_SHARED)

IF(BUILD_AIOUSBCPP_SHARED)
  SET(AIOUSBCPP_LIBRARY_TYPE SHARED)
ELSE(BUILD_AIOUSBCPP_SHARED)
  SET(AIOUSBCPP_LIBRARY_TYPE STATIC)
ENDIF(BUILD_AIOUSBCPP_SHARED)

IF(BUILD_AIOUSBCPPDBG_SHARED)
  SET(AIOUSBCPPDBG_LIBRARY_TYPE SHARED)
ELSE(BUILD_AIOUSBCPPDBG_SHARED)
  SET(AIOUSBCPPDBG_LIBRARY_TYPE STATIC)
ENDIF(BUILD_AIOUSBCPPDBG_SHARED)


file (GLOB aiousb_files  ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}" "${C_SOURCE_DIR}/*.c" )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Generate 
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
foreach( c_file ${aiousb_files} )
  string(REGEX REPLACE "\\.c$" "" outvar ${c_file})
  set(aiousbcpp_var "${outvar}.cpp" )
  set(aiousbdbg_var "${outvar}.dbg.c" )
  set(aiousbcppdbg_var "${outvar}.dbg.cpp" )
  LIST(APPEND aiousbcpp_files ${aiousbcpp_var} )
  LIST(APPEND aiousbdbg_files ${aiousbdbg_var} )
  LIST(APPEND aiousbcppdbg_files ${aiousbcppdbg_var} )
  message(STATUS "Trying file ${aiousbcpp_var}" )
  add_custom_command( OUTPUT ${aiousbcpp_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${c_file} ${aiousbcpp_var} )
  add_custom_command( OUTPUT ${aiousbdbg_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${c_file} ${aiousbdbg_var} )
  add_custom_command( OUTPUT ${aiousbcppdbg_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${c_file} ${aiousbcppdbg_var} )
endforeach( c_file )


add_custom_target( aiousbcpp_copies DEPENDS ${aiousbcpp_files} )
add_custom_target( aiousbdbg_copies DEPENDS ${aiousbdbg_files} )
add_custom_target( aiousbcppdbg_copies DEPENDS ${aiousbcppdbg_files} )

add_library( aiousb        ${AIOUSB_LIBRARY_TYPE}       ${aiousb_files} )
add_library( aiousbdbg     ${AIOUSBDBG_LIBRARY_TYPE}    ${aiousbdbg_files} )
add_library( aiousbcpp     ${AIOUSBCPP_LIBRARY_TYPE}    ${aiousbcpp_files}  )
add_library( aiousbcppdbg  ${AIOUSBCPPDBG_LIBRARY_TYPE} ${aiousbcppdbg_files} )

#include_directories( "${C_SOURCE_DIR}" )
#set( AIOUSB_LIBS ${C_SOURCE_DIR} )

target_link_libraries( aiousb ${CORELIBS} ${EXTRA_LIBS} )

add_dependencies( aiousbcpp aiousbcpp_copies )
add_dependencies( aiousbcppdbg aiousbcppdbg_copies )
add_dependencies( aiousbdbg aiousbdbg_copies )



#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Installation targets
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

install( TARGETS aiousb DESTINATION lib )
install( TARGETS aiousbdbg DESTINATION lib )
install( TARGETS aiousbcpp DESTINATION lib )
install( TARGETS aiousbcppdbg DESTINATION lib )
install( FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/AIOUSB_Core.h" 
  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/AIOUSB_USB.h" 
  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/aiousb.h"  
  DESTINATION include 
)
