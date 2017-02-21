set(ROOT_DICT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(STRUCTURES_SOURCE "${ROOT_DICT_BUILD_DIR}/Structures.cpp")
set(STRUCTURES_HEADER "${ROOT_DICT_BUILD_DIR}/Structures.h")
set(LINKDEF_FILE "${ROOT_DICT_BUILD_DIR}/LinkDef.h")

set(COPIED_STRUCTURES_HEADER "${TOP_DIRECTORY}/include/Structures.h")

set(CINT_OUTPUT_FILE "${ROOT_DICT_BUILD_DIR}/${DICTIONARY_PREFIX}.cpp")
set(CINT_PCM_FILE "${ROOT_DICT_BUILD_DIR}/${DICTIONARY_PREFIX}_rdict.pcm")

add_custom_target(GenerateDict ALL DEPENDS ${STRUCTURES_SOURCE} ${STRUCTURES_HEADER} ${COPIED_STRUCTURES_HEADER} ${LINKDEF_FILE} ${CINT_OUTPUT_FILE} ${CINT_PCM_FILE})

#Use rcbuild to generate data structure source files.
add_custom_command(OUTPUT ${STRUCTURES_SOURCE} ${STRUCTURES_HEADER} ${LINKDEF_FILE} COMMAND ${RCBUILD_EXECUTABLE} -i ${DEF_STRUCT_FILE} -d ${ROOT_DICT_BUILD_DIR} DEPENDS ${DEF_STRUCT_FILE} VERBATIM)

#Install the new Structures file.
install(FILES ${STRUCTURES_HEADER} DESTINATION include)

#Copy the new Structures file to the top-level include directory.
add_custom_command(OUTPUT ${COPIED_STRUCTURES_HEADER} COMMAND cp ${STRUCTURES_HEADER} ${COPIED_STRUCTURES_HEADER} DEPENDS ${STRUCTURES_HEADER} VERBATIM)

#Use rootcint to generate dictionary source file.
add_custom_command(OUTPUT ${CINT_OUTPUT_FILE} ${CINT_PCM_FILE} COMMAND ${ROOTCINT_EXECUTABLE} -f ${CINT_OUTPUT_FILE} -c ${STRUCTURES_HEADER} ${LINKDEF_FILE} DEPENDS ${STRUCTURES_HEADER} ${LINKDEF_FILE} VERBATIM)

#Set the scan sources that we will make a lib out of.
set(DictSources ${STRUCTURES_SOURCE} ${CINT_OUTPUT_FILE})

#Add the sources to the library.
add_library(DictObjects OBJECT ${DictSources})

#Create shared library.
message(STATUS "Building Root Dictionary Shared Objects")
add_library(${DICTIONARY_PREFIX} SHARED $<TARGET_OBJECTS:DictObjects>)
install(TARGETS ${DICTIONARY_PREFIX} DESTINATION lib)

#Create static library.
add_library(${DICTIONARY_PREFIX}Static STATIC $<TARGET_OBJECTS:DictObjects>)
install(TARGETS ${DICTIONARY_PREFIX}Static DESTINATION lib)
