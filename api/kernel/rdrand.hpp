#ifndef KERNEL_RDRAND_HPP
#define KERNEL_RDRAND_HPP

#include <cstdint>
extern bool rdrand16(uint16_t* result);
extern bool rdrand32(uint32_t* result);

#endif
