
macro ( build_sample_c_file project c_file ) 
  GET_FILENAME_COMPONENT( tmp_c_file ${c_file} NAME )
  STRING(REGEX REPLACE "\\.c$" "" binary_name ${tmp_c_file})
  ADD_EXECUTABLE( "${project}_${binary_name}" ${c_file} )
  TARGET_LINK_LIBRARIES( "${project}_${binary_name}" TestCaseSetup aiousb aiousbcpp)
endmacro ( build_sample_c_file) 

macro ( build_sample_cpp_file project cpp_file )
  GET_FILENAME_COMPONENT( tmp_cpp_file ${cpp_file} NAME )
  STRING(REGEX REPLACE "\\.cpp$" "" binary_name ${tmp_cpp_file})
  ADD_EXECUTABLE( "${project}_${binary_name}" ${cpp_file} )
  SET_TARGET_PROPERTIES( "${project}_${binary_name}" PROPERTIES OUTPUT_NAME  ${binary_name} ) 
  TARGET_LINK_LIBRARIES( "${project}_${binary_name}" TestCaseSetup aiousbdbg aiousbcppdbg classaiousbdbg)
endmacro ( build_sample_cpp_file )


macro ( build_all_samples project ) 
  file( GLOB C_FILES ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/*.c" )
  file( GLOB CXX_FILES ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" )
  foreach( c_file ${C_FILES} )
    build_sample_c_file( ${project} ${c_file} )
  endforeach( c_file )
  foreach( cpp_file ${CXX_FILES} )
    build_sample_cpp_file( ${project} ${cpp_file} )
  endforeach( cpp_file )
endmacro ( build_all_samples )

macro ( include_testcase_lib project )
  IF( NOT AIOUSB_INCLUDE_DIR)
    SET(AIOUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib )
    # MESSAGE(STATUS "Setting AIOUSB_INCLUDE_DIR to be : ${AIOUSB_INCLUDE_DIR}" )
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
  # MESSAGE(status "Doing something")

  SET(CMAKE_CXX_FLAGS "-D__aiousb_cplusplus -fPIC" )
  SET(CMAKE_C_FLAGS   "-std=gnu99 " )

endmacro( include_testcase_lib )


