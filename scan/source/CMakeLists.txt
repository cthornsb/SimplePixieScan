#Set the scan sources that we will make a lib out of
set(ScanSources ScanInterface.cpp Unpacker.cpp XiaData.cpp TraceFitter.cpp)

#Add the sources to the library
add_library(ScanObjects OBJECT ${ScanSources})

#Create ScanStatic library and add ncurses if we have it
add_library(ScanStatic STATIC $<TARGET_OBJECTS:ScanObjects>)
target_link_libraries(ScanStatic CoreStatic OptionStatic ${CMAKE_THREAD_LIBS_INIT})
if (${CURSES_FOUND})
    target_link_libraries(ScanStatic ${CURSES_LIBRARIES})
endif()
install(TARGETS ScanStatic DESTINATION lib)
