// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#pragma once
#ifndef LIVEUPDATE_HEADER_HPP
#define LIVEUPDATE_HEADER_HPP

#include <net/tcp/connection.hpp>
#include <net/stream.hpp>
#include <delegate>
#include <string>
#include <vector>
struct storage_entry;
struct storage_header;

namespace liu
{
struct Storage;
struct Restore;
typedef std::vector<char> buffer_t;

/**
 * The beginning and the end of the LiveUpdate process is the exec() and resume() functions.
 * exec() is called with a provided fixed memory location for where to store all serialized data,
 * and after an update is_resumable, with the same fixed memory location, will return true.
 * resume() can then be called with this same location, and it will call handlers for each @id it finds,
 * unless no such handler is registered, in which case it just calls the default handler which is passed
 * to the call to resume(). The call to resume returns true if everything went well.
**/
struct LiveUpdate
{
  // The buffer_t parameter is the update blob (the new kernel) and can be null.
  // If the parameter is null, you can assume that it's currently not a live update.
  typedef delegate<void(Storage&, const buffer_t*)> storage_func;
  typedef delegate<void(Restore&)> resume_func;

  // Register a function to be called when serialization phase begins
  // Internally it will be stored as its own partition and can be restored using
  // the same @key value during the resume process
  static void register_partition(std::string key, storage_func);

  // Start a live update process, storing all user-defined data
  // If no storage functions are registered no state will be saved
  // If @storage_area is nullptr (default) it will be retrieved from OS
  static void exec(const buffer_t& blob, void* storage_area = nullptr);
  // Same as above, but including the partition [@key, func]
  static void exec(const buffer_t& blob, std::string key, storage_func func);

  // In the event that LiveUpdate::exec() fails,
  // call this function in the C++ exception catch scope:
  static void restore_environment();

  // Only store user data, as if there was a live update process
  // Throws exception if process or sanity checks fail
  static buffer_t store();

  // Returns true if there is stored data from before.
  // It performs an extensive validation process to make sure the data is
  // complete and consistent
  static bool is_resumable();
  static bool is_resumable(const void* location);

  // Restore existing state for a partition named @key.
  // Returns false if there was no such partition
  // Can throw lots of standard exceptions
  static bool resume(std::string key, resume_func handler);

  // Check whether a partition named @key exists at default update location.
  // When @storage_area is nullptr (default) the area is retrieved from OS
  static bool partition_exists(const std::string& key, const void* storage_area = nullptr) noexcept;

  // When explicitly resuming from heap, heap overrun checks are disabled
  static void resume_from_heap(void* location, std::string key, resume_func);

  // Retrieve the recorded length, in bytes, of a valid storage area
  // Throws std::runtime_error when something bad happens
  // Never returns zero
  static size_t stored_data_length(const void* storage_area = nullptr);

  // Set location of known good blob to rollback to if something happens
  static void set_rollback_blob(const void*, size_t) noexcept;
  // Returns true if a backup rollback blob has been set
  static bool has_rollback_blob() noexcept;
  // Immediately start a rollback, not saving any state other than
  // performing a soft-reset to reduce downtime to a minimum
  // Never returns, and upon failure intentionally hard-resets the OS
  static void rollback_now(const char* reason);
  // Returns if the state in the OS has been set to being "liveupdated"
  static bool os_is_liveupdated() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

// IMPORTANT:
// Calls to resume can fail even if is_resumable validates everything correctly.
// this is because when the user restores all the saved data, it could grow into
// the storage area used by liveupdate, if enough data is stored, and corrupt it.
// All failures are of type std::runtime_error. Make sure to give VM enough RAM!

////////////////////////////////////////////////////////////////////////////////

/**
 * The Storage object is passed to the user from the handler given to the
 * call to exec(), starting the liveupdate process. When the handler is
 * called the system is ready to serialize data into the given @location.
 * By using the various add_* functions, the user stores data with @uid
 * as a marker to be able to recognize the object when restoring data.
 * IDs don't have to have specific values, and the user is free to use any value.
 *
 * When using the add() function, the type cannot be verified on the other side,
 * simply because type_info isn't guaranteed to work across updates. A new update
 * could have been compiled with a different compiler.
 *
**/
struct Storage
{
  typedef net::tcp::Connection_ptr Connection_ptr;
  typedef uint16_t uid;

  template <typename T>
  inline void add(uid, const T& type);

