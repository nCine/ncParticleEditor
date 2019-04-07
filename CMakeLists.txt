cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
get_filename_component(PARENT_SOURCE_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_BINARY_DIR ${CMAKE_BINARY_DIR} DIRECTORY)

include(package_info)
include(package_check_info)

project(${PACKAGE_NAME})

include(package_options)

include(find_ncine)

if(NOT IS_DIRECTORY ${PACKAGE_DATA_DIR})
	message(WARNING "${PACKAGE_NAME} data directory not found at: ${PACKAGE_DATA_DIR}")
else()
	message(STATUS "${PACKAGE_NAME} data directory: ${PACKAGE_DATA_DIR}")
endif()

include(package_get_version)
include(package_installation)

add_executable(${PACKAGE_EXE_NAME} ${PACKAGE_SOURCES})

if(MSVC AND EXISTS ${PACKAGE_DATA_DIR}/icons/${PACKAGE_ICON_NAME}.ico)
	message(STATUS "Writing a resource file for executable icon")

	set(GENERATED_SOURCE_DIR "${CMAKE_BINARY_DIR}/generated")
	set(GENERATED_INCLUDE_DIR "${GENERATED_SOURCE_DIR}/include")

	set(RESOURCE_RC_FILE "${GENERATED_SOURCE_DIR}/resource.rc")
	file(WRITE ${RESOURCE_RC_FILE} "IDI_ICON1 ICON DISCARDABLE \"${PACKAGE_ICON_NAME}.ico\"")
	file(COPY ${PACKAGE_DATA_DIR}/icons/${PACKAGE_ICON_NAME}.ico DESTINATION ${GENERATED_INCLUDE_DIR})
	target_sources(${PACKAGE_EXE_NAME} PRIVATE ${RESOURCE_RC_FILE})
endif()

if(MSVC AND PACKAGE_CRASHRPT)
	set(CRASHRPT_STRINGS_FILE ${CMAKE_BINARY_DIR}/generated/CrashRptStrings.cpp)
	file(WRITE ${CRASHRPT_STRINGS_FILE} "#include \"CrashRptWrapper.h\"\n\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::appName = \"${PACKAGE_NAME}\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::appVersion = \"${PACKAGE_VERSION}\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::emailSubject = \"${PACKAGE_NAME} ${PACKAGE_VERSION} Error Report\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::emailTo = \"${PACKAGE_AUTHOR_MAIL}\";\n")
	file(COPY CrashRptWrapper.h DESTINATION generated)
	# Source files can't vary by configuration with the Visual Studio generator, they are added even in release
	target_sources(${PACKAGE_EXE_NAME} PRIVATE CrashRptWrapper.h CrashRptWrapper.cpp ${CRASHRPT_STRINGS_FILE})
	set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
	set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY TRUE)
endif()

if(NOT DEFAULT_DATA_DIR_DIST) # Don't set the startup project if it wouldn't find the data directory
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PACKAGE_EXE_NAME})
endif()

if(MSVC AND EXISTS ${PACKAGE_DATA_DIR}/icons/${PACKAGE_ICON_NAME}.ico)
	target_include_directories(${PACKAGE_EXE_NAME} PRIVATE ${GENERATED_INCLUDE_DIR}) # for the executable icon
endif()
target_link_libraries(${PACKAGE_EXE_NAME} ncine::ncine_main ncine::ncine)

target_compile_definitions(${PACKAGE_EXE_NAME} PRIVATE "$<$<CONFIG:Debug>:${PACKAGE_UPPER_NAME}_DEBUG>")

if(MSVC)
	target_compile_options(${PACKAGE_EXE_NAME} PRIVATE /MP)
	include(package_crashrpt)
endif()

if(APPLE)
	file(RELATIVE_PATH RELPATH_TO_LIB ${CMAKE_INSTALL_PREFIX}/${RUNTIME_INSTALL_DESTINATION}/ ${CMAKE_INSTALL_PREFIX}/${LIBRARY_INSTALL_DESTINATION})
	set_target_properties(${PACKAGE_EXE_NAME} PROPERTIES INSTALL_RPATH "@executable_path/${RELPATH_TO_LIB}")
endif()

if(DEFAULT_DATA_DIR_DIST)
	if(MSVC OR APPLE)
		# Relative path from tests to data on Windows and OS X, where the user can choose the installation directory
		file(RELATIVE_PATH PACKAGE_DEFAULT_DATA_DIR
			${CMAKE_INSTALL_PREFIX}/${RUNTIME_INSTALL_DESTINATION}
			${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DESTINATION}) # Always strips trailing slash
		set(PACKAGE_DEFAULT_DATA_DIR "${PACKAGE_DEFAULT_DATA_DIR}/")
	else()
		set(PACKAGE_DEFAULT_DATA_DIR "${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DESTINATION}/")
	endif()
elseif(NOT PACKAGE_DEFAULT_DATA_DIR)
	set(PACKAGE_DEFAULT_DATA_DIR "${PACKAGE_DATA_DIR}/data/")
endif()

if(PACKAGE_DEFAULT_DATA_DIR)
	file(TO_CMAKE_PATH "${PACKAGE_DEFAULT_DATA_DIR}" PACKAGE_DEFAULT_DATA_DIR) # Always strips trailing slash
	set(PACKAGE_DEFAULT_DATA_DIR "${PACKAGE_DEFAULT_DATA_DIR}/")

	message(STATUS "Default data directory: ${PACKAGE_DEFAULT_DATA_DIR}")
	target_compile_definitions(${PACKAGE_EXE_NAME} PRIVATE "PACKAGE_DEFAULT_DATA_DIR=\"${PACKAGE_DEFAULT_DATA_DIR}\"")
endif()

install(TARGETS ${PACKAGE_EXE_NAME} RUNTIME DESTINATION ${RUNTIME_INSTALL_DESTINATION})
install(FILES README.md DESTINATION ${README_INSTALL_DESTINATION})
if(MSVC OR APPLE)
	install(FILES LICENSE DESTINATION . RENAME LICENSE.txt)
endif()
install(DIRECTORY ${PACKAGE_DATA_DIR}/data/ DESTINATION ${DATA_INSTALL_DESTINATION})
install(FILES ${PACKAGE_DATA_DIR}/README.md DESTINATION ${DATA_INSTALL_DESTINATION})
if(IS_DIRECTORY ${NCINE_SHADERS_DIR})
	install(DIRECTORY ${NCINE_SHADERS_DIR} DESTINATION ${SHADERS_INSTALL_DESTINATION})
endif()

if(MSVC)
	install(FILES ${NCINE_LOCATION} DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(DIRECTORY ${BINDIR}/ DESTINATION ${RUNTIME_INSTALL_DESTINATION} FILES_MATCHING PATTERN "*.dll")
elseif(APPLE)
	install(FILES ${NCINE_LOCATION} DESTINATION ${LIBRARY_INSTALL_DESTINATION})
	install(DIRECTORY ${FRAMEWORKS_DIR}/ DESTINATION ${FRAMEWORKS_INSTALL_DESTINATION})
endif()

include(package_copy_targets)
include(package_build_android)
include(package_strip_binaries)
