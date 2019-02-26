set(SRCS
    multiboot.cpp
    syscalls.cpp
    os.cpp
    cpuid.cpp
    block.cpp
    events.cpp
    memmap.cpp
    pci_manager.cpp
    service_stub.cpp
    elf.cpp
    vga.cpp
    context.cpp
    context_asm.asm
    fiber.cpp
    tls.cpp
    profile.cpp
    scoped_profiler.cpp
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
    rtc.cpp
  )
endif()
add_library(kernel OBJECT ${SRCS})
