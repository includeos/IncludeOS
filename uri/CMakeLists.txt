cmake_minimum_required(VERSION 2.8.12)
project(URI)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Travis
add_subdirectory(src)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
find_package(Lest)

if(LEST_FOUND)
  enable_testing()
  add_subdirectory(test)
else()
  message("Lest was not found, tests will not be built/run.")
endif()
