#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CMAKE file for building Python wrappers 
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

MESSAGE(STATUS "Rules for building Python wrapper")

project( aiousblibs )

# Required that we have BUILD_AIOUSB_SHARED ON


FIND_PACKAGE( SWIG REQUIRED )
FIND_PACKAGE( PythonInterp REQUIRED )
INCLUDE(${SWIG_USE_FILE})

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Python
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
FIND_PACKAGE( PythonLibs )
INCLUDE_DIRECTORIES( ${PYTHON_INCLUDE_PATH} )
SET_SOURCE_FILES_PROPERTIES(AIOUSB.i PROPERTIES CPLUSPLUS OFF)
SET_SOURCE_FILES_PROPERTIES(AIOUSB.i PROPERTIES SWIG_FLAGS "-I${AIOUSB_INCLUDE_DIR} -includeall")

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig; print distutils.sysconfig.get_python_version()"
  OUTPUT_VARIABLE PY_VERSION 
  RESULT_VARIABLE _res 
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

SWIG_ADD_MODULE(AIOUSB python AIOUSB.i )
SWIG_LINK_LIBRARIES(AIOUSB ${PYTHON_LIBRARIES} aiousb )

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/AIOUSB.py DESTINATION lib/python${PY_VERSION} )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/_AIOUSB.so DESTINATION lib/python${PY_VERSION} )
