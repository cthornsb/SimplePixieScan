set(CoreSources helperFunctions.cpp Display.cpp hribf_buffers.cpp poll2_socket.cpp)

if (${CURSES_FOUND})
	list(APPEND CoreSources CTerminal.cpp)
endif()

# Build static libs
add_library(CoreObjects OBJECT ${CoreSources})
add_library(CoreStatic STATIC $<TARGET_OBJECTS:CoreObjects>)
if (${CURSES_FOUND})
	target_link_libraries(CoreStatic ${CURSES_LIBRARIES})
endif()
add_library(OptionObjects OBJECT optionHandler.cpp)
add_library(OptionStatic STATIC $<TARGET_OBJECTS:OptionObjects>)

# Build shared libs
if(${BUILD_SHARED})
	add_library(SimpleCore SHARED $<TARGET_OBJECTS:CoreObjects>)
	target_link_libraries(SimpleCore ${CURSES_LIBRARIES})
	add_library(SimpleOption SHARED $<TARGET_OBJECTS:OptionObjects>)
	install(TARGETS SimpleCore SimpleOption DESTINATION lib)
endif(${BUILD_SHARED})
