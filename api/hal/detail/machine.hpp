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

#include <typeindex>
#include <unordered_map>
#include <hal/machine.hpp>
#include <hal/machine_memory.hpp>
#include <util/typename.hpp>

namespace os::detail {
  using namespace util;

  class Machine_access_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  class Machine {
  public:
    using Memory = os::Machine::Memory;

    template <typename T>
    using Allocator = os::Machine::Allocator<T>;

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
    Parts_vec& get_vector() {
      const std::type_index t_idx = std::type_index(typeid(T));
      auto vec_it = parts_.find(t_idx);
      if (vec_it == parts_.end()) {
        throw Machine_access_error("Requested machine parts not found");
      }
      return vec_it->second;
    }

    template <typename T>
    T& get(int i) {
      const std::type_index t_idx = std::type_index(typeid(T));
      auto& vec = get_vector<T>();
      auto& part = vec.at(i);
      Expects(part.t_idx == t_idx);
      T* tptr = (T*)part.ptr;
      return *tptr;
    }

    template <typename T>
    os::Machine::Vector<T> get() {
      auto& vec = get_vector<T>();

      // Populate new vector of ref-wrappers
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
        // Create vector for T if it doesn't exist
        auto res = parts_.emplace(t_idx, Parts_vec(ptr_alloc_));
        if (! res.second)  {
          return -1;
        }
        vec_it = res.first;
      }

      // Add part to T vector
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
    ssize_t count() {
      if (parts_.count(std::type_index(typeid(T))) == 0)
        return 0;

      auto& vec = get_vector<T>();
      return vec.size();
    }

    template <typename T>
    void remove(int i) {
      auto& vec = get_vector<T>();
      if(UNLIKELY(vec.size() < i))
        throw Machine_access_error{"Requested machine part not found: " + std::to_string(i)};
      vec.erase(vec.begin() + i);
    }


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
    INFO("Machine", "Adding %s", demangle(typeid(T).name()).c_str());
    return impl->add<T>(part.release(), detail::Machine::Part::Storage::heap);
  }

  template <typename T, typename... Args>
  ssize_t Machine::add_new(Args&&... args) {
    INFO("Machine", "Adding new %s", demangle(typeid(T).name()).c_str());
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
  void Machine::remove(int i) {
    impl->remove<T>(i);
  };

  template <typename T>
  ssize_t Machine::count() {
    return impl->count<T>();
  }

}

#endif // OS_MACHINE_HPP
