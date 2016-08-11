#cmake_minimum_required(VERSION 2.8.9)

#project (boot)

#set(INCLUDEOS_INSTALL_DIR /home/andy/IncludeOS_install/)
#set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../)

#set(CMAKE_CXX_FLAGS "-MMD -target i686-elf -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=15583 -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -Wall -Wextra  -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\"v0.8.1-709-gf8fccf4\"")

#include_directories(${INCLUDEOS_ROOT}/api/sys ${INCLUDEOS_INSTALL_DIR}/libcxx/include ${INCLUDEOS_INSTALL_DIR}/newlib/include ${INCLUDEOS_ROOT}/src/include ${INCLUDEOS_ROOT}/api ${INCLUDEOS_ROOT}/mod/GSL/include)

add_executable(multiboot multiboot.cpp)

# TODO: use prefix to install into the right directory
install(TARGETS multiboot DESTINATION bin)

# Problem/TODO: the linker always gets the same flags as the compiler (which is expected for GCC). LLVM/Clang does not like it that way and outputs warnings -- still compiles though

# cmake
# /usr/lib64/ccache/clang++   -MMD -target i686-elf -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=15583 -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -Wall -Wextra  -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION="v0.8.1-709-gf8fccf4"   CMakeFiles/multiboot.dir/multiboot.cpp.o  -o multiboot -rdynamic

# make
# /usr/lib64/ccache/clang++ -MMD -target i686-elf -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=15583 -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -Wall -Wextra  -c -m32 -std=c++14 -I../api/sys -I/home/andy/IncludeOS_install/libcxx/include -I/home/andy/IncludeOS_install/newlib/include -Iinclude -I../api -I../mod/GSL/include -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION="\"v0.8.1-709-gf8fccf4\"" -o boot/multiboot.o boot/multiboot.cpp

# work on NASM

#enable_language(ASM_NASM)

#include_directories(${INCLUDEOS_ROOT}/api/sys ${INCLUDEOS_INSTALL_DIR}/libcxx/include ${INCLUDEOS_INSTALL_DIR}/newlib/include ${INCLUDEOS_ROOT}/src/include ${INCLUDEOS_ROOT}/src/ ${INCLUDEOS_ROOT}/api ${INCLUDEOS_ROOT}/mod/GSL/include)

#add_executable(bootloader bootloader.asm disk_read_lba.asm)

add_custom_command(
	OUTPUT bootloader
	COMMAND nasm -f bin -I${CMAKE_CURRENT_SOURCE_DIR}/../ -o bootloader ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.asm
	DEPENDS bootloader.asm disk_read_lba.asm
)

add_custom_target(run ALL DEPENDS bootloader)
