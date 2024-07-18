cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp plugin1.cpp plugin2.cpp plugin3.cpp
  )

os_add_executable(kernel_plugin_init "Plugin init test" ${SOURCES})
os_add_stdout(kernel_plugin_init default_stdout)
os_add_plugins(kernel_plugin_init example)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
