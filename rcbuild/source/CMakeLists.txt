#Build rcbuild executable.
add_executable(rcbuild rcbuild.cpp)
target_link_libraries(rcbuild OptionStatic)
install(TARGETS rcbuild DESTINATION bin)

#Build unregister tool.
add_executable(unregister unregister.cpp)
