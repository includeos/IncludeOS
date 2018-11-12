
include(ExternalProject)

ExternalProject_Add(solo5_repo
	PREFIX precompiled
	BUILD_IN_SOURCE 1
	GIT_REPOSITORY https://github.com/solo5/solo5.git
	GIT_TAG 285b80aa4da12b628838a78dc79793f4d669ae1b
	CONFIGURE_COMMAND CC=gcc ./configure.sh
	UPDATE_COMMAND ""
	BUILD_COMMAND make
	INSTALL_COMMAND ""
)

set(SOLO5_REPO_DIR ${CMAKE_CURRENT_BINARY_DIR}/precompiled/src/solo5_repo)
set(SOLO5_INCLUDE_DIR ${SOLO5_REPO_DIR}/kernel)

# solo5 in ukvm mode (let's call it "solo5")
add_library(solo5 STATIC IMPORTED)
set_target_properties(solo5 PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/kernel/ukvm/solo5.o)

# ukvm-bin
add_library(ukvm-bin STATIC IMPORTED)
set_target_properties(solo5 PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/ukvm/ukvm-bin)

add_dependencies(solo5 solo5_repo)
add_dependencies(ukvm-bin solo5_repo)

# Some OS components depend on solo5 (for solo5.h for example)
#add_dependencies(PrecompiledLibraries solo5)
#add_dependencies(PrecompiledLibraries ukvm-bin)
