set (LEST_ROOT_DIR $ENV{HOME}/IncludeOS/test/lest/include)
find_path(LEST_INCLUDE_DIR
  NAMES lest/lest.hpp
  PATHS ${LEST_ROOT_DIR}
  DOC "Lest include path"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LEST DEFAULT_MSG LEST_INCLUDE_DIR)