  // storing as int saves some storage space compared to all the other types
  void add_int   (uid, int value);
  void add_string(uid, const std::string&);
  void add_buffer(uid, const buffer_t&);
  void add_buffer(uid, const void*, size_t length);
  // store vectors of PODs or std::string
  template <typename T>
  inline void add_vector(uid, const std::vector<T>& vector);
  // store a TCP connection
  void add_connection(uid, Connection_ptr);
  // store a Stream, but not its underlying transport
  // NOTE: UID is taken and used to determine its underlying type
  void add_stream(net::Stream&);

  // markers are used to delineate the end of variable-length structures
  void put_marker(uid);

  Storage(storage_header& sh) : hdr(sh) {}
private:
  void add_vector (uid, const void*, size_t count, size_t element_size);
  void add_string_vector (uid, const std::vector<std::string>&);
  storage_header& hdr;
};

/**
 * A Restore object is given to the user in a restore handler,
 * during the resume() process. The user should know what type
 * each id is, and call the correct as_* function. The object
 * will still be validated, and an error is thrown if there was
 * a type mismatch in some cases.
 *
 * Use go_next() on the Restore& object to go to the next
 * deserializable object in the storage.
 *
**/
struct Restore
{
  typedef net::tcp::Connection_ptr Connection_ptr;

  bool  is_end()    const noexcept;
  bool  is_marker() const noexcept;
  bool  is_int()    const noexcept;
  bool  is_string() const noexcept;
  bool  is_buffer() const noexcept;
  bool  is_vector() const noexcept; // not for string-vector
  bool  is_string_vector() const noexcept; // for string-vector
  bool  is_stream() const noexcept;

  int             as_int()    const;
  std::string     as_string() const;
  buffer_t        as_buffer() const;
  Connection_ptr  as_tcp_connection(net::TCP&) const;
  net::Stream_ptr as_tcp_stream    (net::TCP&) const;
  // TLS streams require: 1. a context to restore
  // 2. select whether or not its an outgoing or incoming connection
  // 3. provide the underlying transport stream, for example a TCP stream
  net::Stream_ptr as_tls_stream(void* ctx, bool outgoing, net::Stream_ptr tr);

  template <typename S>
  inline const S& as_type() const;

  template <typename T>
  inline std::vector<T> as_vector() const;

  int16_t     get_type() const noexcept;
  uint16_t    get_id()   const noexcept;
  int         length()   const noexcept;
  const void* data()     const noexcept;
  uint16_t    next_id()  const noexcept;
  // go to the next storage entry
  void        go_next();

  // go *past* the first marker found (or end reached)
  // if a marker is found, the markers id is returned
  // if the end is reached, 0 is returned
  uint16_t pop_marker();
  // go *past* the first marker found (or end reached)
  // if a marker is found, verify that @id matches
  // if the end is reached, @id is not used
  void pop_marker(uint16_t id);

  // NOTE:
  // it is safe to immediately use is_end() after any call to:
  // go_next(), pop_marker(), pop_marker(uint16_t)

  Restore(storage_entry* ptr) : ent(ptr) {}
private:
  const void* get_segment(size_t, size_t&) const;
  std::vector<std::string> rebuild_string_vector() const;
  storage_entry* ent;
};

/// various inline functions

template <typename S>
inline const S& Restore::as_type() const {
  if (sizeof(S) != length()) {
    throw std::runtime_error("Mismatching length for id " + std::to_string(get_id()));
  }
  return *reinterpret_cast<const S*> (data());
}
template <typename T>
inline std::vector<T> Restore::as_vector() const
{
  size_t count = 0;
  auto*  first = (T*) get_segment(sizeof(T), count);
  return std::vector<T> (first, first + count);
}
template <>
inline std::vector<std::string> Restore::as_vector() const
{
  return rebuild_string_vector();
}

template <typename T>
inline void Storage::add(uid id, const T& thing)
{
  add_buffer(id, &thing, sizeof(T));
}
template <typename T>
inline void Storage::add_vector(uid id, const std::vector<T>& vector)
{
  add_vector(id, vector.data(), vector.size(), sizeof(T));
}
template <>
inline void Storage::add_vector(uid id, const std::vector<std::string>& vector)
{
  add_string_vector(id, vector);
}

class liveupdate_exec_success : public std::exception
{
public:
    const char* what() const throw() {
      return "LiveUpdate::exec() success";
    }
};

} // liu

#endif
