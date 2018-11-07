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
  install(FILES ${SOLO5_REPO_DIR}/kernel/ukvm/solo5.o ${SOLO5_REPO_DIR}/ukvm/ukvm-bin DESTINATION includeos/${ARCH}/lib)
endif()

install(FILES ${SOLO5_INCLUDE_DIR}/solo5.h DESTINATION includeos/${ARCH}/include)
endif(WITH_SOLO5)
