cmake_minimum_required(VERSION 3.6.0)

#we are only creating libraries for ELF
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (includeos C CXX)

option(SMP "Compile with SMP (multiprocessing)" OFF)

#Are we executing cmake from conan or locally
#if locally then pull the deps from conanfile.py
#if buiding from conan expect the conanbuildinfo.cmake to already be present
if(CONAN_EXPORTED OR EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CONAN_SETTINGS_BUILD_TYPE})
  endif()

else() # in user space

  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  #Sets the includeos default profile to clang-5.0
  if (DEFINED CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()

  #ordering matters we create opts here
  if (SMP)
    set(OPTS OPTIONS smp="True")
  endif()

  if (PLATFORM)
    if (OPTS)
      list(APPEND OPTS platform=${PLATFORM})
    else()
      set(OPTS OPTIONS platform=${PLATFORM})
    endif()
  endif()

  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/develop/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)

  conan_check(VERSION 1.8.4 REQUIRED)

  conan_cmake_run(
    CONANFILE conanfile.py
    ${OPTS}
    BASIC_SETUP
    ${CONANPROFILE}
  )
endif()

include(cmake/includeos.cmake)

add_subdirectory(src)

# Install cmake files
install(FILES cmake/linux.service.cmake DESTINATION cmake)
install(FILES cmake/os.cmake DESTINATION cmake)
install(FILES cmake/includeos.cmake DESTINATION cmake)

install(DIRECTORY api/ DESTINATION include/os)
