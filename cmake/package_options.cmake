option(PACKAGE_BUILD_ANDROID "Build the Android version of the game" OFF)

set(PACKAGE_DATA_DIR "${PARENT_SOURCE_DIR}/${PACKAGE_NAME}-data" CACHE PATH "Set the path to the game data directory")

if(PACKAGE_BUILD_ANDROID)
	set(NDK_DIR "" CACHE PATH "Set the path to the Android NDK")
	set(PACKAGE_NDK_ARCHITECTURES armeabi-v7a CACHE STRING "Set the NDK target architectures")
	option(PACKAGE_ASSEMBLE_APK "Assemble the Android APK of the game with Gradle" OFF)
endif()

# Package options presets
set(PACKAGE_OPTIONS_PRESETS "Default" CACHE STRING "Presets for CMake options")
set_property(CACHE PACKAGE_OPTIONS_PRESETS PROPERTY STRINGS Default BinDist)

if(PACKAGE_OPTIONS_PRESETS STREQUAL BinDist)
	message(STATUS "Options presets: ${PACKAGE_OPTIONS_PRESETS}")

	set(CMAKE_BUILD_TYPE Release)
	set(CMAKE_CONFIGURATION_TYPES Release)
	set(DEFAULT_DATA_DIR_DIST ON)
	set(PACKAGE_BUILD_ANDROID OFF)
endif()
