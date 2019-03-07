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

#include <fs/disk.hpp>
#include <fs/filesystem.hpp>
#include <fs/path.hpp>
#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <fs/fd_compatible.hpp>

// Demangle
extern "C" char* __cxa_demangle(const char* mangled_name,
                            char*       output_buffer,
                            size_t*     length,
                            int*        status);

inline std::string type_name(const std::type_info& type_, size_t max_chars = 0) {
  int status;
  std::string result;
  auto* res =  __cxa_demangle(type_.name(), nullptr, 0, &status);
  if (status == 0) {
        result = std::string(res);
        std::free(res);
  } else {
    result = type_.name();
  }

  if (max_chars and result.size() > max_chars)
    return result.substr(0, max_chars - 3) + "...";

  return result;
}


namespace fs {

  struct VFS_err : public std::runtime_error {
    using runtime_error::runtime_error;
  };
  /** Exception: Trying to fetch an object of wrong type  **/
  struct Err_bad_cast : public VFS_err {
    using VFS_err::VFS_err;
  };

  /** Exception: trying to fetch object from non-leaf **/
  struct Err_not_leaf : public VFS_err {
    using VFS_err::VFS_err;
  };

  /** Exception: trying to access children of non-parent **/
  struct Err_not_parent : public VFS_err {
    using VFS_err::VFS_err;
  };

  /** Exception: trying to access non-existing node  **/
  struct Err_not_found : public VFS_err {
    using VFS_err::VFS_err;
  };

  /** Exception: trying to mount on an occupied or non-existing mount point **/
  struct Err_mountpoint_invalid : public VFS_err {
    using VFS_err::VFS_err;
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

    virtual std::string name() const { return name_; }
    virtual std::string desc() const { return desc_; }

    const std::type_info& type() const
    { return type_; }

    std::string type_name(size_t max_chars = 0) const {
      return ::type_name(type_, max_chars);
    }

    template <typename T>
    VFS_entry(T& obj, const std::string& name, const std::string& desc) // Can't have default desc due to clash with next ctor
      : type_{typeid(T)}, obj_is_const{std::is_const<T>::value},
        obj_{const_cast<typename std::remove_cv<T>::type*>(&obj)},
        name_{name}, desc_{desc},
        is_fd_compatible{std::is_base_of<FD_compatible, T>::value}
    {
    }

    VFS_entry(std::string name, std::string desc)
      : type_{typeid(nullptr)}, obj_{nullptr}, name_{name}, desc_{desc},
        is_fd_compatible{false}
    {
    }

    VFS_entry() = delete;
    VFS_entry(VFS_entry&) = delete;
    VFS_entry(VFS_entry&&) = delete;
    VFS_entry& operator=(VFS_entry&) = delete;
    VFS_entry& operator=(VFS_entry&&) = delete;

    // Node ownership is handled by unique pointers
    // Object pointer is borrowed
    virtual ~VFS_entry() = default;

    /** Fetch the object mounted at this node, if any **/
    template <typename T>
    T& obj() const {

      if (UNLIKELY(not obj_))
        throw Err_not_leaf(name_ + " does not hold an object ");

      // if not the same type, and not FD_compatible
      if (UNLIKELY(typeid(T) != type_ and
        not (std::is_same<FD_compatible, T>::value and this->is_fd_compatible)))
      {
        throw Err_bad_cast(name_ + " is not of type " + ::type_name(typeid(T)));
      }

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

      printf("%s-- %s", tabs.c_str(), name_.c_str());

      if (obj_)
        printf(" (%s)\n", type_name(20).c_str());
      else
        printf("\n");

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

      Expects(not path.empty());

      Obs_ptr current_node = this;
      Obs_ptr next_node = nullptr;

      while (not path.empty()) {
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
      }


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

      Expects(not path.empty());

      auto token = path.back();
      path.pop_back();

      VFS_entry* parent = nullptr;

      if (not path.empty()) {
        parent = walk<create>(path);
        if (not parent) {
          Expects(not create); // Parent node should have been created otherwise
          throw_if<not create, Err_mountpoint_invalid>(path.back() + " doesn't exist");
        }

      } else {
        parent = this;
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
      children_.emplace_back(std::make_unique<VFS_entry>(token, "Directory"));
      return children_.back().get();
    }

    template <typename T>
    VFS_entry& insert(const std::string& token, T& obj, const std::string& desc) {
      children_.emplace_back(std::make_unique<VFS_entry>(obj, token, desc));
      return *children_.back();
    }

    const std::type_info& type_;
    bool obj_is_const = true;
    void* obj_ = nullptr;
    std::string name_;
    std::string desc_;
    std::vector<Own_ptr> children_;
    bool is_fd_compatible = false;

  }; // End VFS_entry


  /** Entry point for global VFS_entry tree **/
  struct VFS {

    using Disk_id  = int;
    using Disk_key = Disk_id;
    using Disk_map = std::map<Disk_key, fs::Disk_ptr>;
    using Path_str = std::string;
    using Dirent_mountpoint = std::pair<Disk_key, Path_str>;
    using Dirent_map = std::map<Dirent_mountpoint, Dirent>;
    using Disk_ptr_weak = std::weak_ptr<Disk>;
    using insert_dirent_delg = delegate<void(error_t, Dirent&)>;
    using on_mount_delg = delegate<void(fs::error_t)>;

