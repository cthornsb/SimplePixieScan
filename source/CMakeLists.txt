#Set the scan sources that we will make a lib out of.
set(CoreSources ColorTerm.cpp Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp ConfigFile.cpp MapFile.cpp CalibFile.cpp)

set(ProcessorSources TriggerProcessor.cpp VandleProcessor.cpp PhoswichProcessor.cpp LiquidBarProcessor.cpp LiquidProcessor.cpp
                     HagridProcessor.cpp GenericProcessor.cpp GenericBarProcessor.cpp LogicProcessor.cpp TraceProcessor.cpp)

add_library(Scan SHARED IMPORTED)
set_property(TARGET Scan PROPERTY IMPORTED_LOCATION ${PAASS_LIB_SCAN})

#Add the sources to the library.
add_library(CoreObjects OBJECT ${CoreSources})
add_library(ProcessorObjects OBJECT ${ProcessorSources})

#Generate a static library.
add_library(SimpleScanStatic STATIC $<TARGET_OBJECTS:CoreObjects> $<TARGET_OBJECTS:ProcessorObjects>)

target_link_libraries(SimpleScanStatic Scan)

#Build simpleScan executable.
add_executable(simpleScan Scanner.cpp)
target_link_libraries(simpleScan SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
install(TARGETS simpleScan DESTINATION bin)
