#pragma once
#include <delegate>
#include <string>

struct ReadQ
{
  typedef delegate<void(const std::string&)> on_read_func;
  // returns false if max readq exceeded
  bool read(uint8_t* buf, size_t len, on_read_func);
  
  size_t size() const noexcept {
    return buffer.size();
  }
  const std::string& get() const noexcept {
    return buffer;
  }
  void set(std::string val) {
    buffer = std::move(val);
  }
  void clear() {
    buffer.clear();
    buffer.shrink_to_fit();
  }
  
private:
  std::string buffer;
};
