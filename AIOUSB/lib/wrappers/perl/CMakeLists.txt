#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# CMAKE file for building Perl wrapper.
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

MESSAGE(STATUS "Rules for building wrapper scripts")

project( ruby_swig )

# Required that we have BUILD_AIOUSB_SHARED ON


FIND_PACKAGE( SWIG REQUIRED )
INCLUDE(${SWIG_USE_FILE})

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Perl
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
FIND_PACKAGE( Perl )
FIND_PACKAGE( PerlLibs )
INCLUDE_DIRECTORIES( ${PERL_INCLUDE_PATH} ${PERL_ARCHLIB}/Core )
MESSAGE(STATUS "Using dirs: ${PERL_INCLUDE_PATH} ${PERL_ARCHLIB}/Core " )
set(CMAKE_C_FLAGS " -D_GNU_SOURCE ")
SET_SOURCE_FILES_PROPERTIES(AIOUSB.i PROPERTIES CPLUSPLUS OFF)
SET_SOURCE_FILES_PROPERTIES(AIOUSB.i PROPERTIES SWIG_FLAGS "-I${AIOUSB_INCLUDE_DIR} -includeall ")
 # -lusb-1.0 -lpthread -laiousbdbg  -laiousbcppdbg 
SWIG_ADD_MODULE(AIOUSBPerl perl AIOUSB.i )
SET_TARGET_PROPERTIES(AIOUSBPerl PROPERTIES OUTPUT_NAME "AIOUSB" ) 

SWIG_LINK_LIBRARIES(AIOUSBPerl ${PERL_LIBRARY} aiousb )

