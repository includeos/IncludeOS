include_directories(../api)
include_directories(../mod)
include_directories(../mod/GSL)
include_directories(../src/include)

add_subdirectory(src)
if (BUILD_PLUGINS)
add_subdirectory(src/plugins)
endif()
add_subdirectory(includeos)
