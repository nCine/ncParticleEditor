set(NCPROJECT_NAME "ncParticleEditor")
set(NCPROJECT_EXE_NAME "ncparticleeditor")
set(NCPROJECT_VENDOR "Angelo Theodorou")
set(NCPROJECT_COPYRIGHT "Copyright ©2018-2022 ${NCPROJECT_VENDOR}")
set(NCPROJECT_DESCRIPTION "A particle editor made with the nCine")
set(NCPROJECT_HOMEPAGE "https://ncine.github.io")
set(NCPROJECT_REVERSE_DNS "io.github.ncine.ncparticleeditor")

set(NCPROJECT_SOURCES
	src/particle_editor.h
	src/particle_editor.cpp
	src/particle_editor_gui_labels.h
	src/particle_editor_gui.cpp
	src/particle_editor_lua.h
	src/particle_editor_lua.cpp
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

function(callback_before_target)
	option(CUSTOM_WITH_FONTAWESOME "Download FontAwesome and include it in ImGui atlas" OFF)
	if(NCPROJECT_OPTIONS_PRESETS STREQUAL BinDist)
		set(CUSTOM_WITH_FONTAWESOME ON CACHE BOOL "Download FontAwesome and include it in ImGui atlas" FORCE)
	endif()
endfunction()

function(callback_after_target)
	if(MSVC)
		set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
		include(custom_crashrpt)
	endif()

	include(custom_iconfontcppheaders)
	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android" AND IS_DIRECTORY ${NCPROJECT_DATA_DIR})
		generate_textures_list()
		generate_scripts_list()

		include(custom_fontawesome)
		if(CUSTOM_WITH_FONTAWESOME)
			file(GLOB FONT_FILES "${NCPROJECT_DATA_DIR}/data/fonts/*.ttf")
		endif()
		if(EXISTS ${NCPROJECT_DATA_DIR}/data/config.lua)
			set(CONFIG_FILE ${NCPROJECT_DATA_DIR}/data/config.lua)
		endif()
		set(NCPROJECT_ANDROID_ASSETS ${CONFIG_FILE} ${FONT_FILES} ${SCRIPT_FILES} ${TEXTURE_FILES} CACHE STRING "" FORCE)
	endif()

	if(CMAKE_SYSTEM_NAME STREQUAL "Android" AND CUSTOM_WITH_FONTAWESOME)
		target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE "WITH_FONTAWESOME")
		target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE ${NCPROJECT_BINARY_DIR}/${ICONFONTCPPHEADERS_SOURCE_DIR_NAME})
	endif()

	# Needed to compile on Android
	set(GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

function(generate_textures_list)
	set(TEXTURES_DIR ${NCPROJECT_DATA_DIR}/data/textures)
	set(NUM_TEXTURES 0)
	if(IS_DIRECTORY ${TEXTURES_DIR})
		file(GLOB TEXTURE_FILES "${TEXTURES_DIR}/*.png" "${TEXTURES_DIR}/*.webp")
		list(LENGTH TEXTURE_FILES NUM_TEXTURES)
		set(TEXTURE_FILES ${TEXTURE_FILES} PARENT_SCOPE)
	endif()

	set(TEXTURES_H_FILE "${GENERATED_INCLUDE_DIR}/texture_strings.h")
	set(TEXTURES_CPP_FILE "${GENERATED_SOURCE_DIR}/texture_strings.cpp")

	get_filename_component(TEXTURES_H_FILENAME ${TEXTURES_H_FILE} NAME)
	file(WRITE ${TEXTURES_H_FILE} "#ifndef PROJECT_TEXTURE_STRINGS\n")
	file(APPEND ${TEXTURES_H_FILE} "#define PROJECT_TEXTURE_STRINGS\n\n")
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

	target_sources(${NCPROJECT_EXE_NAME} PRIVATE ${TEXTURES_H_FILE} ${TEXTURES_CPP_FILE})
	list(APPEND GENERATED_SOURCES ${TEXTURES_CPP_FILE})
	set(GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

function(generate_scripts_list)
	set(SCRIPTS_DIR ${NCPROJECT_DATA_DIR}/data/scripts)
	set(NUM_SCRIPTS 0)
	if(IS_DIRECTORY ${SCRIPTS_DIR})
		file(GLOB SCRIPT_FILES "${SCRIPTS_DIR}/*.lua")
		list(LENGTH SCRIPT_FILES NUM_SCRIPTS)
		set(SCRIPT_FILES ${SCRIPT_FILES} PARENT_SCOPE)
	endif()

	set(SCRIPTS_H_FILE "${GENERATED_INCLUDE_DIR}/script_strings.h")
	set(SCRIPTS_CPP_FILE "${GENERATED_SOURCE_DIR}/script_strings.cpp")

	get_filename_component(SCRIPTS_H_FILENAME ${SCRIPTS_H_FILE} NAME)
	file(WRITE ${SCRIPTS_H_FILE} "#ifndef PROJECT_SCRIPT_STRINGS\n")
	file(APPEND ${SCRIPTS_H_FILE} "#define PROJECT_SCRIPT_STRINGS\n\n")
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

	target_sources(${NCPROJECT_EXE_NAME} PRIVATE ${SCRIPTS_H_FILE} ${SCRIPTS_CPP_FILE})
	list(APPEND GENERATED_SOURCES ${SCRIPTS_CPP_FILE})
	set(GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

# Don't edit beyond this line
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	return()
endif()

cmake_minimum_required(VERSION 3.13)
project(${NCPROJECT_NAME})

find_path(NCPROJECT_DIR cmake/project_main.cmake
	PATHS ${nCine_DIR} $ENV{NCINE_DIR}
	PATH_SUFFIXES project ../project ../../../project ../../../share/ncine/project
	NO_CMAKE_FIND_ROOT_PATH # For Emscripten
	DOC "Path to the nCine template project scripts")

if(IS_DIRECTORY ${NCPROJECT_DIR})
	include(${NCPROJECT_DIR}/cmake/project_main.cmake)
else()
	if(NOT DEFINED nCine_DIR AND "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "Set the nCine_DIR CMake variable or the NCINE_DIR environment variable to a valid nCine directory.")
	elseif(DEFINED nCine_DIR)
		message(FATAL_ERROR "The nCine_DIR CMake variable \"${nCine_DIR}\" is not a valid nCine directory.")
	elseif(NOT "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "The NCINE_DIR environment variable \"$ENV{NCINE_DIR}\" is not a valid nCine directory.")
	endif()
endif()
