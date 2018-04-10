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

#pragma once
#ifndef TAR_HPP
#define TAR_HPP

#include <tar.h>
#include <tinf.h>             // From uzlib (mod)
#include <util/crc32.hpp>
#include <info>

#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <cstring>

extern uint8_t _binary_input_bin_start;
extern uintptr_t _binary_input_bin_size;

#define LENGTH_NAME 100
#define LENGTH_MODE 8
#define LENGTH_UID 8
#define LENGTH_GID 8
#define LENGTH_SIZE 12
#define LENGTH_MTIME 12
#define LENGTH_CHECKSUM 8
#define LENGTH_TYPEFLAG 1
#define LENGTH_LINKNAME 100
#define LENGTH_MAGIC 6
#define LENGTH_VERSION 2
#define LENGTH_UNAME 32
#define LENGTH_GNAME 32
#define LENGTH_DEVMAJOR 8
#define LENGTH_DEVMINOR 8
#define LENGTH_PREFIX 155
#define LENGTH_PAD 12

struct Tar_header {
  char name[LENGTH_NAME];             // Name of header file entry
  char mode[LENGTH_MODE];             // Permission and mode bits
  char uid[LENGTH_UID];               // User ID of owner
  char gid[LENGTH_GID];               // Group ID of owner
  char size[LENGTH_SIZE];             // File size in bytes (octal base)
  char mod_time[LENGTH_MTIME];        // Last modification time in numeric Unix time format (octal)
  char checksum[LENGTH_CHECKSUM];     // Checksum for header record (6 digit octal number with leading zeroes)
  char typeflag;                      // Type of header entry
  char linkname[LENGTH_LINKNAME];     // Target name of link
  char magic[LENGTH_MAGIC];           // Ustar indicator
  char version[LENGTH_VERSION];       // Ustar version
  char uname[LENGTH_UNAME];           // User name of owner
  char gname[LENGTH_GNAME];           // Group name of owner
  char devmajor[LENGTH_DEVMAJOR];     // Major number of character or block device
  char devminor[LENGTH_DEVMINOR];     // Minor number of character or block device
  char prefix[LENGTH_PREFIX];         // Prefix for file name
  char pad[LENGTH_PAD];
};

namespace tar {

static const int SECTOR_SIZE = 512;
static const int DECOMPRESSION_SIZE = 256;

static bool has_uzlib_init = false;

using Tar_data = std::vector<uint8_t>;

// --------------------------- Tar_exception ---------------------------

class Tar_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

// ------------------------------ Element ------------------------------

// Element/file/folder in tarball
class Element {

public:
  Element(Tar_header& header)
    : header_{header} {}

  Element(Tar_header& header, const uint8_t* content_start)
    : header_{header}, content_start_{content_start} {}

  const Tar_header& header() const noexcept { return header_; }
  void set_header(const Tar_header& header) { header_ = header; }

  const uint8_t* content() const { return content_start_; }
  void set_content_start(const uint8_t* content_start) { content_start_ = content_start; }

  std::string name() const { return std::string{header_.name}; }
  std::string mode() const { return std::string{header_.mode}; }
  std::string uid() const { return std::string{header_.uid}; }
  std::string gid() const { return std::string{header_.gid}; }
  long int size() const noexcept;
  std::string mod_time() const { return std::string{header_.mod_time, LENGTH_MTIME}; }
  std::string checksum() const { return std::string{header_.checksum}; }
  char typeflag() const noexcept { return header_.typeflag; }
  std::string linkname() const { return std::string{header_.linkname}; }
  std::string magic() const { return std::string{header_.magic}; }
  std::string version() const { return std::string{header_.version, LENGTH_VERSION}; }
  std::string uname() const { return std::string{header_.uname}; }
  std::string gname() const { return std::string{header_.gname}; }
  std::string devmajor() const { return std::string{header_.devmajor}; }
  std::string devminor() const { return std::string{header_.devminor}; }
  std::string prefix() const { return std::string{header_.prefix}; }
  std::string pad() const { return std::string{header_.pad}; }

  bool is_ustar() const noexcept { return (strncmp(header_.magic, TMAGIC, TMAGLEN) == 0); }
  bool is_dir() const noexcept { return header_.typeflag == DIRTYPE; }
  bool typeflag_is_set() const noexcept { return header_.typeflag not_eq ' '; }
  bool is_empty() const noexcept { return size() == 0; }
  bool is_tar_gz() const noexcept;

  int num_content_blocks() const noexcept {
    int num_blocks = 0;
    const long int sz = size();

    if (sz % SECTOR_SIZE not_eq 0)
      num_blocks = (sz / SECTOR_SIZE) + 1;
    else
      num_blocks = (sz / SECTOR_SIZE);

    return num_blocks;
  }

private:
  Tar_header& header_;
  const uint8_t* content_start_ = nullptr;
};

// --------------------------------- Tar ---------------------------------

class Tar {

public:
  using Elements = std::vector<Element>;
  Tar() = default;
  int num_elements() const noexcept { return elements_.size(); }
  void add_element(const Element& element) { elements_.push_back(element); }
  const Element& element(const std::string& path) const;
  const Elements& elements() const noexcept { return elements_; }
  std::vector<std::string> element_names() const;

  auto begin() const noexcept
  { return elements_.begin(); }

  auto end() const noexcept
  { return elements_.end(); }

private:
  std::vector<Element> elements_;

};  // class Tar

// ------------------------------ Reader ------------------------------

class Reader {
public:
  static Tar read_tar()
  {
    const uint8_t* bin_content   = &_binary_input_bin_start;
    const int   bin_size      = (intptr_t) &_binary_input_bin_size;
    return read(bin_content, bin_size);
  }

  static Tar_data decompress_tar()
  {
    const uint8_t* bin_content   = &_binary_input_bin_start;
    const int   bin_size      = (intptr_t) &_binary_input_bin_size;
    return decompress(bin_content, bin_size);
  }

  static Tar read(const uint8_t* data, const size_t size);

  /**
    dlen is only given if the data is from an Element - see decompress(Element&)
  */
  static Tar_data decompress(const uint8_t* data,
                             const size_t size,
                             unsigned int dlen = 0);

  /* When have a tar.gz file inside a tar file f.ex. */
  static Tar_data decompress(const Element& element)
  {
    return decompress(element.content(), element.size(),
      decompressed_length(element.content(), element.size()));
  }

  static uint32_t checksum(const Element& element)
  { return crc32(element.content(), element.size()); }

  static unsigned int decompressed_length(const uint8_t* data, const size_t size);

};  // < class Reader

};  // namespace tar

#endif
