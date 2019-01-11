set(SRCS
    fuzzy_packet.cpp
    fuzzy_stack.cpp
  )

add_library(fuzz OBJECT ${SRCS})
