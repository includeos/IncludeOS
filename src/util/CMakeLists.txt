set(SRCS
    memstream.c
    async.cpp
    statman.cpp
    statman_liu.cpp
    logger.cpp
    sha1.cpp
    syslog_facility.cpp
    syslogd.cpp
    percent_encoding.cpp
    path_to_regex.cpp
    config.cpp
    crc32.cpp
)

#if (NOT CMAKE_TESTING_ENABLED)

if (NOT CORE_OS)
  list(APPEND SRCS
    tar.cpp
    uri.cpp #rapidjson
    autoconf.cpp
  )
endif()

add_library(util OBJECT ${SRCS})
