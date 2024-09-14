cmake_minimum_required(VERSION 3.6)

#we are only creating libraries for ELF
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (includeos C CXX)

option(SMP "Compile with SMP (multiprocessing)" ON)
option(PROFILE "Compile with startup profilers" OFF)

include(cmake/includeos.cmake)

add_subdirectory(src)

# Install cmake files
install(FILES cmake/linux.service.cmake DESTINATION cmake)
install(FILES cmake/os.cmake DESTINATION cmake)
install(FILES cmake/includeos.cmake DESTINATION cmake)

# Install config to enable find_package(IncludeOS)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/includeos-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/includeos-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/includeos-config.cmake DESTINATION cmake)

# Install IncludeOS user code dependencies
install(FILES src/service_name.cpp DESTINATION src)

# Install IncludeOS headers
install(DIRECTORY api/ DESTINATION include/os)
