cmake_minimum_required(VERSION 3.0)
project("LLVM_libc++ for IncludeOS")
message(STATUS, "IncludeOS toplevel CMake for LLVM's libc++")

find_package(musl)
find_package(libunwind)
find_package(libcxxabi)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeListsOriginal.txt)

message(STATUS, "DEBUG: Found targets: ${BUILDSYSTEM_TARGETS}")

# target_link_libraries(... musl::musl libunwind::libunwind libcxxabi::libcxxabi)
