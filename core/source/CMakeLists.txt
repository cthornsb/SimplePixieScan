set(CoreSources Display.cpp hribf_buffers.cpp poll2_socket.cpp )

if (${CURSES_FOUND})
	list(APPEND CoreSources CTerminal.cpp)
endif()

add_library(CoreObjects OBJECT ${CoreSources})
add_library(CoreStatic STATIC $<TARGET_OBJECTS:CoreObjects>)
if (${CURSES_FOUND})
	target_link_libraries(CoreStatic ${CURSES_LIBRARIES})
endif()
install(TARGETS CoreStatic DESTINATION lib)
