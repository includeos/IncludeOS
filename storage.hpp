#pragma once
#include <cstdint>
#include <string>
#include <delegate>

enum storage_type
{
  TYPE_END = 0,
  TYPE_STRING,
  TYPE_BUFFER,
  TYPE_VECTOR,
  TYPE_TCP,
  TYPE_UDP,
};

struct segmented_entry
{
  size_t     count;
  size_t     esize;
  char       vla[0];
};

struct storage_entry
{
  storage_entry(int16_t type, uint16_t id, int length);
  storage_entry(int16_t type);
  
  int16_t   type = TYPE_END;
  uint16_t  id   = 0;
  int       len  = 0;
  char      vla[0];
  
  int size() const noexcept {
    return sizeof(storage_entry) + len;
  }
  segmented_entry& get_segs() noexcept {
    return *(segmented_entry*) vla;
  }
  const char* data() const noexcept {
    return vla;
  }
  
  storage_entry* next() const noexcept;
  uint32_t       checksum() const;
};

struct storage_header
{
  typedef delegate<int(char*)> construct_func;
  static const uint64_t  LIVEUPD_MAGIC;
  
  uint32_t get_entries() const noexcept {
    return this->entries;
  }
  
  storage_header();
  
  void add_string(uint16_t id, const std::string& data);
  void add_buffer(uint16_t id, const char*, int);
  storage_entry& add_struct(int16_t type, uint16_t id, int length);
  storage_entry& add_struct(int16_t type, uint16_t id, construct_func);
  void add_vector(uint16_t, const void*, size_t cnt, size_t esize);
  void add_end();
  
  storage_entry* begin();
  storage_entry* next(storage_entry*);
  
  template <typename... Args>
  storage_entry& create_entry(Args&&... args);
  
  inline storage_entry&
  var_entry(int16_t type, uint16_t id, construct_func func);
  
  void append_eof() noexcept {
    ((storage_entry*) &vla[length])->type = TYPE_END;
  }
  void finalize();
  bool validate() noexcept;
  
  // zero out the entire header and its data, for extra security
  void zero();
  
private:
  uint32_t generate_checksum() noexcept;
  
  uint64_t magic;
  uint32_t crc;
  uint32_t entries = 0;
  uint32_t length  = 0;
  char     vla[0];
};

template <typename... Args>
inline storage_entry&
storage_header::create_entry(Args&&... args)
{
  // create entry
  auto* entry = (storage_entry*) &vla[length];
  new (entry) storage_entry(args...);
  // next storage_entry will be this much further out:
  this->length += entry->size();
  this->entries++;
  // make sure storage is properly EOF'd
  this->append_eof();
  return *entry;
}

inline storage_entry&
storage_header::var_entry(int16_t type, uint16_t id, construct_func func)
{
  // create entry
  auto* entry = (storage_entry*) &vla[length];
  new (entry) storage_entry(type, id, 0);
  // determine and set size of entry
  int size = func(entry->vla);
  entry->len = size;
  // next storage_entry will be this much further out:
  this->length += entry->size();
  this->entries++;
  // make sure storage is properly EOF'd
  this->append_eof();
  return *entry;
}
