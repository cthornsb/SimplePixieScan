cmake_minimum_required (VERSION 2.8.11)

project (SimplePixieScan)

#Compile with c++11 or c++0x depending on the compiler version
if (CMAKE_COMPILER_IS_GNUCXX)
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.3" AND
            ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.7")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    elseif(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.6")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.3" AND
            ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.7")

    #Make compiler messages nice with colored tags.
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.9")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
endif()

#if user does not specify prefix we assign it to the exec directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   message(STATUS "Install Prefix not specified.")
   file(MAKE_DIRECTORY install)
   get_filename_component(INSTALL_DIR ${CMAKE_SOURCE_DIR}/install REALPATH)
   set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH "Install Prefix" FORCE)
endif()
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

#Define the default build type to be Release
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Build type, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
   message(STATUS "Build type not defined, using default: ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#Add additional CXX flags if we use the Debug option
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

#------------------------------------------------------------------------------

#Install options
option(BUILD_TOOLS "Build and install tool programs." OFF)
option(INSTALL_CONFIG "Copy default configuration files." OFF)
option(USE_NCURSES "Use ncurses for terminal" ON)
mark_as_advanced(USE_NCURSES)

#------------------------------------------------------------------------------

#Find required packages.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

#Find rcbuild executable (https://github.com/cthornsb/devTools)
find_program(RCBUILD_EXECUTABLE rcbuild /opt/devTools/install/bin)
if("${RCBUILD_EXECUTABLE}" STREQUAL "RCBUILD_EXE-NOTFOUND")
	message(FATAL_ERROR "Could NOT find rcbuild")
else()
	message(STATUS "Searching for rcbuild: ${RCBUILD_EXE}")
endif()

#Find ROOT install.
find_package (ROOT REQUIRED)
mark_as_advanced(FORCE GENREFLEX_EXECUTABLE ROOTCINT_EXECUTABLE ROOT_CONFIG_EXECUTABLE)
include_directories(${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})

#Find curses library used for Scan library
if(USE_NCURSES)
	find_package(Curses REQUIRED)
endif()
if (CURSES_FOUND)
	add_definitions("-D USE_NCURSES")
	mark_as_advanced(FORCE CURSES_HAVE_CURSES_H CURSES_CURSES_H_PATH CURSES_FORM_LIBRARY)
else()
	message(STATUS "Curses unavailable, basic terminal will be used.")
	set(USE_NCURSES OFF)
endif()

set(TOP_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(DICTIONARY_PREFIX "RootDict" CACHE STRING "Prefix to root dictionary.")
set(DEF_STRUCT_FILE "${TOP_DIRECTORY}/dict/def.struct" CACHE PATH "Root structure definition file.")

#Add the core components directory.
add_subdirectory(core)

#Add the scan controller directory.
add_subdirectory(scan)

#Add the root dictionary directory.
add_subdirectory(dict)

#Add the simpleScan source directories.
include_directories(include)
add_subdirectory(source)

set(CONFIG_INSTALL_DIR "${TOP_DIRECTORY}/config" CACHE PATH "Path to configuration file install directory.")

if(INSTALL_CONFIG)
	set(DEFAULT_CONFIG_DIR "${TOP_DIRECTORY}/config")
	set(DEFAULT_CONFIG_FILES "config.dat" "energy.cal" "map.dat" "position.cal" "time.cal")
	
	#Install default config files.
	foreach(configFile ${DEFAULT_CONFIG_FILES})
		add_custom_target(${configFile} ALL DEPENDS ${CONFIG_INSTALL_DIR}/${configFile})
		add_custom_command(OUTPUT ${CONFIG_INSTALL_DIR}/${configFile} COMMAND mkdir -p ${CONFIG_INSTALL_DIR} && cp ${DEFAULT_CONFIG_DIR}/${configFile} ${CONFIG_INSTALL_DIR}/ DEPENDS ${DEFAULT_CONFIG_DIR}/${configFile} VERBATIM)
	endforeach(configFile)
endif()

if(BUILD_TOOLS)
	add_subdirectory(tools)
endif()

#Build/install the miscellaneous stuff
add_subdirectory(share)
