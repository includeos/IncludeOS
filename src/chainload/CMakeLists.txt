cmake_minimum_required(VERSION 3.6)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# IncludeOS install location
project(chainloader C CXX)

find_package(IncludeOS REQUIRED)

set(ARCH i686)
set(PLATFORM nano)
set(ELF_SYMBOLS true)

include(os)

option(default_stdout "" ON)
set(SOURCES
    service.cpp hotswap.cpp
  )
set(BINARY_NAME chainloader)

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)
os_install(TARGETS chainloader.elf.bin.copy DESTINATION bin)
os_install(TARGETS _elf_symbols.bin DESTINATION bin)
