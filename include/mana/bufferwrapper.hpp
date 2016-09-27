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

#ifndef SERVER_BUFFER_HPP
#define SERVER_BUFFER_HPP

namespace server {

/**
 *
 */
template <typename PTR>
class BufferWrapper {
private:
  using ptr_t = PTR;

public:
  /**
   *
   */
  BufferWrapper(ptr_t ptr, const size_t sz)
    : data_ {ptr}
    , size_ {sz}
  {}

  /**
   *
   */
  const ptr_t begin()
  { return data_; }

  /**
   *
   */
  const ptr_t end()
  { return data_ + size_; }

private:
  ptr_t        data_;
  const size_t size_;
}; //< class BufferWrapper

} //< namespace server

#endif //< SERVER_BUFFER_HPP
