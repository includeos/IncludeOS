// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef UTIL_SIGNAL_HPP
#define UTIL_SIGNAL_HPP

#include <vector>
#include <functional>

template <typename F>
class signal {
public:
  //! \brief Callable type of the signal handlers
  using handler = std::function<F>;

  //! \brief Default constructor
  explicit signal() = default;

  //! \brief Default destructor
  ~signal() noexcept = default;

  //! \brief Default move constructor
  explicit signal(signal&&) noexcept = default;

  //! \brief Default assignment operator
  signal& operator=(signal&&) = default;

  //! \brief Connect a callable object to this signal
  void connect(handler&& fn) {
    funcs.emplace_back(std::forward<handler>(fn));
  }
        
  //! \brief Emit this signal by executing all connected callable objects
  template<typename... Args>
  void emit(Args&&... args) {
    for(auto& fn : funcs)
      fn(std::forward<Args>(args)...);
  }
        
private:
  // Set of callable objects registered to be called on demand
  std::vector<handler> funcs;

  // Avoid copying
  signal(signal const&) = delete;

  // Avoid assignment
  signal& operator=(signal const&) = delete;
}; //< class signal

#endif //< UTILITY_SIGNAL_HPP
