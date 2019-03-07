####################################
#   Linux Userspace CMake script   #
####################################

#set(CMAKE_CXX_STANDARD 17)
set(COMMON "-g -O2 -march=native -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${COMMON}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON}")

option(BUILD_PLUGINS "Build all plugins as libraries" OFF)
option(DEBUGGING    "Enable debugging" OFF)
option(PORTABLE     "Enable portable TAP-free userspace" ON)
option(LIBCPP       "Enable libc++" OFF)
option(PERFORMANCE  "Enable performance mode" OFF)
option(GPROF        "Enable profiling with gprof" OFF)
option(PGO_ENABLE   "Enable guided profiling (PGO)" OFF)
option(PGO_GENERATE "PGO is in profile generating mode" ON)
option(SANITIZE     "Enable undefined- and address sanitizers" OFF)
option(LIBFUZZER    "Enable in-process fuzzer" OFF)
option(PAYLOAD_MODE "Disable things like checksumming" OFF)
option(ENABLE_LTO   "Enable LTO for use with Clang/GCC" ON)
option(CUSTOM_BOTAN "Enable building with a local Botan" OFF)
option(ENABLE_S2N   "Enable building a local s2n" OFF)
option(STATIC_BUILD "Build a portable static executable" OFF)
option(STRIP_BINARY "Strip final binary to reduce size" OFF)
option(USE_LLD "Allow linking against LTO archives" OFF)

if(DEBUGGING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
elseif(PERFORMANCE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()

if (ENABLE_LTO)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto=thin")
  endif()
endif()

if(GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
endif()

if (PGO_ENABLE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (PROF_LOCATION "${CMAKE_BINARY_DIR}/pgo")
    file(MAKE_DIRECTORY ${PROF_LOCATION})
    if (PGO_GENERATE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-dir=${PROF_LOCATION} -fprofile-generate")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-dir=${PROF_LOCATION} -fprofile-use")
    endif()
  else()
    set (PROF_LOCATION "${CMAKE_BINARY_DIR}")
    if (PGO_GENERATE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate=${PROF_LOCATION}/default.profraw")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-use=${PROF_LOCATION}/default.profdata")
    endif()
  endif()
endif()

if(SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
endif()
if(PAYLOAD_MODE)
  add_definitions(-DDISABLE_INET_CHECKSUMS)
endif()
if(LIBFUZZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer-no-link")
  add_definitions(-DLIBFUZZER_ENABLED)
  add_definitions(-DDISABLE_INET_CHECKSUMS)
endif()

if (LIBCPP)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if(CUSTOM_BOTAN)
  include_directories("/usr/local/include/botan/botan-2")
endif()

set(ARCH "x86_64")
add_definitions("-DARCH=${ARCH}" "-DARCH_${ARCH}")
add_definitions(-DOS_TERMINATE_ON_CONTRACT_VIOLATION)
add_definitions(-DARP_PASSTHROUGH)
add_definitions(-DNO_DEBUG)
add_definitions(-DSERVICE=\"\\\"${BINARY}\\\"\")
add_definitions(-DSERVICE_NAME=\"\\\"${SERVICE_NAME}\\\"\")
add_definitions(-DUSERSPACE_KERNEL)
if (PORTABLE)
	add_definitions(-DPORTABLE_USERSPACE)
endif()

set(IOSPATH $ENV{INCLUDEOS_SRC})
set(IOSLIBS $ENV{INCLUDEOS_PREFIX}/${ARCH}/lib)

# IncludeOS userspace
add_subdirectory(${IOSPATH}/userspace   userspace)

# linux executable
add_executable(service ${SOURCES} ${IOSPATH}/src/service_name.cpp)
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

target_include_directories(service PUBLIC
    ${IOSPATH}/api
    ${IOSPATH}/mod/GSL
    ${LOCAL_INCLUDES}
  )

set(LPATH ${IOSPATH}/linux)
set(PLUGIN_LOC "${IOSPATH}/linux/plugins")
set(DRIVER_LOC "${IOSPATH}/${ARCH}/drivers")

# IncludeOS plugins
# TODO: implement me

if (CUSTOM_BOTAN)
  set(BOTAN_LIBS /usr/local/lib/libbotan-2.a)
  target_link_libraries(service ${BOTAN_LIBS} -ldl -pthread)
endif()
if (ENABLE_S2N)
  find_package(OpenSSL REQUIRED)
  include(ExternalProject)
  ExternalProject_add(libs2n
      URL https://github.com/fwsGonzo/s2n_bundle/releases/download/v1.4/s2n.tar
      CONFIGURE_COMMAND ""
      BUILD_COMMAND     ""
      INSTALL_COMMAND   ""
      DOWNLOAD_NAME libs2n.a
  )

  add_library(s2n STATIC IMPORTED)
  set_target_properties(s2n PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(s2n PROPERTIES IMPORTED_LOCATION libs2n-prefix/src/libs2n/lib/libs2n.a)
  add_library(crypto STATIC IMPORTED)
  set_target_properties(crypto PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(crypto PROPERTIES IMPORTED_LOCATION libcrypto.a)
  add_library(openssl STATIC IMPORTED)
  set_target_properties(openssl PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(openssl PROPERTIES IMPORTED_LOCATION libssl.a)

  set(S2N_LIBS s2n OpenSSL::SSL)
  target_link_libraries(service ${S2N_LIBS} -ldl -pthread)
  target_include_directories(includeos PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/libs2n-prefix/src/libs2n/api)
  target_include_directories(microlb PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/libs2n-prefix/src/libs2n/api)
endif()
target_link_libraries(service ${PLUGINS_LIST})
target_link_libraries(service includeos linuxrt microlb liveupdate
                      includeos linuxrt http_parser)
target_link_libraries(service ${EXTRA_LIBS})
if (CUSTOM_BOTAN)
  target_link_libraries(service ${BOTAN_LIBS})
endif()
if (ENABLE_S2N)
  target_link_libraries(service ${S2N_LIBS})
endif()

if (STATIC_BUILD)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  target_link_libraries(service -static-libstdc++ -static-libgcc)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -pthread")
  set(BUILD_SHARED_LIBRARIES OFF)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # use system linker
else()
  if (ENABLE_LTO OR USE_LLD)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
  endif()
endif()
if (LIBFUZZER)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=fuzzer")
endif()

if (STRIP_BINARY)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()

# write binary name to file
file(WRITE ${CMAKE_BINARY_DIR}/binary.txt ${BINARY})
