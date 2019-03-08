set(SRCS
    pci_device.cpp
    nic.cpp
    ps2.cpp
    serial.cpp
    vga_gfx.cpp
    msi.cpp
    pci_msi.cpp
    usernet.cpp
  )


add_library(hw OBJECT ${SRCS})
