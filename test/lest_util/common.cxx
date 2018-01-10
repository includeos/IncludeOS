#pragma once
#include <lest/lest.hpp>

#define CASE( name ) lest_CASE( specification(), name )
extern lest::tests & specification();

#ifdef DEBUG_UNIT
#define LINK printf("%s:%i: OK\n",__FILE__,__LINE__)
#else
#define LINK (void)
#endif


#ifndef HAVE_LEST_MAIN
#include "lestmain.cxx"
#endif
