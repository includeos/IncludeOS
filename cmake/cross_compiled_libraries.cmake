# ex: set syntax=cmake:

# If a local bundle location is set that is used, otherwise download from github
if (BUNDLE_LOC)
	include(ExternalProject)
  message(STATUS "Using bundle ${BUNDLE_LOC}")
	ExternalProject_Add(PrecompiledLibraries
			    PREFIX precompiled
			    URL ${BUNDLE_LOC}
			    CONFIGURE_COMMAND ""
			    BUILD_COMMAND ""
			    UPDATE_COMMAND ""
			    INSTALL_COMMAND ""
	)

else(BUNDLE_LOC)
	# TODO: improve (like dynamically download latest version)
	include(ExternalProject)
	ExternalProject_Add(PrecompiledLibraries
			    PREFIX precompiled
			    URL https://github.com/hioa-cs/IncludeOS/releases/download/v0.12.0-rc.2/IncludeOS_dependencies_v0-12-0_libcpp5_threaded.tar.gz
			    URL_HASH SHA1=792613743b6fc7b4ba2d6a180d32e04d2a973846
			    CONFIGURE_COMMAND ""
			    BUILD_COMMAND ""
			    UPDATE_COMMAND ""
			    INSTALL_COMMAND ""
	)

endif (BUNDLE_LOC)

if (WITH_SOLO5)
ExternalProject_Add(solo5_repo
	PREFIX precompiled
	BUILD_IN_SOURCE 1
	GIT_REPOSITORY https://github.com/solo5/solo5.git
	GIT_TAG 2765e0f5f090c0b27a8d62a48285842236e7d20f
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
add_dependencies(PrecompiledLibraries solo5)
add_dependencies(PrecompiledLibraries ukvm-bin)

endif (WITH_SOLO5)

set(PRECOMPILED_DIR ${CMAKE_CURRENT_BINARY_DIR}/precompiled/src/PrecompiledLibraries/${ARCH})

set(LIBCXX_INCLUDE_DIR ${PRECOMPILED_DIR}/libcxx/include/)
set(LIBCXX_LIB_DIR ${PRECOMPILED_DIR}/libcxx/)

add_library(libcxx STATIC IMPORTED)
add_library(libcxxabi STATIC IMPORTED)

add_dependencies(libcxx PrecompiledLibraries)
add_dependencies(libcxxabi PrecompiledLibraries)
set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${LIBCXX_LIB_DIR}/libc++.a)
set_target_properties(libcxxabi PROPERTIES IMPORTED_LOCATION ${LIBCXX_LIB_DIR}/libc++abi.a)

set(NEWLIB_INCLUDE_DIR ${PRECOMPILED_DIR}/newlib/include/)
set(NEWLIB_LIB_DIR ${PRECOMPILED_DIR}/newlib/)

set(LIBGCC_LIB_DIR ${PRECOMPILED_DIR}/libgcc/)

add_library(libc STATIC IMPORTED)
set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${NEWLIB_LIB_DIR}/libc.a)
add_dependencies(libc PrecompiledLibraries)

add_library(libm STATIC IMPORTED)
set_target_properties(libm PROPERTIES IMPORTED_LOCATION ${NEWLIB_LIB_DIR}/libm.a)
add_dependencies(libm PrecompiledLibraries)

set(CRTEND ${PRECOMPILED_DIR}/crt/crtend.o)
set(CRTBEGIN ${PRECOMPILED_DIR}/crt/crtbegin.o)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(DIRECTORY ${LIBCXX_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/libcxx)

install(DIRECTORY ${NEWLIB_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/newlib)

install(FILES ${CRTEND} ${CRTBEGIN} DESTINATION includeos/${ARCH}/lib)

install(FILES ${NEWLIB_LIB_DIR}/libc.a ${NEWLIB_LIB_DIR}/libg.a ${NEWLIB_LIB_DIR}/libm.a ${LIBGCC_LIB_DIR}/libgcc.a ${LIBCXX_LIB_DIR}/libc++.a ${LIBCXX_LIB_DIR}/libc++abi.a DESTINATION includeos/${ARCH}/lib)

if (WITH_SOLO5)
# Only x86_64 supported at the moment
if ("${ARCH}" STREQUAL "x86_64")
  install(FILES ${SOLO5_REPO_DIR}/kernel/ukvm/solo5.o ${SOLO5_REPO_DIR}/ukvm/ukvm-bin DESTINATION includeos/${ARCH}/lib)
endif()

install(FILES ${SOLO5_INCLUDE_DIR}/solo5.h DESTINATION includeos/${ARCH}/include)
endif(WITH_SOLO5)
