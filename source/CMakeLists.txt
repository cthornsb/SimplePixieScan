#Set the scan sources that we will make a lib out of.
set(SimpleCoreSources ColorTerm.cpp Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp ConfigFile.cpp MapFile.cpp)

set(ProcessorSources TriggerProcessor.cpp PhoswichProcessor.cpp LiquidProcessor.cpp LiquidBarProcessor.cpp
    HagridProcessor.cpp GenericProcessor.cpp GenericBarProcessor.cpp LogicProcessor.cpp TraceProcessor.cpp
    PSPmtProcessor.cpp)

#Add the sources to the library.
add_library(SimpleCoreObjects OBJECT ${SimpleCoreSources})
add_library(ProcessorObjects OBJECT ${ProcessorSources})

#Generate a static library.
add_library(SimpleScanStatic STATIC $<TARGET_OBJECTS:SimpleCoreObjects> $<TARGET_OBJECTS:ProcessorObjects>)

target_link_libraries(SimpleScanStatic ScanStatic)

#Build simpleScan executable.
add_executable(simpleScan Scanner.cpp)
target_link_libraries(simpleScan SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
install(TARGETS simpleScan DESTINATION bin)
