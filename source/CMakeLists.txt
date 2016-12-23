#Set the scan sources that we will make a lib out of.
set(CoreSources Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp ConfigFile.cpp MapFile.cpp CalibFile.cpp)

set(ProcessorSources TriggerProcessor.cpp VandleProcessor.cpp PhoswichProcessor.cpp LiquidBarProcessor.cpp LiquidProcessor.cpp
                     HagridProcessor.cpp GenericProcessor.cpp LogicProcessor.cpp TraceProcessor.cpp)

add_library(ScanStatic STATIC IMPORTED)
set_property(TARGET ScanStatic PROPERTY IMPORTED_LOCATION ${PAASS_LIB_SCAN_STATIC})

add_library(PixieStatic STATIC IMPORTED)
set_property(TARGET PixieStatic PROPERTY IMPORTED_LOCATION "${PAASS_LIBRARY_DIR}/libPixieCoreStatic.a")

#Add the sources to the library.
add_library(CoreObjects OBJECT ${CoreSources})
add_library(ProcessorObjects OBJECT ${ProcessorSources})

#Generate a static library.
add_library(SimpleScanStatic STATIC $<TARGET_OBJECTS:CoreObjects> $<TARGET_OBJECTS:ProcessorObjects>)

target_link_libraries(SimpleScanStatic ScanStatic PixieStatic "/usr/lib/x86_64-linux-gnu/libcurses.so")

#Build simpleScan executable.
add_executable(simpleScan Scanner.cpp)
target_link_libraries(simpleScan SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
install(TARGETS simpleScan DESTINATION bin)
