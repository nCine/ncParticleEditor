option(CUSTOM_CRASHRPT "Enable CrashRpt integration on Windows with MSVC" OFF)
set(CRASHRPT_VERSION "1403" CACHE STRING "Set the CrashRpt version string")
set(CRASHRPT_SOURCE_DIR "" CACHE PATH "Set the path to the CrashRpt sources directory")
set(CRASHRPT_BINARY_DIR "" CACHE PATH "Set the path to the CrashRpt build directory")

if(MSVC AND CUSTOM_CRASHRPT)
	set(CRASHRPT_STRINGS_FILE ${GENERATED_SOURCE_DIR}/CrashRptStrings.cpp)
	file(WRITE ${CRASHRPT_STRINGS_FILE} "#include \"CrashRptWrapper.h\"\n\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::appName = \"${NCPROJECT_NAME}\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::appVersion = \"${NCPROJECT_VERSION}\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::emailSubject = \"${NCPROJECT_NAME} ${NCPROJECT_VERSION} Error Report\";\n")
	file(APPEND ${CRASHRPT_STRINGS_FILE} "const char *CrashRptWrapper::emailTo = \"${NCPROJECT_AUTHOR_MAIL}\";\n")
	file(COPY include/CrashRptWrapper.h DESTINATION ${GENERATED_SOURCE_DIR})
	# Source files can't vary by configuration with the Visual Studio generator, they are added even in release
	target_sources(${NCPROJECT_EXE_NAME} PRIVATE include/CrashRptWrapper.h src/CrashRptWrapper.cpp ${CRASHRPT_STRINGS_FILE})
	set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
	set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY TRUE)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CRASHRPT_ARCH x64)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CRASHRPT_ARCH x86)
	endif()

	target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE $<$<CONFIG:Debug>:"WITH_CRASHRPT">)
	target_compile_options(${NCPROJECT_EXE_NAME} PRIVATE $<$<CONFIG:Debug>:/Oy->)

	find_path(CRASHRPT_INCLUDE_DIR CrashRpt.h PATHS ${CRASHRPT_SOURCE_DIR}/include)
	message(STATUS "CrashRpt include dir: ${CRASHRPT_INCLUDE_DIR}")
	
	find_library(CRASHRPT_LIBRARY CrashRpt${CRASHRPT_VERSION} PATHS ${CRASHRPT_BINARY_DIR}/lib/${CRASHRPT_ARCH})
	message(STATUS "CrashRpt library: ${CRASHRPT_LIBRARY}")
	
	target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE $<$<CONFIG:Debug>:${CRASHRPT_INCLUDE_DIR}>)
	target_link_libraries(${NCPROJECT_EXE_NAME} PRIVATE $<$<CONFIG:Debug>:${CRASHRPT_LIBRARY}>)
	
	install(FILES ${CRASHRPT_BINARY_DIR}/bin/${CRASHRPT_ARCH}/CrashRpt${CRASHRPT_VERSION}.dll DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(FILES ${CRASHRPT_BINARY_DIR}/bin/${CRASHRPT_ARCH}/CrashSender${CRASHRPT_VERSION}.exe DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(FILES ${CRASHRPT_SOURCE_DIR}/bin/crashrpt_lang.ini DESTINATION ${RUNTIME_INSTALL_DESTINATION})
	install(FILES ${CRASHRPT_SOURCE_DIR}/bin/dbghelp.dll DESTINATION ${RUNTIME_INSTALL_DESTINATION})

	add_custom_target(copy_crashrpt_files ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHRPT_BINARY_DIR}/bin/${CRASHRPT_ARCH}/CrashRpt${CRASHRPT_VERSION}.dll ${CMAKE_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHRPT_BINARY_DIR}/bin/${CRASHRPT_ARCH}/CrashSender${CRASHRPT_VERSION}.exe ${CMAKE_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRASHRPT_SOURCE_DIR}/bin/crashrpt_lang.ini ${CMAKE_BINARY_DIR}
		COMMENT "Copying CrashRpt files..."
	)
	set_target_properties(copy_crashrpt_files PROPERTIES FOLDER "CustomCopyTargets")
endif()
