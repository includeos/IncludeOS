set(SRCS
  virtio_queue.cpp
)
if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
    virtio.cpp
  )
endif()
add_library(virtio OBJECT ${SRCS})
