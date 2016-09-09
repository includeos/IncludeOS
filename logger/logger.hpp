// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef UTIL_LOGGER_HPP
#define UTIL_LOGGER_HPP

#include <gsl/span>
#include <string>
#include <iterator>
#include <vector>
#include <cstdlib>

/**
 * @brief A utility which logs a limited amount of entries
 * @details Logs strings inside a ringbuffer.
 *
 * If the buffer is full, the oldest entry will be overwritten.
 *
 */
class Logger {
public:
  using Log = gsl::span<char>;

public:

  Logger(Log& log, Log::index_type = 0);

  /**
   * @brief Log a string
   * @details
   * Write the string to the buffer a zero at the end to indicate end of string.
   *
   * If the string overlaps another string, it will clear the remaining string with zeros.
   *
   * @param str Message to be logged
   */
  void log(const std::string& str);

  /**
   * @brief Retreive all entries from the log
   * @details Iterates forward over the whole buffer, building strings on the way
   *
   * Order old => new
   *
   * @return a vector with all the log entries
   */
  std::vector<std::string> entries() const;

  /**
   * @brief Retreive entries N from the log
   * @details
   * Retrieves N (or less) latest entries from the log,
   * starting with the oldest.
   *
   *
   * @param n maximum number of entries
   * @return a vector with entries
   */
  std::vector<std::string> entries(size_t n) const;

  /**
   * @brief Clear the log
   * @details Sets every byte to 0 and set position to start at the beginning.
   */
  void flush();

  /**
   * @brief Size of the log in bytes.
   * @details Assumes every byte is filled with either data or 0
   * @return size in bytes
   */
  auto size() const
  { return log_.size(); }

private:
  /** The underlaying log */
  Log& log_;

  /**
   * @brief A "circular" iterator, operating on a Logger::Log
   * @details Wraps around everytime it reaches the end
   */
  class iterator : public Log::iterator {
  public:
    using base = Log::iterator;

    // inherit constructors
    using base::base;

    constexpr iterator& operator++() noexcept
    {
      Expects(span_ && index_ >= 0);
      index_ = (index_ < span_->length()-1) ? index_+1 : 0;
      return *this;
    }

    constexpr iterator& operator--() noexcept
    {
      static_assert(true, "Decrement not supported");
      Expects(span_ && index_ < span_->length());
      index_ = (index_ > 0) ? index_-1 : span_->length()-1;
      return *this;
    }

    constexpr iterator& operator+=(difference_type n) noexcept
    {
      Expects(span_);
      index_ = (index_ + n < span_->length()) ? index_ + n : std::abs((n - ((span_->length()) - index_)) % span_->length());
      return *this;
    }

    constexpr span_iterator& operator-=(difference_type n) noexcept
    {
      // No use case for this (yet)
      static_assert(true, "Decrement not supported");
      //index_ = (index - n >= 0) ? index_ - n : std::abs((n -))
      return *this += -n;
    }
  }; // < class Logger::iterator

  /** Current position in the log */
  iterator pos_;

}; // << class Logger

#endif

