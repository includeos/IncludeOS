SET(SRCS
    c_abi.c
    ctype_b_loc.c
    ctype_tolower_loc.c
    string.c
    quick_exit.cpp
    cxx_abi.cpp
    )
add_library(crt OBJECT ${SRCS})

# disable sanitizers on c_abi and cxx_abi, etc.
set_source_files_properties(c_abi.c PROPERTIES COMPILE_FLAGS "-fno-sanitize=all")
set_source_files_properties(cxx_abi.cpp PROPERTIES COMPILE_FLAGS "-fno-sanitize=all")
