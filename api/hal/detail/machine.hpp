// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef OS_DETAIL_MACHINE_HPP
#define OS_DETAIL_MACHINE_HPP

#include <arch.hpp>
#include <typeindex>
#include <unordered_map>
#include <hal/machine.hpp>
#include <util/alloc_lstack.hpp>

namespace os {
  using Machine_allocator =
    util::alloc::Lstack<util::alloc::Lstack_opt::merge, os::Arch::word_size>;

  /**  Extensible Memory class  **/
  class Machine::Memory : public Machine_allocator {
  public:
    using Alloc = Machine_allocator;
    using Alloc::Alloc;
    // TODO: Keep track of donated pools;
  };
}

namespace os::detail {
  using namespace util;

  class Machine_access_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  class Machine {
  public:
    using Memory = os::Machine::Memory;
    template <typename T>
    using Allocator = mem::Allocator<T, Memory>;

    template <typename T>
    using Vec = std::vector<T,Allocator<T>>;

    template <typename K, typename V>
    using Map = std::unordered_map<K, V,
                                   std::hash<K>,
                                   std::equal_to<K>,
                                   Allocator<std::pair<const K,V>>>;

    struct Part {
      enum class Storage : uint8_t {
        machine, heap
       };

      void* ptr;
      Storage storage;
      std::type_index t_idx;
    };

    using Parts_vec   = Vec<Part>;
    using Parts_ent   = std::pair<std::type_index, Parts_vec>;
    using Parts_alloc = const Allocator<Parts_ent>;
    using Parts_map   = Map<std::type_index, Parts_vec>;
    using Ptr_alloc   = const Allocator<void*>;

    template <typename T>
    struct Deleter {
      void operator()(T* obj) noexcept {
        os::machine().memory().deallocate(obj, sizeof(T));
      }
    };

    template <typename T>
    using Unique_ptr = std::unique_ptr<T,Deleter<T>>;

    using Arch = os::Arch;

    Machine(void* mem, size_t size);
    const char* arch();
    virtual const char* name() { return  "PC"; }

    template <typename T>
    T& get(int i) {
      const std::type_index t_idx = std::type_index(typeid(T));
      auto vec_it = parts_.find(t_idx);
      if (vec_it == parts_.end()) {
        throw Machine_access_error("Requested machine part not found");
      }

      auto& vec = vec_it->second;
      auto& part = vec.at(i);
      Expects(part.t_idx == t_idx);
      T* tptr = (T*)part.ptr;
      return *tptr;
    }

    template <typename T>
    Machine::Allocator<T>& allocator() {
      return  ptr_alloc_;
    }

    template <typename T>
    os::Machine::Vector<T> get() {
      const std::type_index t_idx = std::type_index(typeid(T));
      auto vec_it = parts_.find(t_idx);
      if (vec_it == parts_.end()) {
        throw Machine_access_error("Requested machine parts not found");
      }

      auto& vec = vec_it->second;

      os::Machine::Vector<T> new_parts(ptr_alloc_);
      for (auto& part : vec) {
        auto ref = std::ref(*(reinterpret_cast<T*>(part.ptr)));
        new_parts.emplace_back(ref);
      }
      return new_parts;
    }

    template <typename T>
    ssize_t add(T* part, Part::Storage storage) {
      const std::type_index t_idx = std::type_index(typeid(T));
      auto vec_it = parts_.find(t_idx);
      if (vec_it == parts_.end()) {
        auto res = parts_.emplace(t_idx, Parts_vec(ptr_alloc_));
        if (! res.second)  {
          return -1;
        }
        vec_it = res.first;
      }

      auto& vec = vec_it->second;

      Part p {part, storage, t_idx};
      vec.push_back(p);
      Ensures(vec.size() > 0);
      return vec.size() - 1;
    }

    template <typename T, typename... Args>
    ssize_t add_new(Args&&... args) {
      auto* mem = memory().allocate(sizeof(T));
      auto* ptr (new (mem) T{std::forward<Args>(args)...});
      return add<T>(ptr, Part::Storage::machine);
    }

    template <typename T>
    void remove(int i);

    inline Memory& memory() {
      return mem_;
    }

    void init();

  private:
    Memory mem_;
    Ptr_alloc ptr_alloc_;
    Parts_map parts_;
  };

} // namespace detail


// Machine wrappers
namespace os {
  template <typename T>
  ssize_t Machine::add(std::unique_ptr<T> part) noexcept {
    return impl->add<T>(part.release(), detail::Machine::Part::Storage::heap);
  }

  template <typename T, typename... Args>
  ssize_t Machine::add_new(Args&&... args) {
    return impl->add_new<T>(args...);
  }

  template <typename T>
  T& Machine::get(int id) {
    return impl->get<T>(id);
  }

  template <typename T>
  Machine::Vector<T> Machine::get() {
    return impl->get<T>();
  }

  template <typename T>
  Machine::Allocator<T>& Machine::allocator() {
    return impl->allocator<T>();
  }

}

#endif // OS_MACHINE_HPP
