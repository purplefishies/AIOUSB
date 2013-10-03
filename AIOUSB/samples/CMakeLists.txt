#
# Build samples for USB-AO16-16
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# if ( ${THIS_PROJECT} STREQUAL "" )
#   message(FATAL_ERROR "Expected THIS_PROJECT to be defined, not '${THIS_PROJECT}'")
# endif ( ${THIS_PROJECT} STREQUAL "")

if ( NOT THIS_PROJECT ) 
   message(FATAL_ERROR "Expected THIS_PROJECT to be defined, not '${THIS_PROJECT}'")
endif( NOT THIS_PROJECT )

# UNSET(THIS_PROJECT)
#message(STATUS "THIS_PROJECT: '${THIS_PROJECT}'")


INCLUDE( build_samples )
include_testcase_lib( ${THIS_PROJECT} )
build_all_samples( ${THIS_PROJECT} )




