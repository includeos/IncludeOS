
#pragma once
#ifndef X86_TSS_HPP
#define X86_TSS_HPP

#include <cstdint>

namespace x86
{
  struct AMD64_TSS
  {
    uint32_t ign;  // 4
    uint64_t rsp0; // 12
    uint64_t rsp1; // 20
    uint64_t rsp2; // 28
    uint32_t ign2; // 32
    uint32_t ign3; // 36
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7; // 92 0x5C
    uint32_t ign4;
    uint32_t ign5;
    uint16_t ign6;
    uint16_t iomap_base;
  } __attribute__((packed));

  struct AMD64_TS {
  	uint64_t td_lolimit:16; /* segment extent (lsb) */
  	uint64_t td_lobase:24;  /* segment base address (lsb) */
  	uint64_t td_type:5;     /* segment type */
  	uint64_t td_dpl:2;      /* segment descriptor priority level */
  	uint64_t td_present:1;  /* segment descriptor present */
  	uint64_t td_hilimit:4;  /* segment extent (msb) */
  	uint64_t td_resv1:3;    /* avl, long and def32 (not used) */
  	uint64_t td_gran:1;     /* limit granularity (byte/page) */
  	uint64_t td_hibase:40;  /* segment base address (msb) */
  	uint64_t td_resv2:8;    /* reserved */
  	uint64_t td_zero:5;     /* must be zero */
  	uint64_t td_resv3:19;   /* reserved */
  } __attribute__((packed));

}

#endif
