set(SRCS
    block.cpp
    cpuid.cpp
    elf.cpp
    events.cpp
    fiber.cpp
    memmap.cpp
    multiboot.cpp
    pci_manager.cpp
    os.cpp
    profile.cpp
    syscalls.cpp
    service_stub.cpp
    #scoped_profiler.cpp
    system_log.cpp
#    elf.cpp
  #  fiber.cpp
#    profile.cpp
    terminal.cpp
    timers.cpp
    rng.cpp
    tls.cpp
    vga.cpp
    context.cpp
    #context_asm.asm
    )
if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
  #  rdrand.cpp
    heap.cpp
    kernel.cpp
    rtc.cpp
  )
endif()

if("${ARCH}" STREQUAL "x86_64" OR "${ARCH}" STREQUAL "i686")
  list(APPEND SRCS
    context_asm.asm
    scoped_profiler.cpp
  )
endif()

add_library(kernel OBJECT ${SRCS})
