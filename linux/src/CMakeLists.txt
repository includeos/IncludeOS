
set(SOURCES
    arch.cpp
    config.cpp
    main.cpp
    os.cpp
    profile.cpp
    drivers/usernet.cpp
    drivers/tap_driver.cpp
    drivers/memdisk.cpp
  )

add_library(linuxrt STATIC ${SOURCES})
install(TARGETS linuxrt DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos/linux)
install(FILES drivers/usernet.hpp DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos/linux/drivers)
