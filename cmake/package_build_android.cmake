if(PACKAGE_BUILD_ANDROID)
	if(NOT IS_DIRECTORY ${NDK_DIR})
		unset(NDK_DIR CACHE)
		find_path(NDK_DIR
			NAMES ndk-build ndk-build.cmd ndk-gdb ndk-gdb.cmd ndk-stack ndk-stack.cmd ndk-which ndk-which.cmd
			PATHS $ENV{ANDROID_NDK_HOME} $ENV{ANDROID_NDK_ROOT} $ENV{ANDROID_NDK}
			DOC "Path to the Android NDK directory")

		if(NOT IS_DIRECTORY ${NDK_DIR})
			message(FATAL_ERROR "Cannot find the Android NDK directory")
		endif()
	endif()
	message(STATUS "Android NDK directory: ${NDK_DIR}")

	set(ANDROID_TOOLCHAIN clang)
	set(ANDROID_STL c++_shared)

	string(REPLACE "/" "." JAVA_PACKAGE "${PACKAGE_JAVA_URL}")
	set(ANDROID_MANIFEST_XML_IN ${CMAKE_SOURCE_DIR}/android/src/main/AndroidManifest.xml.in)
	set(ANDROID_MANIFEST_XML ${CMAKE_BINARY_DIR}/android/src/main/AndroidManifest.xml)
	configure_file(${ANDROID_MANIFEST_XML_IN} ${ANDROID_MANIFEST_XML} @ONLY)

	set(LOAD_LIBRARIES_JAVA_IN ${CMAKE_SOURCE_DIR}/android/src/main/java/LoadLibraries.java.in)
	set(LOAD_LIBRARIES_JAVA ${CMAKE_BINARY_DIR}/android/src/main/java/${PACKAGE_JAVA_URL}/LoadLibraries.java)
	configure_file(${LOAD_LIBRARIES_JAVA_IN} ${LOAD_LIBRARIES_JAVA} @ONLY)
	set(LOAD_LIBRARIES_TV_JAVA_IN ${CMAKE_SOURCE_DIR}/android/src/main/java/LoadLibrariesTV.java.in)
	set(LOAD_LIBRARIES_TV_JAVA ${CMAKE_BINARY_DIR}/android/src/main/java/${PACKAGE_JAVA_URL}/LoadLibrariesTV.java)
	configure_file(${LOAD_LIBRARIES_TV_JAVA_IN} ${LOAD_LIBRARIES_TV_JAVA} @ONLY)

	set(STRINGS_XML_IN ${CMAKE_SOURCE_DIR}/android/src/main/res/values/strings.xml.in)
	set(STRINGS_XML ${CMAKE_BINARY_DIR}/android/src/main/res/values/strings.xml)
	configure_file(${STRINGS_XML_IN} ${STRINGS_XML} @ONLY)

	foreach(INCLUDE_DIR ${PACKAGE_INCLUDE_DIRS})
		if(IS_ABSOLUTE ${INCLUDE_DIR})
			list(APPEND ANDROID_INCLUDE_DIRS "${INCLUDE_DIR}")
		else()
			list(APPEND ANDROID_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/${INCLUDE_DIR}")
		endif()
	endforeach()

	foreach(SOURCE ${PACKAGE_SOURCES})
		if(IS_ABSOLUTE ${SOURCE})
			list(APPEND ANDROID_PACKAGE_SOURCES "\t${SOURCE}")
		else()
			list(APPEND ANDROID_PACKAGE_SOURCES "\t${CMAKE_SOURCE_DIR}/${SOURCE}")
		endif()
	endforeach()
	string(REPLACE ";" "\n" ANDROID_PACKAGE_SOURCES "${ANDROID_PACKAGE_SOURCES}")
	set(CMAKELIST_TXT_IN ${CMAKE_SOURCE_DIR}/android/src/main/cpp/CMakeLists.txt.in)
	set(CMAKELIST_TXT ${CMAKE_BINARY_DIR}/android/src/main/cpp/CMakeLists.txt)
	configure_file(${CMAKELIST_TXT_IN} ${CMAKELIST_TXT} @ONLY)

	# Reset compilation flags that external tools might have set in environment variables
	set(RESET_FLAGS_ARGS -DCMAKE_C_FLAGS="" -DCMAKE_CXX_FLAGS="" -DCMAKE_EXE_LINKER_FLAGS=""
		-DCMAKE_MODULE_LINKER_FLAGS="" -DCMAKE_SHARED_LINKER_FLAGS="" -DCMAKE_STATIC_LINKER_FLAGS="")
	set(ANDROID_PASSTHROUGH_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
		-DNCINE_DYNAMIC_LIBRARY=${NCINE_DYNAMIC_LIBRARY} -DEXTERNAL_ANDROID_DIR=${NCINE_EXTERNAL_ANDROID_DIR}
		-DGENERATED_INCLUDE_DIR=${GENERATED_INCLUDE_DIR} -DPACKAGE_STRIP_BINARIES=${PACKAGE_STRIP_BINARIES})
	set(ANDROID_CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=${ANDROID_API_LEVEL}
		-DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=${ANDROID_TOOLCHAIN_VERSION} -DCMAKE_ANDROID_STL_TYPE=${ANDROID_STL_TYPE})
	set(ANDROID_ARM_ARGS -DCMAKE_ANDROID_ARM_MODE=ON -DCMAKE_ANDROID_ARM_NEON=ON)

	if(MSVC)
		list(APPEND ANDROID_CMAKE_ARGS "-GNMake Makefiles")
	else()
		list(APPEND ANDROID_CMAKE_ARGS "-G${CMAKE_GENERATOR}")
	endif()

	string(REPLACE ";" "', '" GRADLE_PASSTHROUGH_ARGS "${ANDROID_PASSTHROUGH_ARGS}")
	string(REPLACE ";" "', '" GRADLE_CMAKE_ARGS "${ANDROID_CMAKE_ARGS}")
	string(REPLACE ";" "', '" GRADLE_ARM_ARGS "${ANDROID_ARM_ARGS}")
	string(REPLACE ";" "', '" GRADLE_NDK_ARCHITECTURES "${PACKAGE_NDK_ARCHITECTURES}")

	# Added later to skip the string replace operation and keep them as lists in Gradle too
	list(APPEND ANDROID_PASSTHROUGH_ARGS -DGENERATED_SOURCES="${GENERATED_SOURCES}" -DANDROID_GENERATED_FLAGS="${ANDROID_GENERATED_FLAGS}")
	set(GRADLE_PASSTHROUGH_ARGS "${GRADLE_PASSTHROUGH_ARGS}', '-DGENERATED_SOURCES=${GENERATED_SOURCES}', '-DANDROID_GENERATED_FLAGS=${ANDROID_GENERATED_FLAGS}")
	# Not added to Gradle arguments as it is handled by substituting `GRADLE_NDK_DIR`
	list(APPEND ANDROID_CMAKE_ARGS -DCMAKE_ANDROID_NDK=${NDK_DIR})

	set(GRADLE_BUILDTOOLS_VERSION 28.0.3)
	set(GRADLE_COMPILESDK_VERSION 28)
	set(GRADLE_MINSDK_VERSION 21)
	set(GRADLE_TARGETSDK_VERSION 28)
	set(GRADLE_VERSIONCODE 1)

	set(GRADLE_LIBCPP_SHARED "false")
	if(ANDROID_STL STREQUAL "c++_shared")
		set(GRADLE_LIBCPP_SHARED "true")
	endif()

	set(BUILD_GRADLE_IN ${CMAKE_SOURCE_DIR}/android/build.gradle.in)
	set(BUILD_GRADLE ${CMAKE_BINARY_DIR}/android/build.gradle)
	set(GRADLE_JNILIBS_DIRS "'src/main/cpp/ncine', 'src/main/cpp/openal'")
	configure_file(${BUILD_GRADLE_IN} ${BUILD_GRADLE} @ONLY)
	set(GRADLE_PROPERTIES_IN ${CMAKE_SOURCE_DIR}/android/gradle.properties.in)
	set(GRADLE_PROPERTIES ${CMAKE_BINARY_DIR}/android/gradle.properties)
	set(GRADLE_CMAKE_COMMAND ${CMAKE_COMMAND})
	set(GRADLE_NDK_DIR ${NDK_DIR})
	configure_file(${GRADLE_PROPERTIES_IN} ${GRADLE_PROPERTIES} @ONLY)

	file(COPY ${PACKAGE_DATA_DIR}/icons/icon48.png DESTINATION android/src/main/res/mipmap-mdpi)
	file(RENAME ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-mdpi/icon48.png ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-mdpi/ic_launcher.png)
	file(COPY ${PACKAGE_DATA_DIR}/icons/icon72.png DESTINATION android/src/main/res/mipmap-hdpi)
	file(RENAME ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-hdpi/icon72.png ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-hdpi/ic_launcher.png)
	file(COPY ${PACKAGE_DATA_DIR}/icons/icon96.png DESTINATION android/src/main/res/mipmap-xhdpi)
	file(RENAME ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xhdpi/icon96.png ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xhdpi/ic_launcher.png)
	file(COPY ${PACKAGE_DATA_DIR}/icons/icon144.png DESTINATION android/src/main/res/mipmap-xxhdpi)
	file(RENAME ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xxhdpi/icon144.png ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xxhdpi/ic_launcher.png)
	file(COPY ${PACKAGE_DATA_DIR}/icons/icon192.png DESTINATION android/src/main/res/mipmap-xxxhdpi)
	file(RENAME ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xxxhdpi/icon192.png ${CMAKE_BINARY_DIR}/android/src/main/res/mipmap-xxxhdpi/ic_launcher.png)

	if(DEFINED PACKAGE_ANDROID_ASSETS)
		foreach(ASSET ${PACKAGE_ANDROID_ASSETS})
			if(IS_ABSOLUTE ${ASSET})
				file(COPY ${ASSET} DESTINATION android/src/main/assets/)
			else()
				file(COPY ${PACKAGE_DATA_DIR}/${ASSET} DESTINATION android/src/main/assets/)
			endif()
		endforeach()
	endif()

	find_path(NCINE_ANDROID_NCINE_LIB_DIR
		NAMES libncine.so libncine.a
		PATHS ${NCINE_ANDROID_DIR}/src/main/cpp/ncine ${NCINE_ANDROID_DIR}/build/ncine/
		PATH_SUFFIXES armeabi-v7a arm64-v8a x86_64
	)
	get_filename_component(NCINE_ANDROID_NCINE_LIB_DIR ${NCINE_ANDROID_NCINE_LIB_DIR} DIRECTORY)

	set(NCINE_ANDROID_LIBNAME libncine.so)
	if(NOT NCINE_DYNAMIC_LIBRARY)
		set(NCINE_ANDROID_LIBNAME libncine.a)
	endif()

	find_path(NCINE_ANDROID_OPENAL_LIB_DIR
		NAMES libopenal.so
		PATHS ${NCINE_ANDROID_DIR}/src/main/cpp/openal ${NCINE_EXTERNAL_ANDROID_DIR}/openal/
		PATH_SUFFIXES armeabi-v7a arm64-v8a x86_64
	)
	get_filename_component(NCINE_ANDROID_OPENAL_LIB_DIR ${NCINE_ANDROID_OPENAL_LIB_DIR} DIRECTORY)

	foreach(ARCHITECTURE ${PACKAGE_NDK_ARCHITECTURES})
		file(COPY ${NCINE_ANDROID_OPENAL_LIB_DIR}/${ARCHITECTURE}/libopenal.so DESTINATION android/src/main/cpp/openal/${ARCHITECTURE})
		file(COPY ${NCINE_ANDROID_NCINE_LIB_DIR}/${ARCHITECTURE}/${NCINE_ANDROID_LIBNAME} DESTINATION android/src/main/cpp/ncine/${ARCHITECTURE})
		file(COPY ${NCINE_ANDROID_NCINE_LIB_DIR}/${ARCHITECTURE}/libncine_main.a DESTINATION android/src/main/cpp/ncine/${ARCHITECTURE})
	endforeach()

	foreach(INCLUDE_DIR ${NCINE_INCLUDE_DIR})
		if(IS_DIRECTORY ${INCLUDE_DIR}/ncine)
			file(COPY ${INCLUDE_DIR}/ncine DESTINATION android/src/main/cpp/ncine/include)
		endif()
		if(IS_DIRECTORY ${INCLUDE_DIR}/nctl)
			file(COPY ${INCLUDE_DIR}/nctl DESTINATION android/src/main/cpp/ncine/include)
		endif()
	endforeach()

	add_custom_target(package_android ALL)
	set_target_properties(package_android PROPERTIES FOLDER "Android")
	add_dependencies(package_android ${PACKAGE_EXE_NAME})

	foreach(ARCHITECTURE ${PACKAGE_NDK_ARCHITECTURES})
		set(ANDROID_BINARY_DIR ${CMAKE_BINARY_DIR}/android/build/${PACKAGE_LOWER_NAME}/${ARCHITECTURE})
		set(ANDROID_ARCH_ARGS -DCMAKE_ANDROID_ARCH_ABI=${ARCHITECTURE})
		if(ARCHITECTURE STREQUAL "armeabi-v7a")
			list(APPEND ANDROID_ARCH_ARGS ${ANDROID_ARM_ARGS})
		endif()
		add_custom_command(OUTPUT ${ANDROID_BINARY_DIR}/libgame.so
			COMMAND ${CMAKE_COMMAND} -H${CMAKE_BINARY_DIR}/android/src/main/cpp/ -B${ANDROID_BINARY_DIR}
				-DCMAKE_TOOLCHAIN_FILE=${NDK_DIR}/build/cmake/android.toolchain.cmake
				-DANDROID_PLATFORM=android-${GRADLE_MINSDK_VERSION} -DANDROID_ABI=${ARCHITECTURE}
				${RESET_FLAGS_ARGS} ${ANDROID_PASSTHROUGH_ARGS} ${ANDROID_CMAKE_ARGS} ${ANDROID_ARCH_ARGS}
			COMMAND ${CMAKE_COMMAND} --build ${ANDROID_BINARY_DIR}
			COMMENT "Compiling the Android library for ${ARCHITECTURE}")
		add_custom_target(package_android_${ARCHITECTURE} DEPENDS ${ANDROID_BINARY_DIR}/libgame.so)
		set_target_properties(package_android_${ARCHITECTURE} PROPERTIES FOLDER "Android")
		add_dependencies(package_android package_android_${ARCHITECTURE})
	endforeach()

	if(PACKAGE_ASSEMBLE_APK)
		find_program(GRADLE_EXECUTABLE gradle $ENV{GRADLE_HOME}/bin)
		if(GRADLE_EXECUTABLE)
			message(STATUS "Gradle executable: ${GRADLE_EXECUTABLE}")

			if(CMAKE_BUILD_TYPE MATCHES "Release")
				set(GRADLE_TASK assembleRelease)
				set(APK_BUILDTYPE_DIR release)
				set(APK_FILE_SUFFIX release-unsigned)
			else()
				set(GRADLE_TASK assembleDebug)
				set(APK_BUILDTYPE_DIR debug)
				set(APK_FILE_SUFFIX debug)
			endif()

			foreach(ARCHITECTURE ${PACKAGE_NDK_ARCHITECTURES})
				list(APPEND APK_FILES ${CMAKE_BINARY_DIR}/android/build/outputs/apk/${APK_BUILDTYPE_DIR}/android-${ARCHITECTURE}-${APK_FILE_SUFFIX}.apk)
			endforeach()

			# Invoking Gradle to create the Android APK of the game
			add_custom_command(OUTPUT ${APK_FILES}
				COMMAND ${GRADLE_EXECUTABLE} -p android ${GRADLE_TASK})
			add_custom_target(package_gradle_apk ALL DEPENDS ${APK_FILES})
			set_target_properties(package_gradle_apk PROPERTIES FOLDER "Android")
			add_dependencies(package_gradle_apk package_android)
		else()
			message(WARNING "Gradle executable not found, Android APK will not be assembled")
		endif()
	endif()
endif()
