if(${BUILD_SHARED})
	# Copy header files to the install directory.
	install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
endif(${BUILD_SHARED})
add_subdirectory(source)
