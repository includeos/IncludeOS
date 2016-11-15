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

#ifndef FS_VFS_HPP
#define FS_VFS_HPP

#include <memory>
#include <fs/filesystem.hpp>
#include <fs/path.hpp>
#include <hw/devices.hpp>
#include <typeinfo>
#include <stdexcept>
#include <algorithm>

// Demangle
extern "C" char* __cxa_demangle(const char* mangled_name,
                            char*       output_buffer,
                            size_t*     length,
                            int*        status);

namespace fs {

  /** Exception: Trying to fetch an object of wrong type  **/
  struct Err_bad_cast : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  /** Exception: trying to fetch object from non-leaf **/
  struct Err_not_leaf : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  /** Exception: trying to access children of non-parent **/
  struct Err_not_parent : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  /** Exception: trying to access non-existing node  **/
  struct Err_not_found : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  /** Exception: trying to mount on an occupied or non-existing mount point **/
  struct Err_mountpoint_invalid : public std::runtime_error {
    using runtime_error::runtime_error;
  };


  /**
   * Node in virtual file system tree
   *
   * A node can hold (unique) pointers to other nodes or an arbitrary object.
   *
   * @note : VFS entries will not assume ownership of objects they hold. Only of children.
   * */
  struct VFS_entry {

    // Owning pointer
    using Own_ptr = std::unique_ptr<VFS_entry>;

    // Observation pointer. No ownership, no delete, can be invalidated
    using Obs_ptr = VFS_entry*;

    virtual std::string name() { return name_; }
    virtual std::string desc() { return desc_; }

    const std::type_info& type() const
    { return type_; }

    const std::string type_name(size_t max_chars = 0) const {
      int status;
      std::string result;
      auto* res =  __cxa_demangle(type_.name(), nullptr, 0, &status);
      if (status == 0) {
        result = std::string(res);
        std::free(res);
      } else {
        result = type_.name();
      }

      if (max_chars and max_chars < result.size())
        result = result.substr(0, max_chars - 3) + "...";

      return result;
    }


    template <typename T>
    VFS_entry(T& obj, const std::string& name, const std::string& desc) // Can't have default desc due to clash with next ctor
      : type_{typeid(T)}, obj_is_const{std::is_const<T>::value},
        obj_{const_cast<typename std::remove_cv<T>::type*>(&obj)},
        name_{name}, desc_{desc}
    {}

    VFS_entry(std::string name, std::string desc)
      : type_{typeid(nullptr)}, obj_{nullptr}, name_{name}, desc_{desc}
    {}

    VFS_entry() = delete;
    VFS_entry(VFS_entry&) = delete;
    VFS_entry(VFS_entry&&) = delete;
    VFS_entry& operator=(VFS_entry&) = delete;
    VFS_entry& operator=(VFS_entry&&) = delete;

    // Node ownership is handled by unique pointers
    // Object pointer is borrowed
    ~VFS_entry() = default;

    /** Fetch the object mounted at this node, if any **/
    template <typename T>
    T& obj() const {

      if (UNLIKELY(not obj_))
        throw Err_not_leaf(name_ + " does not hold an object ");

      if (UNLIKELY(typeid(T) != type_))
        throw Err_bad_cast(name_ + " is not of type " + type_name());

      if (UNLIKELY(obj_is_const and not std::is_const<T>::value))
        throw Err_bad_cast(name_ + " must be retrieved as const " + type_name());

      return *static_cast<T*>(obj_);

    };

    /** Convert to the type of held object **/
    template <typename T>
    operator T&() {
      return obj<T>();
    }


    void print_tree(std::string tabs = "" ) const {

      std::cout << tabs << "-- " << name_;

      if (obj_)
        std::cout << " (" << type_name(20) << ") \n";
      else
        std::cout << "\n";

      for (auto&& it = children_.begin(); it != children_.end(); it++) {
        auto& node = *(it->get());

        std::replace(tabs.begin(), tabs.end(), '`', ' ');

        if (it < children_.end() - 1)
          node.print_tree(tabs + "   |");
        else
          node.print_tree(tabs + "   `");

      }
    }


    int child_count() const
    { return children_.size(); }


