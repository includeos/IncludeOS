// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef UPLINK_TRANSPORT_HPP
#define UPLINK_TRANSPORT_HPP

#include <cstdint>
#include <vector>
#include <delegate>
#include <string>
#include <memory>

namespace uplink {

  enum class Transport_code : uint8_t {
    IDENT     = 1,
    LOG       = 2,
    UPDATE    = 5,
    APPDATA   = 6,
    PANIC     = 7,
    STATS     = 8,
    UPLINK    = 9,
    ERROR     = 255 // Legacy
  };

  // send service name and current binary hash

  using Hashtype = uint32_t;

  struct Header {
    Transport_code  code;
    uint32_t        length;

    Header() = default;
    Header(Transport_code c, uint32_t len)
      : code{c}, length{len}
    {}

    static Header parse(const char* data);

  }__attribute__((packed));

  class Transport {
  public:
    using Data = std::vector<char>;
    using Cargo_it = Data::iterator;
    using Cargo_cit = Data::const_iterator;

  public:
    Transport(const char* data, size_t len)
      : data_{data, data + len}
    {
    }

    Transport(Header&& header)
    {
      data_.reserve(sizeof(Header) + header.length);
      const char* hdr = reinterpret_cast<const char*>(&header);
      data_.insert(data_.end(), hdr, hdr + sizeof(Header));
    }

    const Header& header() const
    { return *(reinterpret_cast<const Header*>(data_.data())); }

    Transport_code code() const
    { return header().code; }

    std::string message() const
    { return {begin(), end()}; }

    Cargo_it begin()
    { return data_.begin() + sizeof(Header); }

    Cargo_it end()
    { return data_.end(); }

    Cargo_cit begin() const
    { return data_.begin() + sizeof(Header); }

    Cargo_cit end() const
    { return data_.end(); }

    auto size() const
    { return data_.size() - sizeof(Header); }

    void load_cargo(const char* data, size_t len)
    {
      data_.insert(data_.end(), data, data+len);
    }

    const Data& data() const
    { return data_; }

    Data& data()
    { return data_; }

    bool is_complete() const
    { return header().length == size(); }

  private:
    Data data_;

  }; // < class Transport

  using Transport_ptr = std::unique_ptr<Transport>;

  class Transport_parser {
  public:
    using Transport_complete  = delegate<void(Transport_ptr)>;
    using Header_complete     = delegate<void(const Header&)>;

  public:
    Transport_complete  on_complete;
    Header_complete     on_header;

    Transport_parser(Transport_complete cb);

    void parse(const char* data, size_t len);

  private:
    Transport_ptr transport_;

  }; // < class Transport_parser

} // < namespace uplink

#endif
