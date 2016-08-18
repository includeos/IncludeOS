#pragma once
#include <lest/lest.hpp>

#define CASE( name ) lest_CASE( specification(), name )
extern lest::tests & specification();

#ifndef HAVE_LEST_MAIN
#include "lestmain.cxx"
#endif
