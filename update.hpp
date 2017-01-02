/**
 * 
 * Live Update
 * 
 * Master Thesis by Alf-Andre Walla
 * 
**/
#pragma once
#ifndef LIVEUPDATE_UPDATE_HPP
#define LIVEUPDATE_UPDATE_HPP

#include <net/tcp/connection.hpp>
#include <delegate>
#include <vector>

struct buffer_len {
  const char* buffer;
  int length;
  
  buffer_len deep_copy() const;
};

struct storage_entry;
struct storage_header;

struct Storage
{
  typedef net::tcp::Connection_ptr Connection_ptr;
  typedef uint16_t uid;

  template <typename T>
  inline void add(uid, const T& type);

  void add_string(uid, const std::string&);
  void add_strings(uid, const std::vector<std::string>&);
  void add_buffer(uid, buffer_len);
  void add_buffer(uid, const void*, size_t);
  void add_vector(uid, const void*, size_t count, size_t element_size);

  template <typename T>
  inline void add_vector(uid, const std::vector<T>& vector);

  void add_connection(uid, Connection_ptr);
  
  Storage(storage_header& sh) : hdr(sh) {}
  
private:
  storage_header& hdr;
};

struct Restore
{
  typedef net::tcp::Connection_ptr Connection_ptr;
  
  std::string    as_string() const;
  buffer_len     as_buffer() const;
  Connection_ptr as_tcp_connection(net::TCP&) const;
  
  template <typename S>
  inline S& as_type() const;

  template <typename T>
  inline std::vector<T> as_vector() const;

  int16_t     get_type() const noexcept;
  uint16_t    get_id() const noexcept;
  int         length() const noexcept;
  const void* data() const noexcept;
  
  bool     is_end()  const noexcept;
  uint16_t next_id() const noexcept;
  void     go_next();
  
  Restore(storage_entry*& ptr) : ent(ptr) {}
  Restore(const Restore&);
private:
  const void* get_segment(size_t, size_t&) const;
  storage_entry*& ent;
};

struct LiveUpdate
{
  typedef delegate<void(Restore)> resume_func;
  typedef delegate<void(Storage, buffer_len)> storage_func;

  // start a live update process
  static void begin(void* location, buffer_len blob, storage_func);
  
  // returns true if there is stored data from before
  static bool is_resumable(void* location);
  
  // register handler for a specific id
  static void on_resume(uint16_t id, resume_func);
  
  // attempt to restore existing payloads
  // returns false if there was nothing there
  static bool resume(void* location, resume_func);
};

template <typename S>
inline S& Restore::as_type() const {
  return *(S*) data();
}
template <typename T>
inline std::vector<T> Restore::as_vector() const
{
  size_t count = 0;
  auto*  first = (T*) get_segment(sizeof(T), count);
  return std::vector<T> (first, first + count);
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

#endif
