#pragma once

#include <kernel/os.hpp>
struct YieldCounter
{
  YieldCounter(const int max)
    : MAX(max), counter(0)
  {}
  
  int MAX;
  int counter;
  
  
  YieldCounter& operator++ ()
  {
    counter++;
    if (counter >= MAX) {
      counter = 0;
      OS::block();
    }
    return *this;
  }
};
