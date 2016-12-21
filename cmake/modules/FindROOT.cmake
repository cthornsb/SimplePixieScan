# - Finds ROOT instalation
# This module sets up ROOT information 
# It defines:
# ROOT_FOUND          If the ROOT is found
# ROOT_INCLUDE_DIR    PATH to the include directory
# ROOT_LIBRARIES      Most common libraries
# ROOT_GUI_LIBRARIES  Most common gui libraries
# ROOT_LIBRARY_DIR    PATH to the library directory 
#
#Last updated by K. Smith (ksmit218@utk.edu) on Apr 10, 2014

#Find the root-config executable
set(ROOTSYS $ENV{ROOTSYS} CACHE Path "ROOT directory.")
find_program(ROOT_CONFIG_EXECUTABLE root-config
  PATHS ${ROOTSYS}/bin)
find_program(ROOTCINT_EXECUTABLE rootcint PATHS $ENV{ROOTSYS}/bin)
find_program(GENREFLEX_EXECUTABLE genreflex PATHS $ENV{ROOTSYS}/bin)

#If we found root-config then get all relevent varaiables
if(ROOT_CONFIG_EXECUTABLE)
  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --prefix 
    OUTPUT_VARIABLE ROOTSYS 
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --version 
    OUTPUT_VARIABLE ROOT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --incdir
    OUTPUT_VARIABLE ROOT_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE ROOT_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --glibs
    OUTPUT_VARIABLE ROOT_GUI_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(ROOT_LIBRARY_DIR ${ROOTSYS}/lib)
endif()

#---Report the status of finding ROOT-------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ROOT DEFAULT_MSG 
		ROOTSYS ROOT_CONFIG_EXECUTABLE ROOTCINT_EXECUTABLE GENREFLEX_EXECUTABLE
		ROOT_VERSION ROOT_INCLUDE_DIR ROOT_LIBRARIES ROOT_LIBRARY_DIR)

mark_as_advanced(FORCE ROOT_LIBRARIES ROOT_GUI_LIBRARIES)
