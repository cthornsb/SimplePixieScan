#Configure and install the module file
configure_file("modulefiles/simplePixieScan" simplePixieScan @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/simplePixieScan DESTINATION share/modulefiles/)
