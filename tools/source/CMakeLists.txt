add_library(GuiObj OBJECT simpleGui.cpp)
add_library(GuiStatic STATIC $<TARGET_OBJECTS:GuiObj>)

add_library(ToolObj OBJECT cmcalc.cpp CalibFile.cpp simpleTool.cpp)
add_library(ToolStatic STATIC $<TARGET_OBJECTS:ToolObj>)
target_link_libraries(ToolStatic OptionStatic ScanStatic)

#SimpleScan tools.
add_executable(calibrate calibrate.cpp)
target_link_libraries(calibrate ToolStatic ${ROOT_LIBRARIES})

add_executable(mapReader mapReader.cpp)
target_link_libraries(mapReader ToolStatic ${ROOT_LIBRARIES})

add_executable(rawEventAnalyzer rawEventAnalyzer.cpp)
target_link_libraries(rawEventAnalyzer ToolStatic ${ROOT_LIBRARIES})

add_executable(specFitter specFitter.cpp)
target_link_libraries(specFitter ToolStatic GuiStatic ${ROOT_GUI_LIBRARIES} -lFitPanel)

add_executable(timeAlign timeAlign.cpp)
target_link_libraries(timeAlign ToolStatic ${ROOT_LIBRARIES} -lSpectrum)

add_executable(angleAnalyzer angleAnalyzer.cpp)
target_link_libraries(angleAnalyzer ToolStatic ${ROOT_LIBRARIES})

#Tools requiring linking against simpleScan root dictionary.
add_executable(cmbinner cmbinner.cpp)
target_link_libraries(cmbinner ToolStatic SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(barifier barifier.cpp)
target_link_libraries(barifier ToolStatic SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(phasePhase phasePhase.cpp)
target_link_libraries(phasePhase ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(tracer tracer.cpp)
target_link_libraries(tracer ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

add_executable(instantTime instantTime.cpp)
target_link_libraries(instantTime ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})

install(TARGETS calibrate cmbinner barifier mapReader phasePhase rawEventAnalyzer specFitter timeAlign tracer instantTime angleAnalyzer DESTINATION bin)
