#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CMAKE file for building aioclasslib. This library is deprecated
# and will be replaced over time with the SWIG code under lib/wrappers
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

MESSAGE(STATUS "Loading libraries for deprecated AIOUSBCLASSLIB ")

project( classaiousb )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Options for building shared vs. Static libraries
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
OPTION(BUILD_CLASS_AIOUSB_SHARED "Build the AIOUSB C++ Class libraries shared." ON)
OPTION(BUILD_CLASS_AIOUSBDBG_SHARED "Build the AIOUSB C++ Class libraries shared." ON)

IF(BUILD_CLASS_AIOUSB_SHARED)
  SET(CLASSAIOUSB_LIBRARY_TYPE SHARED)
ELSE(BUILD_CLASS_AIOUSB_SHARED)
  SET(CLASSAIOUSB_LIBRARY_TYPE STATIC)
ENDIF(BUILD_CLASS_AIOUSB_SHARED)

IF(BUILD_CLASS_AIOUSBDBG_SHARED)
  SET(CLASSAIOUSBDBG_LIBRARY_TYPE SHARED)
ELSE(BUILD_CLASS_AIOUSBDBG_SHARED)
  SET(CLASSAIOUSBDBG_LIBRARY_TYPE STATIC) 
ENDIF(BUILD_CLASS_AIOUSBDBG_SHARED)

file (GLOB classaiousb_files  ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}" "${C_SOURCE_DIR}/*.c" )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Generate 
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
foreach( cpp_file ${classaiousb_files} )
  string(REGEX REPLACE "\\.cpp$" "" outvar ${cpp_file})
  set(classaiousbdbg_var "${outvar}.dbg.c" )
  LIST(APPEND classaiousbdbg_files ${classaiousbdbg_var} )
  add_custom_command( OUTPUT ${classaiousbdbg_var} COMMAND ${CMAKE_COMMAND} -E copy_if_different ${cpp_file} ${classaiousbdbg_var} )

endforeach( cpp_file  )

add_custom_target( classaiousbdbg_copies DEPENDS ${classaiousbdbg_files} )

# Requires that you have build the aiousb library first
add_library( classaiousb ${CLASSAIOUSB_LIBRARY_TYPE}  ${classaiousb_files} )
add_library( classaiousbdbg ${CLASSAIOUSBDBG_LIBRARY_TYPE} ${classaiousbdbg_files} )

target_link_libraries( classaiousb ${CORELIBS} ${EXTRA_LIBS} aiousb )
target_link_libraries( classaiousbdbg ${CORELIBS} ${EXTRA_LIBS} aiousb )

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Installation targets
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

install( TARGETS classaiousb DESTINATION lib )
install( TARGETS classaiousbdbg DESTINATION lib )