    template<bool create_path = true, typename T>
    static void mount(Path path, T& obj, std::string desc) {
      INFO("VFS", "Mounting %s on %s", type_name(typeid(obj)).c_str(), path.to_string().c_str());;
      mutable_root().mount<create_path, T>(path, obj, desc);
    }

    /** Mount a path local to a disk, on a VFS path - async **/
    static inline void mount(
      Path local,
      Disk_id disk,
      Path remote,
      std::string desc,
      on_mount_delg callback
    )
    {

      INFO(
        "VFS",
        "Creating mountpoint for %s::%s on %s",
        (std::string("blk") + std::to_string(disk)).c_str(),
        remote.to_string().c_str(),
        local.to_string().c_str()
      );

      VFS::insert_dirent(
        disk,
        remote,
        insert_dirent_delg::make_packed(
        [local, callback, desc](error_t err, auto& dirent_ref)
        {
          VFS::mount<true>(local, dirent_ref, desc);
          callback(err);
        })
      );
    }

    template <typename P = Path>
    static VFS_entry& get_entry(P path){

      Path p{path};
      auto item = VFS::mutable_root().walk(p);

      if (not item)
        throw Err_not_found(std::string("Path ") + p.to_string() + " does not exist");

      return *item;
    }

    template <typename T, typename P = Path>
    static T& get(P path) {
      return get_entry(path).template obj<T>();
    }


    template<typename P = Path>
    static void stat(P path, on_stat_func fn) {

      Path p{path};
      auto item = VFS::mutable_root().walk(p, true);

      if (not item)
        throw Err_not_found(std::string("Path ") + p.to_string() + " does not exist");

      auto&& obj = item->obj<Dirent>();

      obj.stat(p, fn);
    }

    template<typename P = Path>
    static Dirent stat_sync(P path) {

      Path p{path};
      auto item = VFS::mutable_root().walk(p, true);

      if (not item)
        throw Err_not_found(std::string("Path ") + p.to_string() + " does not exist (stat sync)");

      auto&& obj = item->obj<Dirent>();

      return obj.stat_sync(p);
    }


    static const VFS_entry& root() {
      return mutable_root();
    }

    static fs::Disk_ptr& insert_disk(hw::Block_device& blk) {
      Disk_ptr ptr = std::make_shared<Disk>(blk);
      auto& res = (disk_map().emplace(blk.id(), ptr)).first->second;
      return res;
    }

    static void insert_dirent(Disk_key disk_id, Path path, insert_dirent_delg fn) {

      // Get the disk, and file system from the disk
      auto disk_it = disk_map().find(disk_id);

      if (disk_it == disk_map().end())
        throw Err_not_found(std::string("Disk ") + std::to_string(disk_id) + " is not mounted");

      auto disk = disk_it->second;

      if (not disk->fs_ready())
        throw Disk::Err_not_mounted(std::string("Disk ") + std::to_string(disk_id) + " does not have an initialized file system");
      auto& fs = disk_it->second->fs();

      // Get the dirent from the file system at path
      fs.stat(
        path,
        fs::on_stat_func::make_packed(
        [fn, disk_id, path](auto err, auto dir)
        {
          if (err)
            throw Err_not_found(std::string("Dirent ") + std::to_string(disk_id) + "::" + path.to_string());

          // Preserve the dirent
          auto res_it = (dirent_map().emplace(Dirent_mountpoint{disk_id, path.to_string()}, dir));

          if (res_it.second) {
            auto& saved_dir = res_it.first->second;
            fn(err, saved_dir);
          } else {
            fn(err, invalid_dirent());
          }
        })
      );
    }

  private:

    static Dirent& invalid_dirent() {
      static Dirent dir{nullptr};
      return dir;
    }

    static VFS_entry& mutable_root() {
      static VFS_entry root_{"/", "Root directory"};
      return root_;
    };

    static Disk_map& disk_map() {
      static Disk_map mounted_disks_;
      return mounted_disks_;
    }

    static Dirent_map& dirent_map() {
      static Dirent_map mounted_dirents_;
      return mounted_dirents_;
    }

  };


  /** Template specializations **/
  template<>
  inline void VFS::mount(Path path, hw::Block_device& obj, std::string desc) {
    INFO("VFS", "Creating Disk object for %s ", obj.device_name().c_str());
    auto& disk_ptr = VFS::insert_disk(obj);
    VFS::mount<true>(path, disk_ptr, desc);
  }


  /**
   * Global access points
   **/

  /** fs::mount **/
  template <bool create_path = true, typename T = void, typename P = Path>
  inline auto mount(P path, T& obj, std::string desc = "N/A") {
    Path p{path};
    VFS::mount<create_path, T>(p, obj, desc);
  };

  /** fs::root **/
  inline auto&& root() {
    return VFS::root();
  };

  /** fs::get **/
  template <typename T, typename P = Path>
  inline T& get(P pathstr) {
    return VFS::get<T>(pathstr);
  }

  /** fs::stat_sync **/
  template <typename P = Path>
  inline Dirent stat_sync(P path) {
    Path p{path};
    return VFS::stat_sync(p);
  }

  /** fs::stat async **/
  template <typename P = Path>
  inline void stat(P path, on_stat_func func) {
    Path p{path};
    VFS::stat(p, func);
  }

  /** fs::print_tree **/
  inline void print_tree() {
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
