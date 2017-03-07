// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef UTIL_CHUNK_HPP
#define UTIL_CHUNK_HPP

#include <common>
#include <gsl/gsl_assert>
#include <memory>
#include <stdexcept>

/**
 * @brief A chunk of shared bytes with a fixed length.
 */
class Chunk {
public:
  using byte_t      = uint8_t;
  using buffer_t    = std::shared_ptr<byte_t>;
  using size_type   = size_t;
  using index_type  = std::ptrdiff_t;

  class iterator;

  /**
   * @brief Constructs an empty chunk (no buffer and length 0)
   */
  Chunk() noexcept;

  /**
   * @brief Constructs a chunk with a shared buffer and a length
   *
   * @param[in] buf The buffer
   * @param[in] len The length
   */
  Chunk(buffer_t buf, const size_type len);

  /**
   * @brief Constructs a chunk with an allocated buffer of the given length
   *
   * @param[in] len The length
   */
  Chunk(const size_type len);

  /**
   * @brief Constructs a chunk with data and length
   *
   * @details Allocates a buffer with the given length, and copying the data provided
   * into the chunks buffer.
   *
   * @param[in] buf The buffer (data) to be copied into the chunk
   * @param[in] len The length
   */
  Chunk(const byte_t* buf, const size_type len);

  Chunk(const Chunk&)             = default;
  Chunk(Chunk&&)                  noexcept = default;
  Chunk& operator=(const Chunk&)  = default;
  Chunk& operator=(Chunk&&)       noexcept = default;
  ~Chunk()                        = default;

  // Getters
  byte_t* data() const noexcept
  { return buffer_.get(); }

  size_type size() const noexcept
  { return length_; }

  size_type length() const noexcept
  { return size(); }

  buffer_t buffer() const noexcept
  { return buffer_; }

  /**
   * @brief Access the byte at the given index
   *
   * @note Undefined behaviour if index is outside chunk length.
   *
   * Use Chunk::at for safe accessing.
   *
   * @param[in] idx The index
   *
   * @return The byte at index
   */
  constexpr byte_t& operator[](index_type idx) const noexcept
  { return data()[idx]; }

  /**
   * @brief Access the byte at the given index
   *
   * @details Throws std::out_of_range if index is outside length
   *
   * @param[in] idx The index
   *
   * @return The byte at index
   */
  byte_t& at(index_type idx) const;

  /**
   * @brief Create a new chunk by combining this chunk with another chunk.
   *
   * @param[in] c The other chunk
   *
   * @return A chunk
   */
  Chunk operator+(const Chunk& c);

  /**
   * @brief Check whether the chunk is valid/not empty
   */
  operator bool() const noexcept
  { return static_cast<bool>(buffer_) and (length_ > 0); }

  // Iterators
  iterator begin() const noexcept;
  iterator end() const noexcept;

  // Compare operations
  bool operator==(const Chunk& c) const noexcept
  { return (buffer_ == c.buffer_); }

  bool operator!=(const Chunk& c) const noexcept
  { return not (*this == c); }

  //
  // Operations
  //

  /**
   * @brief Clears the data in the chunk (buffer)
   */
  void clear() noexcept
  { std::memset(buffer_.get(), 0, length()); }

  /**
   * @brief Fills the chunk with data, starting from offset.
   *
   * @param[in] buf    The buffer with data to be copied
   * @param[in] len    The length of the data
   * @param[in] offset The offset where to start writing in the chunk
   *
   * @return Number of bytes filled
   */
  size_type fill(const byte_t* buf, size_type len, size_type offset = 0);

  //
  // Helpers
  //

  /**
   * @brief Makes a shared buffer.
   *
   * @param[in] len The length
   *
   * @return A shared buffer
   */
  static buffer_t make_shared_buffer(const size_type len)
  { return buffer_t{new uint8_t[len], std::default_delete<uint8_t[]>{}}; }

public:
  class iterator {
  public:
    using T                 = byte_t;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using reference         = T&;
    using pointer           = T*;
    using iterator_category = std::forward_iterator_tag;

    iterator(const Chunk* chunk, size_type index) noexcept
      : chunk_{chunk}
      , index_{static_cast<ptrdiff_t>(index)}
    {}

    iterator(const iterator& other) noexcept
      : iterator(other.chunk_, other.index_)
    {}

    iterator& operator=(const iterator&) noexcept = default;

    constexpr reference operator*() const
    {
      Expects(chunk_ && *chunk_);

      return (*chunk_)[index_];
    }

    constexpr pointer operator->() const
    {
      Expects(chunk_ && *chunk_);

      return &((*chunk_)[index_]);
    }

    iterator& operator++()
    {
      Expects(chunk_ && index_ >= 0 && index_ < static_cast<difference_type>(chunk_->length()));
      ++index_;

      return *this;
    }

    iterator operator++(int) noexcept
    {
      auto ret = *this;
      ++(*this);

      return ret;
    }

    bool operator==(const iterator& it) const noexcept
    { return (chunk_ == it.chunk_) and (index_ == it.index_); }

    bool operator!=(const iterator& it) const noexcept
    { return not (*this == it); }

    void swap(iterator& rhs) noexcept
    {
      std::swap(index_, rhs.index_);
      std::swap(chunk_, rhs.chunk_);
    }

  protected:
    const Chunk* chunk_;
    std::ptrdiff_t index_;
  }; //< class iterator

private:
  buffer_t  buffer_;
  size_type length_;
}; //< class Chunk

inline Chunk::Chunk() noexcept
 : Chunk(nullptr, 0)
{}

inline Chunk::Chunk(buffer_t buf, const size_type len)
 : buffer_{buf}
 , length_{len}
{
  Expects(len <= static_cast<size_t>(PTRDIFF_MAX));
}

inline Chunk::Chunk(const size_type len)
  : Chunk(make_shared_buffer(len), len)
{
  clear();
}

inline Chunk::Chunk(const byte_t* buf, const size_type len)
  : Chunk(make_shared_buffer(len), len)
{
  std::memcpy(buffer_.get(), buf, len);
}

inline Chunk::byte_t& Chunk::at(index_type idx) const
{
  if(UNLIKELY((idx < 0) or (idx >= static_cast<index_type>(size())))) {
    throw std::out_of_range{"Index out of range"};
  }

  return this->operator[](idx);
}

inline Chunk Chunk::operator+(const Chunk& c)
{
  Chunk res{size() + c.size()};
  std::memcpy(res.data(), data(), size());
  std::memcpy(res.data() + size(), c.data(), c.size());

  return res;
}

inline Chunk::iterator Chunk::begin() const noexcept
{
  return {this, 0};
}

inline Chunk::iterator Chunk::end() const noexcept
{
  return {this, size()};
}

inline Chunk::size_type Chunk::fill(const byte_t* buf, size_type len, size_type offset)
{
  if(UNLIKELY(size() <= offset)) {
    return 0;
  }

  const auto n = std::min((size() - offset), len);
  std::memcpy(data() + offset, buf, n);

  return n;
}

#endif //< UTIL_CHUNK_HPP
