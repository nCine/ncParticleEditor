set(PACKAGE_NAME "ncParticleEditor")
set(PACKAGE_EXE_NAME "ncparticleeditor")
set(PACKAGE_VENDOR "Angelo Theodorou")
set(PACKAGE_COPYRIGHT "Copyright ©2018-2019 ${PACKAGE_VENDOR}")
set(PACKAGE_DESCRIPTION "A particle editor made with the nCine")
set(PACKAGE_HOMEPAGE "https://ncine.github.io")
set(PACKAGE_REVERSE_DNS "io.github.ncine.ncparticleeditor")

set(PACKAGE_SOURCES
	src/particle_editor.h
	src/particle_editor.cpp
	src/particle_editor_gui.cpp
	src/particle_editor_lua.h
	src/particle_editor_lua.cpp
)

function(callback_after_target)
	if(MSVC)
		set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
		include(package_crashrpt)
	endif()

	generate_textures_list()
	generate_scripts_list()
endfunction()

function(generate_textures_list)
	set(TEXTURES_DIR ${PACKAGE_DATA_DIR}/data/textures)
	set(NUM_TEXTURES 0)
	if(IS_DIRECTORY ${TEXTURES_DIR})
		file(GLOB TEXTURE_FILES "${TEXTURES_DIR}/*.png" "${TEXTURES_DIR}/*.webp")
		list(LENGTH TEXTURE_FILES NUM_TEXTURES)
	endif()

	set(TEXTURES_H_FILE "${GENERATED_INCLUDE_DIR}/texture_strings.h")
	set(TEXTURES_CPP_FILE "${GENERATED_SOURCE_DIR}/texture_strings.cpp")

	get_filename_component(TEXTURES_H_FILENAME ${TEXTURES_H_FILE} NAME)
	file(WRITE ${TEXTURES_H_FILE} "#ifndef PACKAGE_TEXTURE_STRINGS\n")
	file(APPEND ${TEXTURES_H_FILE} "#define PACKAGE_TEXTURE_STRINGS\n\n")
	file(APPEND ${TEXTURES_H_FILE} "struct TextureStrings\n{\n")
	file(APPEND ${TEXTURES_H_FILE} "\tstatic const int Count = ${NUM_TEXTURES};\n")
	file(APPEND ${TEXTURES_H_FILE} "\tstatic char const * const Names[Count];\n};\n\n")
	file(APPEND ${TEXTURES_H_FILE} "#endif\n")

	file(WRITE ${TEXTURES_CPP_FILE} "#include \"${TEXTURES_H_FILENAME}\"\n\n")
	file(APPEND ${TEXTURES_CPP_FILE} "char const * const TextureStrings::Names[TextureStrings::Count] =\n")
	file(APPEND ${TEXTURES_CPP_FILE} "{\n")
	foreach(TEXTURE_FILE ${TEXTURE_FILES})
		get_filename_component(TEXTURE_FILENAME ${TEXTURE_FILE} NAME)
		file(APPEND ${TEXTURES_CPP_FILE} "\t\"${TEXTURE_FILENAME}\",\n")
	endforeach()
	file(APPEND ${TEXTURES_CPP_FILE} "};\n")

	target_sources(${PACKAGE_EXE_NAME} PRIVATE ${TEXTURES_H_FILE} ${TEXTURES_CPP_FILE})
endfunction()

function(generate_scripts_list)
	set(SCRIPTS_DIR ${PACKAGE_DATA_DIR}/data/scripts)
	set(NUM_SCRIPTS 0)
	if(IS_DIRECTORY ${SCRIPTS_DIR})
		file(GLOB SCRIPT_FILES "${SCRIPTS_DIR}/*.lua")
		list(LENGTH SCRIPT_FILES NUM_SCRIPTS)
	endif()

	set(SCRIPTS_H_FILE "${GENERATED_INCLUDE_DIR}/script_strings.h")
	set(SCRIPTS_CPP_FILE "${GENERATED_SOURCE_DIR}/script_strings.cpp")

	get_filename_component(SCRIPTS_H_FILENAME ${SCRIPTS_H_FILE} NAME)
	file(WRITE ${SCRIPTS_H_FILE} "#ifndef PACKAGE_SCRIPT_STRINGS\n")
	file(APPEND ${SCRIPTS_H_FILE} "#define PACKAGE_SCRIPT_STRINGS\n\n")
	file(APPEND ${SCRIPTS_H_FILE} "struct ScriptStrings\n{\n")
	file(APPEND ${SCRIPTS_H_FILE} "\tstatic const int Count = ${NUM_SCRIPTS};\n")
	file(APPEND ${SCRIPTS_H_FILE} "\tstatic char const * const Names[Count];\n};\n\n")
	file(APPEND ${SCRIPTS_H_FILE} "#endif\n")

	file(WRITE ${SCRIPTS_CPP_FILE} "#include \"${SCRIPTS_H_FILENAME}\"\n\n")
	file(APPEND ${SCRIPTS_CPP_FILE} "char const * const ScriptStrings::Names[ScriptStrings::Count] =\n")
	file(APPEND ${SCRIPTS_CPP_FILE} "{\n")
	foreach(SCRIPT_FILE ${SCRIPT_FILES})
		get_filename_component(SCRIPT_FILENAME ${SCRIPT_FILE} NAME)
		file(APPEND ${SCRIPTS_CPP_FILE} "\t\"${SCRIPT_FILENAME}\",\n")
	endforeach()
	file(APPEND ${SCRIPTS_CPP_FILE} "};\n")

	target_sources(${PACKAGE_EXE_NAME} PRIVATE ${SCRIPTS_H_FILE} ${SCRIPTS_CPP_FILE})
endfunction()
