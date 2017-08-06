
include(ExternalProject)
ExternalProject_Add(bucket
  GIT_REPOSITORY "https://github.com/includeos/bucket.git"
  PREFIX bucket
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  )

set(BUCKET_DIR ${CMAKE_CURRENT_BINARY_DIR}/bucket/src/)
