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

#include <span>
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
  using Log = std::span<char>;

public:

  Logger(Log& log, Log::size_type = 0);

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

  /**
   * @brief A "circular" iterator, operating on a Logger::Log
   * @details Wraps around everytime it reaches the end
   */
  class log_iterator {
  public:

    using difference_type = Log::difference_type;

    Log* span_;
    std::size_t index_;

    log_iterator(Log* span, std::size_t index) : span_(span), index_(index) {}

    constexpr log_iterator& operator++() noexcept
    {
      //Expects(span_ && index_ >= 0);
      index_ = (index_ < span_->size()-1) ? index_+1 : 0;
      return *this;
    }

    constexpr log_iterator& operator--() noexcept
    {
      //Expects(span_ && index_ < span_->size());
      index_ = (index_ > 0) ? index_-1 : span_->size()-1;
      return *this;
    }

    constexpr log_iterator& operator+=(difference_type n) noexcept
    {
      //Expects(span_);
      index_ = (index_ + n < span_->size()) ? index_ + n : std::abs(static_cast<ssize_t>((n - static_cast<ssize_t>(span_->size() - index_)) % static_cast<ssize_t>(span_->size())));
      return *this;
    }

    constexpr log_iterator& operator-=(difference_type n) noexcept
    {
      // No use case for this (yet)
      return *this += -n;
    }

    constexpr Log::value_type& operator*() const {
      return (*span_)[index_];
    }

    constexpr bool operator==(const log_iterator& other) const noexcept {
      return &(*span_)[index_] == &(*other);
    }

    const std::size_t& index() const {
      return index_;
    }

  }; // < class Logger::iterator

  const Log& log() const { return log_; }
  const log_iterator& current_pos() const { return pos_; }

private:
  /** The underlaying log */
  Log& log_;

  /** Current position in the log */
  log_iterator pos_;

}; // << class Logger

#endif
