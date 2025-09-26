set(SRCS
  control_plane.cpp
  split_queue.cpp
)
add_library(modern_virtio OBJECT ${SRCS})
