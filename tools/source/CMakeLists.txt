
add_library(ToolObj OBJECT cmcalc.cpp CalibFile.cpp simpleTool.cpp)
add_library(ToolStatic STATIC $<TARGET_OBJECTS:ToolObj>)
target_link_libraries(ToolStatic OptionStatic ScanStatic)

if(${BUILD_SHARED} OR ${BUILD_TOOLS_SPECFITTER})
	add_library(GuiObj OBJECT simpleGui.cpp)
	add_library(GuiStatic STATIC $<TARGET_OBJECTS:GuiObj>)
endif()

# Build shared libs
if(${BUILD_SHARED})
	add_library(SimpleTool SHARED $<TARGET_OBJECTS:ToolObj>)
	add_library(SimpleGui SHARED $<TARGET_OBJECTS:GuiObj>)
	install(TARGETS SimpleTool SimpleGui DESTINATION lib)
endif(${BUILD_SHARED})

#SimpleScan tools.
if(${BUILD_TOOLS_CALIBRATE})
	add_executable(calibrate calibrate.cpp)
	target_link_libraries(calibrate ToolStatic ${ROOT_LIBRARIES})
	install(TARGETS calibrate DESTINATION bin)
endif()

if(${BUILD_TOOLS_MAPREADER})
	add_executable(mapReader mapReader.cpp)
	target_link_libraries(mapReader ToolStatic ${ROOT_LIBRARIES})
	install(TARGETS mapReader DESTINATION bin)
endif()

if(${BUILD_TOOLS_RAWEVENT})
	add_executable(rawEventAnalyzer rawEventAnalyzer.cpp)
	target_link_libraries(rawEventAnalyzer ToolStatic ${ROOT_LIBRARIES})
	install(TARGETS rawEventAnalyzer DESTINATION bin)
endif()

if(${BUILD_TOOLS_SPECFITTER})
	add_executable(specFitter specFitter.cpp)
	target_link_libraries(specFitter ToolStatic GuiStatic ${ROOT_GUI_LIBRARIES})
	install(TARGETS specFitter DESTINATION bin)
endif()

if(${BUILD_TOOLS_TIMEALIGN})
	add_executable(timeAlign timeAlign.cpp)
	target_link_libraries(timeAlign ToolStatic ${ROOT_LIBRARIES} -lSpectrum)
	install(TARGETS timeAlign DESTINATION bin)
endif()

if(${BUILD_TOOLS_ANGLEANALYZER})
	add_executable(angleAnalyzer angleAnalyzer.cpp)
	target_link_libraries(angleAnalyzer ToolStatic ${ROOT_LIBRARIES})
	install(TARGETS angleAnalzyer DESTINATION bin)
endif()

if(${BUILD_TOOLS_CHISQUARE})
	add_library(ChiObj OBJECT simpleChisquare.cpp)
	add_library(ChiStatic STATIC $<TARGET_OBJECTS:ChiObj>)
	# Build shared libs
	if(${BUILD_SHARED})
		add_library(SimpleChi SHARED $<TARGET_OBJECTS:ChiObj>)
		install(TARGETS SimpleChi DESTINATION lib)
	endif(${BUILD_SHARED})
	add_executable(chisquare chisquare.cpp)
	target_link_libraries(chisquare ChiStatic ToolStatic ${ROOT_LIBRARIES})
	install(TARGETS chisquare DESTINATION bin)
endif()

#Tools requiring linking against simpleScan root dictionary.
if(${BUILD_TOOLS_CMBINNER})
	add_executable(cmbinner cmbinner.cpp)
	target_link_libraries(cmbinner ToolStatic SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS cmbinner DESTINATION bin)
endif()

if(${BUILD_TOOLS_BARIFIER})
	add_executable(barifier barifier.cpp)
	target_link_libraries(barifier ToolStatic SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS barifier DESTINATION bin)
endif()

if(${BUILD_TOOLS_PHASEPHASE})
	add_executable(phasePhase phasePhase.cpp)
	target_link_libraries(phasePhase ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS phasePhase DESTINATION bin)
endif()

if(${BUILD_TOOLS_TRACER})
	add_executable(tracer tracer.cpp)
	target_link_libraries(tracer ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS tracer DESTINATION bin)
endif()

if(${BUILD_TOOLS_INSTANTTIME})
	add_executable(instantTime instantTime.cpp)
	target_link_libraries(instantTime ToolStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS instantTime DESTINATION bin)
endif()

if(${BUILD_TOOLS_PSPMT})
	add_executable(pspmt pspmt.cpp)
	target_link_libraries(pspmt ToolStatic SimpleScanStatic ${DICTIONARY_PREFIX}Static ${ROOT_LIBRARIES})
	install(TARGETS pspmt DESTINATION bin)
endif()

