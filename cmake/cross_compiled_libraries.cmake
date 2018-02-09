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
			    URL https://github.com/hioa-cs/IncludeOS/releases/download/v0.12.0-rc.2/IncludeOS_dependencies_v0-12-0_musl_libunwind_threaded.tar.gz
			    URL_HASH SHA1=4ab139536295321866ebe4136de25879450eb58b
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
set(LIBUNWIND_INCLUDE_DIR ${PRECOMPILED_DIR}/libunwind/include/)
set(LIBUNWIND_LIB_DIR ${PRECOMPILED_DIR}/libunwind/)

add_library(libcxx STATIC IMPORTED)
add_library(libcxxabi STATIC IMPORTED)
add_library(libunwind STATIC IMPORTED)

add_dependencies(libcxx PrecompiledLibraries)
add_dependencies(libcxxabi PrecompiledLibraries)
set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${LIBCXX_LIB_DIR}/libc++.a)
set_target_properties(libcxxabi PROPERTIES IMPORTED_LOCATION ${LIBCXX_LIB_DIR}/libc++abi.a)
set_target_properties(libunwind PROPERTIES IMPORTED_LOCATION ${LIBUNWIND_LIB_DIR}/libunwind.a)

set(MUSL_INCLUDE_DIR ${PRECOMPILED_DIR}/musl/include/)
set(MUSL_LIB_DIR ${PRECOMPILED_DIR}/musl/lib)

add_library(libc STATIC IMPORTED)
set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${MUSL_LIB_DIR}/libc.a)
add_dependencies(libc PrecompiledLibraries)

add_library(libm STATIC IMPORTED)
set_target_properties(libm PROPERTIES IMPORTED_LOCATION ${MUSL_LIB_DIR}/libm.a)
add_dependencies(libm PrecompiledLibraries)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(DIRECTORY ${LIBCXX_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/libcxx)
install(DIRECTORY ${LIBUNWIND_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/libunwind)

install(DIRECTORY ${MUSL_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/musl)

set(CHAINLOAD_LOC ${CMAKE_CURRENT_BINARY_DIR}/precompiled/src/PrecompiledLibraries/chainloader)
install(FILES ${CHAINLOAD_LOC} DESTINATION includeos)

file(GLOB musl_libs ${MUSL_LIB_DIR}/*.a)
file(GLOB musl_objs ${MUSL_LIB_DIR}/*.o)

add_custom_command(TARGET PrecompiledLibraries POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Installed elf.h into ${CMAKE_INSTALL_PREFIX}/include"
  )

ExternalProject_Add_Step(PrecompiledLibraries copy_elf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/
  COMMAND ${CMAKE_COMMAND} -E copy ${MUSL_INCLUDE_DIR}/elf.h ${CMAKE_INSTALL_PREFIX}/include/
  DEPENDEES download
  )

install(FILES
  ${musl_libs}
  ${musl_objs}
  ${LIBCXX_LIB_DIR}/libc++.a
  ${LIBCXX_LIB_DIR}/libc++abi.a
  ${LIBUNWIND_LIB_DIR}/libunwind.a
  DESTINATION includeos/${ARCH}/lib)

if (WITH_SOLO5)
# Only x86_64 supported at the moment
if ("${ARCH}" STREQUAL "x86_64")
  install(FILES ${SOLO5_REPO_DIR}/kernel/ukvm/solo5.o ${SOLO5_REPO_DIR}/ukvm/ukvm-bin DESTINATION includeos/${ARCH}/lib)
endif()

install(FILES ${SOLO5_INCLUDE_DIR}/solo5.h DESTINATION includeos/${ARCH}/include)
endif(WITH_SOLO5)
