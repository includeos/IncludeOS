cmake_minimum_required(VERSION 3.0)

set(EXAMPLES
  256_color_vga
  acorn
  demo_service
#missing certs bundle..
#  http_client
#missing pmr (HAL)
#  IRCd
#  LiveUpdate
  #missing lua
#  lua5
# unknown wrong libc ?
#  mender
  microLB
  #conan dependency issue wrong include path
  #protobuf
  router
  scoped_profiler
  snake
  # TODO make a conan package instead of git-subdirectory
  #SQlite
  starlight
  STREAM
  syslog
  tcp
  TCP_perf
  TLS_server
  #TODO wait for hal
  #transfer
  UDP_perf
  #TODO
  #userspace_demo
  vlan
  #TODO wait for hal
  #websocket
)


foreach(example ${EXAMPLES})

  add_subdirectory(${example} ${example})

endforeach()
