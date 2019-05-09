set(SRCS
    async.cpp
    statman.cpp
    logger.cpp
    sha1.cpp
    syslog_facility.cpp
    syslogd.cpp
    percent_encoding.cpp
    path_to_regex.cpp
    crc32.cpp
)

#if (NOT CMAKE_TESTING_ENABLED)

if (NOT ${PLATFORM} STREQUAL "nano")
  include_directories(${CMAKE_SOURCE_DIR}/lib/LiveUpdate/include)
  list(APPEND SRCS
    memstream.c
    tar.cpp
    uri.cpp #rapidjson
    autoconf.cpp
    config.cpp
    statman_liu.cpp
  )
endif()



add_library(util OBJECT ${SRCS})
