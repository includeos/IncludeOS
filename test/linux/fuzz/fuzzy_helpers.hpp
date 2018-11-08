#pragma once
#include <cstdint>
#include <net/stream.hpp>

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
    // take up to @bytes and insert into buffer
    size_t insert(net::Stream::buffer_t buffer, size_t bytes) {
      const size_t count = std::min(bytes, this->size);
      buffer->insert(buffer->end(), this->data, this->data + count);
      this->size -= count; this->data += count;
      return count;
    }
    // put randomness into buffer ASCII-variant
    size_t insert_ascii(net::Stream::buffer_t buffer, size_t bytes) {
      const size_t count = std::min(bytes, this->size);
      for (size_t i = 0; i < count; i++) {
        uint8_t byte = data[i];
        if (byte < 33) byte = 123 - (33 - byte);
        if (byte > 122) byte = 33 + (byte - 123);
        buffer->push_back(byte & 0x7F); // ASCII is 7-bit
      }
      this->size -= count; this->data += count;
      return count;
    }
    // put randomness into buffer base64-variant
    size_t insert_base64(net::Stream::buffer_t buffer, size_t bytes)
    {
      static const char LUT[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      const size_t count = std::min(bytes, this->size);
      for (size_t i = 0; i < count; i++) {
        buffer->push_back(LUT[data[i] & 0x3F]);
      }
      this->size -= count; this->data += count;
      return count;
    }
    void fill_remaining(net::Stream::buffer_t buffer)
    {
      buffer->insert(buffer->end(), this->data, this->data + this->size);
      this->size = 0;
    }
    
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
