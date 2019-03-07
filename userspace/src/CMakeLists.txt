
set(SOURCES
    arch.cpp
    config.cpp
    main.cpp
    os.cpp
    profile.cpp
    drivers/memdisk.cpp
  )

if (NOT PORTABLE)
  set(SOURCES ${SOURCES} drivers/tap_driver.cpp linux_evloop.cpp)
endif()

add_library(linuxrt STATIC ${SOURCES})
install(TARGETS linuxrt DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos/linux)
install(FILES drivers/usernet.hpp DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos/linux/drivers)
