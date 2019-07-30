set(PACKAGE_NAME "ncParticleEditor")
set(PACKAGE_EXE_NAME "ncparticleeditor")
set(PACKAGE_VENDOR "Angelo Theodorou")
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
endfunction()
