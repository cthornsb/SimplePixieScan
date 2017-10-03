add_library(Scan SHARED IMPORTED)
set_property(TARGET Scan PROPERTY IMPORTED_LOCATION ${PAASS_LIB_SCAN})

add_library(GuiObj OBJECT simpleGui.cpp)
add_library(GuiStatic STATIC $<TARGET_OBJECTS:GuiObj>)

add_library(ToolObj OBJECT cmcalc.cpp CalibFile.cpp simpleTool.cpp)
add_library(ToolStatic STATIC $<TARGET_OBJECTS:ToolObj>)

#SimpleScan tools.
add_executable(calibrate calibrate.cpp)
target_link_libraries(calibrate ToolStatic Scan ${ROOT_LIBRARIES})

add_executable(mapReader mapReader.cpp)
target_link_libraries(mapReader ToolStatic Scan ${ROOT_LIBRARIES})

add_executable(rawEventAnalyzer rawEventAnalyzer.cpp)
target_link_libraries(rawEventAnalyzer ToolStatic Scan ${ROOT_LIBRARIES})

add_executable(specFitter specFitter.cpp)
target_link_libraries(specFitter ToolStatic Scan GuiStatic ${ROOT_GUI_LIBRARIES} -lFitPanel)

add_executable(timeAlign timeAlign.cpp)
target_link_libraries(timeAlign ToolStatic Scan ${ROOT_LIBRARIES} -lSpectrum)

#Tools requiring linking against simpleScan root dictionary.
add_executable(cmbinner cmbinner.cpp)
target_link_libraries(cmbinner ToolStatic Scan SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(barifier barifier.cpp)
target_link_libraries(barifier ToolStatic Scan SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(phasePhase phasePhase.cpp)
target_link_libraries(phasePhase ToolStatic Scan ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(tracer tracer.cpp)
target_link_libraries(tracer ToolStatic Scan ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(instantTime instantTime.cpp)
target_link_libraries(instantTime ToolStatic Scan ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

install(TARGETS calibrate cmbinner barifier mapReader phasePhase rawEventAnalyzer specFitter timeAlign tracer instantTime DESTINATION bin)
