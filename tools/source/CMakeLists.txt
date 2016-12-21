add_library(MainObj OBJECT main.cpp)
add_library(MainToolStatic STATIC $<TARGET_OBJECTS:MainObj>)
target_link_libraries(MainToolStatic)

add_executable(timeAlign timeAlign.cpp)
target_link_libraries(timeAlign MainToolStatic ${ROOT_LIBRARIES})

add_executable(calibrate calibrate.cpp)
target_link_libraries(calibrate MainToolStatic ${ROOT_LIBRARIES})

install(TARGETS timeAlign calibrate DESTINATION bin)
