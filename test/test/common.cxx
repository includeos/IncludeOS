#include <lest/lest.hpp>

#define CASE( name ) lest_CASE( specification(), name )
extern lest::tests & specification();
