#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CMAKE file for building libaiousb. Please note that this
# version is required to be able to build static and 
# shared library versions.
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

MESSAGE(STATUS "Loading rules for build libaiousb")
cmake_minimum_required(VERSION 2.8)

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


file( GLOB tmp_aiousb_files ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/*.c" )

# MESSAGE(STATUS "aiousb_files: ${aiousb_files}" )
set( CMAKE_INCLUDE_CURRENT_DIR  true )


#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Generate 
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
foreach( tmp_c_file ${tmp_aiousb_files} )
  get_filename_component( c_file ${tmp_c_file} NAME )
  string(REGEX REPLACE "\\.c$" "" file_name ${c_file})
  get_filename_component( dir ${tmp_c_file} PATH ) 
  # SET( ofilename_wo_stem "${dir}/${c_file}" )
  set(ofilename_wo_stem "${CMAKE_CURRENT_BINARY_DIR}/${file_name}" )
  # MESSAGE(STATUS "ofilename_wo_stem: ${ofilename_wo_stem}")
  set(aiousb_var "${ofilename_wo_stem}.c" )
  # MESSAGE(STATUS "aiousb_var: ${aiousb_var}")
  set(aiousbcpp_var "${ofilename_wo_stem}.cpp" )
  set(aiousbdbg_var "${ofilename_wo_stem}.dbg.c" )
  set(aiousbcppdbg_var "${ofilename_wo_stem}.dbg.cpp" )

  # MESSAGE( STATUS "Appending ${aiousb_var}")
  LIST(APPEND aiousb_files ${aiousb_var} )
  LIST(APPEND aiousbcpp_files ${aiousbcpp_var} )
  LIST(APPEND aiousbdbg_files ${aiousbdbg_var} )
  LIST(APPEND aiousbcppdbg_files ${aiousbcppdbg_var} )
  #  message(STATUS "Trying file ${aiousbcpp_var}" )
  # message(STATUS "Trying file ${aiousb_var}" )
  add_custom_command( OUTPUT ${aiousb_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tmp_c_file} ${aiousb_var} )
  add_custom_command( OUTPUT ${aiousbdbg_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tmp_c_file} ${aiousbdbg_var} )
  add_custom_command( OUTPUT ${aiousbcpp_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tmp_c_file} ${aiousbcpp_var} )
  add_custom_command( OUTPUT ${aiousbcppdbg_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tmp_c_file} ${aiousbcppdbg_var} )
endforeach( tmp_c_file )

add_custom_target( aiousb_copies DEPENDS ${aiousb_files} )
add_custom_target( aiousbcpp_copies DEPENDS ${aiousbcpp_files} )
add_custom_target( aiousbdbg_copies DEPENDS ${aiousbdbg_files} )
add_custom_target( aiousbcppdbg_copies DEPENDS ${aiousbcppdbg_files} )

add_library( aiousb        ${AIOUSB_LIBRARY_TYPE}       ${aiousb_files} )
add_library( aiousbdbg     ${AIOUSBDBG_LIBRARY_TYPE}    ${aiousbdbg_files} )
add_library( aiousbcpp     ${AIOUSBCPP_LIBRARY_TYPE}    ${aiousbcpp_files}  )
add_library( aiousbcppdbg  ${AIOUSBCPPDBG_LIBRARY_TYPE} ${aiousbcppdbg_files} )

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )
# MESSAGE(STATUS "Setting include path to include '${CMAKE_CURRENT_SOURCE_DIR}'")
set( AIOUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}  )

target_link_libraries( aiousb ${CORELIBS} ${EXTRA_LIBS} )
target_link_libraries( aiousbcpp ${CORELIBS} ${EXTRA_LIBS} )
target_link_libraries( aiousbcppdbg ${CORELIBS} ${EXTRA_LIBS} )
target_link_libraries( aiousbdbg ${CORELIBS} ${EXTRA_LIBS} )

add_dependencies( aiousb aiousb_copies )
add_dependencies( aiousbcpp aiousbcpp_copies )
add_dependencies( aiousbcppdbg aiousbcppdbg_copies )
add_dependencies( aiousbdbg aiousbdbg_copies )


#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Testing targets
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#gcc -O0   -g  -I. -DAIOUSB_DEBUG_LOG \
#                  -DAIO_DEBUG \
#                  -DTAP_TEST \
#                  -DHAS_PTHREAD \
#                  -DSELF_TEST \
#                  -std=gnu99 -g -o thread_test AIOContinuousBuffer.c \
#                  libaiousbdbg.a -lpthread -lusb-1.0 -lm
INCLUDE( build_samples )


set(SELF_TEST_ENTRIES AIOContinuousBuffer.c )
foreach( selftest_file ${SELF_TEST_ENTRIES} )
  set(MY_FLAGS "-DAIOUSB_DEBUG_LOG -DAIO_DEBUG -DTAP_TEST -DHAS_PTHREAD -DSELF_TEST" )
  set(MY_LIBRARIES aiousbdbg pthread usb-1.0 m )

  # macro ( build_selftest_c_file project c_file  cflags link_libraries )
  build_selftest_c_file( aiousblibs ${selftest_file} ${MY_FLAGS} ${MY_LIBRARIES} )
endforeach( selftest_file ${SELF_TEST_ENTRIES}  )

#ADD_EXECUTABLE( "${project}_${binary_name}" ${cpp_file} )
#SET_TARGET_PROPERTIES( "${project}_${binary_name}" PROPERTIES OUTPUT_NAME  ${binary_name} ) 
#TARGET_LINK_LIBRARIES( "${project}_${binary_name}" TestCaseSetup aiousbdbg aiousbcppdbg classaiousbdbg)



#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Installation targets
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

install( TARGETS aiousb DESTINATION lib )
install( TARGETS aiousbdbg DESTINATION lib )
install( TARGETS aiousbcpp DESTINATION lib )
install( TARGETS aiousbcppdbg DESTINATION lib )
install( FILES AIOUSB_Core.h AIOUSB_USB.h aiousb.h AIOTypes.h DESTINATION include )


