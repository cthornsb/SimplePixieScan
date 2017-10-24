#Build rcbuild executable.
add_executable(rcbuild rcbuild.cpp)
target_link_libraries(rcbuild OptionStatic)
install(TARGETS rcbuild DESTINATION bin)

if(${REMOVE_REGISTER})
	#Build unregister tool.
	add_executable(unregister unregister.cpp)
endif(${REMOVE_REGISTER})