    /**
     * Walk a given path in the VFS tree.
     *
     * @return pointer to found node, nullptr is none found
     * @param partial : walk as far as possible, return last node if dirent
     * @Note: Clobbers path, to support partial walks.
     **/
    template <bool create = false>
    Obs_ptr walk (Path& path, bool partial = false){

      Obs_ptr current_node = this;
      Obs_ptr next_node = nullptr;

      do {
        auto token = path.front();

        next_node = current_node->get_child(token);

        // Fetch next node mentioned in path, if it exists
        if (not next_node) {

          if (partial and current_node->type() == typeid(Dirent))
            return current_node;

          if (not create)
            return nullptr;

          next_node = current_node->insert_parent(token);
        }

        // Pop off token only when we know it can be passed
        path.pop_front();
        current_node = next_node;
      } while (not path.empty());


      Ensures(next_node);
      Ensures(path.empty());

      return next_node;
    }

    template<bool Throw, typename Exception>
    typename std::enable_if<Throw>::type
    throw_if(std::string msg) {
      throw Exception(msg);
    }

    template<bool Throw, typename Exception>
    typename std::enable_if<not Throw>::type
    throw_if(std::string) { }

    friend struct VFS;

  private:

    /**
     * Mount object (leaf node) on this subtree
     * @note : create is template param to avoid implicit conversion to bool
     **/
    template <bool create, typename T>
    void mount(Path path, T& obj, std::string desc) {

      auto token = path.back();
      path.pop_back();

      auto* parent = walk<create>(path);

      if (not parent) {
        Expects(not create); // Parent node should have been created otherwise
        throw_if<not create, Err_mountpoint_invalid>(path.back() + " doesn't exist");
      }

      // Throw if occupied
      if (parent->get_child(token))
        throw Err_mountpoint_invalid(std::string("Mount point ") + token + " occupied");

      // Insert the leaf
      parent->template insert<T>(token, obj, desc);
    }

    Obs_ptr get_child(const std::string& name) const {
      for (auto&& child : children_)
        if (child.get()->name() == name) return child.get();
      return nullptr;
    };

    Obs_ptr insert_parent(const std::string& token){
      children_.emplace_back(new VFS_entry{token, "Directory"});
      return children_.back().get();
    }

    template <typename T>
    VFS_entry& insert(const std::string& token, T& obj, const std::string& desc) {
      children_.emplace_back(new VFS_entry(obj, token, desc));
      return *children_.back();
    }

    const std::type_info& type_;
    bool obj_is_const = true;
    void* obj_ = nullptr;
    std::string name_;
    std::string desc_;
    std::vector<Own_ptr> children_;

  }; // End VFS_entry


  /** Gateway for VFS_entry trees **/
  struct VFS {

    template<bool create, typename T>
    static void mount(Path path, T& obj, std::string desc) {
      auto&& root = mutable_root();
      mutable_root().mount<create, T>(path, obj, desc);
    }

    template <typename T, typename P = const char*>
    static T& get(P path) {

      Path p{path};
      auto item = VFS::mutable_root().walk(p);

      if (not item)
        throw Err_not_found(std::string("Path ") + p.to_string() + " does not exist");

      return item->template obj<T>();
    }


    template<typename P = const char*>
    static Dirent get(P path) {

      Path p{path};
      auto item = VFS::mutable_root().walk(p, true);

      if (not item)
        throw Err_not_found(std::string("Path ") + p.to_string() + " does not exist");

      auto&& obj = item->obj<Dirent>();

      return obj.stat(p);
    }

    static const VFS_entry& root() {
      return mutable_root();
    }


  private:

    static VFS_entry& mutable_root() {
      static VFS_entry root_{"/", "Root directory"};
      return root_;
    };
  };


  /** Global access point **/
  template <bool create = true, typename T = void, typename P = const char*>
  static inline auto mount(P path, T& obj, std::string desc = "N/A") {
    VFS::mount<create, T>({path}, obj, desc);
  };


  static inline auto&& root() {
    return VFS::root();
  };

  template <typename T, typename P = const char*>
  static inline T& get(P pathstr) {
    return VFS::get<T>(pathstr);
  }

  template <typename P = const char*>
  static inline Dirent stat(P path) {
    Path p{path};
    return VFS::get(p);
  }

  static void print_tree() {
    printf("\n");
    FILLINE('=');
    CENTER("Mount points");
    FILLINE('-');
    root().print_tree();
    FILLINE('_');
    printf("\n");
  }


} // fs

#endif
