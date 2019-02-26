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
			    URL https://github.com/hioa-cs/IncludeOS/releases/download/v0.12.0-rc.2/IncludeOS_dependencies_v0-12-0_musl_libunwind_singlethreaded.tar.gz
			    URL_HASH SHA1=d011b393fff5eba6df865ffb085628a105e9404d
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
	GIT_TAG master
	CONFIGURE_COMMAND CC=gcc ./configure.sh
	UPDATE_COMMAND ""
	BUILD_COMMAND make
	INSTALL_COMMAND ""
)

set(SOLO5_REPO_DIR ${CMAKE_CURRENT_BINARY_DIR}/precompiled/src/solo5_repo)
set(SOLO5_INCLUDE_DIR ${SOLO5_REPO_DIR}/include)

# solo5 in hvt mode (let's call it "solo5-hvt")
add_library(solo5_hvt STATIC IMPORTED)
set_target_properties(solo5_hvt PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/bindings/hvt/solo5_hvt.o)

# solo5-hvt
add_library(solo5-hvt STATIC IMPORTED)
set_target_properties(solo5_hvt PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/tenders/hvt/solo5-hvt)

# solo5 in spt mode (let's call it "solo5-spt")
add_library(solo5_spt STATIC IMPORTED)
set_target_properties(solo5_spt PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/bindings/spt/solo5_spt.o)

# solo5-spt
add_library(solo5-spt STATIC IMPORTED)
set_target_properties(solo5_spt PROPERTIES IMPORTED_LOCATION ${SOLO5_REPO_DIR}/tenders/spt/solo5-spt)

add_dependencies(solo5_hvt solo5_repo)
add_dependencies(solo5-hvt solo5_repo)
add_dependencies(solo5_spt solo5_repo)
add_dependencies(solo5-spt solo5_repo)

# Some OS components depend on solo5 (for solo5.h for example)
add_dependencies(PrecompiledLibraries solo5_hvt)
add_dependencies(PrecompiledLibraries solo5-hvt)
add_dependencies(PrecompiledLibraries solo5_spt)
add_dependencies(PrecompiledLibraries solo5-spt)

endif (WITH_SOLO5)

set(PRECOMPILED_DIR ${CMAKE_CURRENT_BINARY_DIR}/precompiled/src/PrecompiledLibraries/${ARCH})

set(LIBGCC_LIB_DIR ${PRECOMPILED_DIR}/libgcc/)
set(LIBCXX_INCLUDE_DIR ${PRECOMPILED_DIR}/libcxx/include/)
set(LIBCXX_LIB_DIR ${PRECOMPILED_DIR}/libcxx/)
set(LIBUNWIND_INCLUDE_DIR ${PRECOMPILED_DIR}/libunwind/include/)
set(LIBUNWIND_LIB_DIR ${PRECOMPILED_DIR}/libunwind/)

set(MUSL_INCLUDE_DIR ${PRECOMPILED_DIR}/musl/include/)
set(MUSL_LIB_DIR ${PRECOMPILED_DIR}/musl/lib)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(DIRECTORY ${LIBCXX_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/libcxx)
install(DIRECTORY ${LIBUNWIND_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/libunwind)

install(DIRECTORY ${MUSL_INCLUDE_DIR} DESTINATION includeos/${ARCH}/include/musl)

add_custom_command(TARGET PrecompiledLibraries POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Installed elf.h into ${CMAKE_INSTALL_PREFIX}/include"
  )

ExternalProject_Add_Step(PrecompiledLibraries copy_elf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/
  COMMAND ${CMAKE_COMMAND} -E copy ${MUSL_INCLUDE_DIR}/elf.h ${CMAKE_INSTALL_PREFIX}/include/
  DEPENDEES download
  )

# Install musl
install(DIRECTORY ${MUSL_LIB_DIR}/ DESTINATION includeos/${ARCH}/lib)

# Install libc++ etc.
install(FILES
  ${LIBCXX_LIB_DIR}/libc++.a
  ${LIBCXX_LIB_DIR}/libc++abi.a
  ${LIBGCC_LIB_DIR}/libcompiler.a
  ${LIBUNWIND_LIB_DIR}/libunwind.a
  DESTINATION includeos/${ARCH}/lib)


if (WITH_SOLO5)
# Only x86_64 supported at the moment
if ("${ARCH}" STREQUAL "x86_64")
  install(FILES ${SOLO5_REPO_DIR}/bindings/hvt/solo5_hvt.o ${SOLO5_REPO_DIR}/tenders/hvt/solo5-hvt DESTINATION includeos/${ARCH}/lib)
  install(FILES ${SOLO5_REPO_DIR}/bindings/spt/solo5_spt.o ${SOLO5_REPO_DIR}/tenders/spt/solo5-spt DESTINATION includeos/${ARCH}/lib)
endif()

install(FILES ${SOLO5_INCLUDE_DIR}/solo5/solo5.h DESTINATION includeos/${ARCH}/include)
endif(WITH_SOLO5)
