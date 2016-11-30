add_library(crti STATIC crti.asm)
add_library(crtn STATIC crtn.asm)

set_target_properties(crti crtn PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS crti crtn DESTINATION includeos/lib)
