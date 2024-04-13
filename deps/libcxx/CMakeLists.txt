cmake_minimum_required(VERSION 2.8)
include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
#libcxx/include MUST be before the musl include #include_next
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#conan_basic_setup the manual way
include_directories(${CONAN_INCLUDE_DIRS_LIBCXXABI})
include_directories(${CONAN_INCLUDE_DIRS_LIBUNWIND})
include_directories(${CONAN_INCLUDE_DIRS_MUSL})
set(LIBCXX_CXX_ABI_INCLUDE_PATHS ${CONAN_INCLUDE_DIRS_LIBCXXABI} CACHE INTERNAL "Force value for subproject" )
set(LIBCXX_CXX_ABI_LIBRARY_PATH  ${CONAN_LIB_DIRS_LIBCXXABI} CACHE INTERNAL "Force value for subproject" )
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeListsOriginal.txt)
