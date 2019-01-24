set(SRCS
    multiboot.cpp
    syscalls.cpp
    os.cpp
    cpuid.cpp
    block.cpp
    cpuid.cpp
    elf.cpp
    events.cpp
    fiber.cpp
    memmap.cpp
    multiboot.cpp
    pci_manager.cpp
    service_stub.cpp
    elf.cpp
    vga.cpp
    context.cpp

    fiber.cpp
    tls.cpp
    profile.cpp

    terminal.cpp
    timers.cpp
    rng.cpp
    system_log.cpp
    solo5_manager.cpp
    )
if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
    rdrand.cpp
    heap.cpp
    kernel.cpp
    rtc.cpp
  )
endif()

if(${ARCH} STREQUAL "x86_64" OR ${ARCH} STREQUAL "i686")
  list(APPEND SRCS
    context_asm.asm
    scoped_profiler.cpp
  )
endif()

add_library(kernel OBJECT ${SRCS})
