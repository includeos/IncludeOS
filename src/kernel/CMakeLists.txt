set(SRCS
    block.cpp
    cpuid.cpp
    elf.cpp
    events.cpp
    fiber.cpp
    memmap.cpp
    multiboot.cpp
    os.cpp
    panic.cpp
    profile.cpp
    service_stub.cpp
    #scoped_profiler.cpp
    smp_common.cpp
    smp_utils.cpp
#    elf.cpp
  #  fiber.cpp
#    profile.cpp
    terminal.cpp
    timers.cpp
    threads.cpp
    #tls.cpp
    rng.cpp
    vga.cpp
    context.cpp
    #context_asm.asm
    )
if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
  #  rdrand.cpp
    heap.cpp
    kernel.cpp
    liveupdate.cpp
    rtc.cpp
    system_log.cpp
  )
endif()

if("${ARCH}" STREQUAL "x86_64" OR "${ARCH}" STREQUAL "i686")
  list(APPEND SRCS
    context_asm.asm
    scoped_profiler.cpp
  )
endif()

add_library(kernel OBJECT ${SRCS})
