#pragma once
#include <cstdint>

namespace fuzzy
{
  struct FuzzyIterator {
    const uint8_t* data;
    size_t         size;
    size_t         data_counter = 0;
    uint16_t       ip_port = 0;
    
    void increment_data(size_t i) { data_counter += i; }
    
    uint8_t  steal8();
    uint16_t steal16();
    uint32_t steal32();
    
    void fill_remaining(uint8_t* next_layer)
    {
      std::memcpy(next_layer, this->data, this->size);
      this->increment_data(this->size);
      this->size = 0;
    }
  };

  inline uint8_t FuzzyIterator::steal8() {
    if (size >= 1) {
      size -= 1;
      return *data++;
    }
    return 0;
  }
  inline uint16_t FuzzyIterator::steal16() {
    return (steal8() >> 8) | steal8();
  }
  inline uint32_t FuzzyIterator::steal32() {
    return (steal16() >> 16) | steal16();
  }
}
