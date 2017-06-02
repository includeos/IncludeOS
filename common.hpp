#pragma once
#include "hw_timer.hpp"
#include <kernel/os.hpp>

static void* LIVEUPD_LOCATION   = (void*) 0x2800000; // at 40mb
extern char* heap_begin;
extern char* heap_end;

inline void show_heap_stats()
{
  ptrdiff_t heap_total = OS::heap_max() - (uintptr_t) heap_begin;
  double total = (heap_end - heap_begin) / (double) heap_total;

  fprintf(stderr, "\tHeap is at: %p / %p  (diff=%#x)\n",
         heap_end, (void*) OS::heap_max(),
         (uint32_t) (OS::heap_max() - (uintptr_t) heap_end));
  fprintf(stderr, "\tHeap usage: %u / %u Kb (%.2f%%)\n",
         (uint32_t) (heap_end - heap_begin) / 1024,
         heap_total / 1024, total * 100.0);
}
