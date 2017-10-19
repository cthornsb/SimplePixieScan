set(ROOT_DICT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(STRUCTURES_SOURCE "${ROOT_DICT_BUILD_DIR}/Structures.cpp")
set(STRUCTURES_HEADER "${ROOT_DICT_BUILD_DIR}/Structures.h")
set(LINKDEF_FILE "${ROOT_DICT_BUILD_DIR}/LinkDef.h")

set(COPIED_STRUCTURES_HEADER "${TOP_DIRECTORY}/include/Structures.h")

set(CINT_OUTPUT_FILE "${ROOT_DICT_BUILD_DIR}/${DICTIONARY_PREFIX}.cpp")
set(UNREG_OUTPUT_FILE "${ROOT_DICT_BUILD_DIR}/${DICTIONARY_PREFIX}_mod.cpp")

if(${REMOVE_REGISTER})
	add_custom_target(GenerateDict ALL DEPENDS ${STRUCTURES_SOURCE} ${STRUCTURES_HEADER} ${COPIED_STRUCTURES_HEADER} ${LINKDEF_FILE} ${UNREG_OUTPUT_FILE})
else()
	add_custom_target(GenerateDict ALL DEPENDS ${STRUCTURES_SOURCE} ${STRUCTURES_HEADER} ${COPIED_STRUCTURES_HEADER} ${LINKDEF_FILE} ${CINT_OUTPUT_FILE})
endif(${REMOVE_REGISTER})

#Use "rcbuild" to generate data structure source files.
add_custom_command(OUTPUT ${STRUCTURES_SOURCE} ${STRUCTURES_HEADER} ${LINKDEF_FILE} COMMAND rcbuild -i ${DEF_STRUCT_FILE} -d ${ROOT_DICT_BUILD_DIR} DEPENDS ${DEF_STRUCT_FILE} VERBATIM)

if(${BUILD_SHARED})
	#Install the new Structures file.
	install(FILES ${STRUCTURES_HEADER} DESTINATION include)
endif(${BUILD_SHARED})

#Copy the new Structures file to the top-level include directory.
add_custom_command(OUTPUT ${COPIED_STRUCTURES_HEADER} COMMAND cp ${STRUCTURES_HEADER} ${COPIED_STRUCTURES_HEADER} DEPENDS ${STRUCTURES_HEADER} VERBATIM)

#Use rootcint to generate dictionary source file.
add_custom_command(OUTPUT ${CINT_OUTPUT_FILE} COMMAND ${ROOTCINT_EXECUTABLE} -f ${CINT_OUTPUT_FILE} -c ${STRUCTURES_HEADER} ${LINKDEF_FILE} DEPENDS ${STRUCTURES_HEADER} ${LINKDEF_FILE} VERBATIM)

if(${REMOVE_REGISTER})
	#Use "unregister" to remove the "Error <TCling::RegisterModule>" error message.
	add_custom_command(OUTPUT ${UNREG_OUTPUT_FILE} COMMAND unregister ${CINT_OUTPUT_FILE} DEPENDS ${CINT_OUTPUT_FILE} VERBATIM)

	#Disable unused variable compiler warnings.
	# This is necessary because we removed part of the code, so some of
	# the variables are now unused and the compiler will likely complain.
	add_definitions("-Wno-unused-variable")

	#Add the sources to the library.
	add_library(DictObjects OBJECT ${STRUCTURES_SOURCE} ${UNREG_OUTPUT_FILE})
else()
	#Add the sources to the library.
	add_library(DictObjects OBJECT ${STRUCTURES_SOURCE} ${CINT_OUTPUT_FILE})
endif(${REMOVE_REGISTER})

#Create static library.
add_library(${DICTIONARY_PREFIX}Static STATIC $<TARGET_OBJECTS:DictObjects>)

if(${BUILD_SHARED})
	#Create shared library for the dictionary.
	add_library(${DICTIONARY_PREFIX} SHARED $<TARGET_OBJECTS:DictObjects>)
	install(TARGETS ${DICTIONARY_PREFIX} DESTINATION lib)
endif(${BUILD_SHARED})
