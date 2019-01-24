###############################################################################
#
# - Finds SimpleScan instalation
# This module sets up SimpleScan information 
# It defines:
#
# SimpleScan_FOUND          If the SimpleScan installation is found
# SimpleScan_INCLUDE_DIR    PATH to the include directory
# SimpleScan_BIN_DIR        PATH to the bin directory
# SimpleScan_SCAN_LIB       PATH to the SimpleScan scan library
# SimpleScan_OPT_LIB        PATH to the option handler library
#
# Last updated by C. R. Thornsberry (cthornsb@vols.utk.edu) on Oct. 24th, 2017
#
# - Possible use of this cmake module.
# Copy the following 4 lines into your main CMakeLists.txt
#
##Find simpleScan install.
#find_package (SimpleScan REQUIRED)
#include_directories(${SimpleScan_INCLUDE_DIR})
#mark_as_advanced(SimpleScan_SCAN_LIB SimpleScan_OPT_LIB)
#
# Use SimpleScan_SCAN_LIB and SimpleScan_OPT_LIB as you would any other
# shared library. e.g.
#
#target_link_libraries(<Executable> SomeOtherLibrary ${SimpleScan_SCAN_LIB} ${SimpleScan_OPT_LIB})
#
###############################################################################

find_path(SimpleScan_INCLUDE_DIR
	NAMES ScanInterface.hpp
	PATHS /opt/simpleScan/install
	PATH_SUFFIXES include)

find_path(SimpleScan_BIN_DIR
	NAMES simpleScan
	PATHS /opt/simpleScan/install
	PATH_SUFFIXES bin)

find_library(SimpleScan_SCAN_LIB
	NAMES libSimpleScan.so
	PATHS /opt/simpleScan/install
	PATH_SUFFIXES lib)

find_library(SimpleScan_OPT_LIB
	NAMES libSimpleOption.so
	PATHS /opt/simpleScan/install
	PATH_SUFFIXES lib)

find_library(SimpleScan_DICTIONARY
	NAMES libRootDict.so
	PATHS /opt/simpleScan/install
	PATH_SUFFIXES lib)

#---Report the status of finding SimpleScan-------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SimpleScan DEFAULT_MSG SimpleScan_INCLUDE_DIR SimpleScan_BIN_DIR SimpleScan_SCAN_LIB SimpleScan_OPT_LIB SimpleScan_DICTIONARY)
