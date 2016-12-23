# - Finds paass instalation
# This module sets up paass information 
# It defines:
#
# PAASS_FOUND          If the paass installation is found
# PAASS_DIR            PATH to the paass install directory
# PAASS_INCLUDE_DIR    PATH to the include directory
# PAASS_LIBRARY_DIR    PATH to the library directory 
#
#Last updated by C. R. Thornsberry (cthornsb@vols.utk.edu) on Dec. 20, 2016

if(EXISTS /opt/paass/)
	set(PAASS_DIR_EXISTS TRUE)
	set(PAASS_DIR "/opt/paass" CACHE Path "PATH to the paass install directory.")
elseif(EXISTS $ENV{HOME}/opt/paass/install/)
	set(PAASS_DIR_EXISTS TRUE)
	set(PAASS_DIR "$ENV{HOME}/opt/paass/install" CACHE Path "PATH to the paass install directory.")
endif()

if(DEFINED PAASS_DIR_EXISTS)
	if(EXISTS ${PAASS_DIR}/include/)
		set(PAASS_INCLUDE_DIR "${PAASS_DIR}/include" CACHE Path "PATH to paass the include directory.")
	endif()
	if(EXISTS ${PAASS_DIR}/lib)
		set(PAASS_LIBRARY_DIR "${PAASS_DIR}/lib" CACHE Path "PATH to the paass library directory.")
		if(EXISTS "${PAASS_LIBRARY_DIR}/libScan.so")
			set(PAASS_LIB_SCAN "${PAASS_LIBRARY_DIR}/libScan.so" CACHE Path "Paass scan library shared object.")
		endif()
		if(EXISTS "${PAASS_LIBRARY_DIR}/libScanStatic.a")
			set(PAASS_LIB_SCAN_STATIC "${PAASS_LIBRARY_DIR}/libScanStatic.a" CACHE Path "Paass scan static library.")
		endif()
	endif()
endif()

#---Report the status of finding paass-------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAASS DEFAULT_MSG PAASS_DIR PAASS_INCLUDE_DIR PAASS_LIBRARY_DIR 
                                  PAASS_LIB_SCAN PAASS_LIB_SCAN_STATIC)
                                  
mark_as_advanced(PAASS_LIB_SCAN PAASS_LIB_SCAN_STATIC)
