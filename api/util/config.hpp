
#pragma once
#ifndef UTIL_CONFIG_HPP
#define UTIL_CONFIG_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <rapidjson/document.h>
#include <cstddef>

/**
 * @brief      Class for the IncludeOS built-in config.
 *
 */
class Config {
public:

  /**
   * @brief      Retrieve the global config.
   *
   * @return     A Config
   */
  static const Config& get() noexcept;

  /**
   * @brief      Retrieve the the config as a parsed json document.
   *
   * @return     Json Doc
   */
  static const rapidjson::Document& doc();

  const char* data() const noexcept
  { return start_; }

  size_t size() const noexcept
  { return end_ - start_; }

  bool empty() const noexcept
  { return size() == 0; }

private:
  Config(const char* start, const char* end) noexcept
    : start_(start), end_(end)
  {}

  const char* const start_;
  const char* const end_;

}; // < class Config

#endif
