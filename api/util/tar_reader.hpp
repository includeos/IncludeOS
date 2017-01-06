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
#ifndef TAR_READER_HPP
#define TAR_READER_HPP

#include <posix/tar.h>        // Our posix header has the Tar_header struct, which the newlib tar.h does not
#include <util/tinf.h>        // From uzlib (mod)
#include <util/crc32.hpp>

#include <string>
#include <vector>
#include <stdexcept>

extern char _binary_input_bin_start;
extern uintptr_t _binary_input_bin_size;

namespace tar {

const int SECTOR_SIZE = 512;
const int DECOMPRESSION_SIZE = 256;

static bool has_uzlib_init = false;

// --------------------------- Tar_exception ---------------------------

class Tar_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

// --------------------------- Content_block ---------------------------

struct Content_block {
  char block[SECTOR_SIZE];
};

// ------------------------------ Element ------------------------------

// Element/file/folder in tarball
class Element {

public:
  Element(Tar_header& header)
    : header_{header} {}

  Element(Tar_header& header, const char* content_start)
    : header_{header}, content_start_{content_start} {}

  /*Element(Tar_header& header, const std::vector<Content_block*>& content)
    : header_{header}, content_{content} {}*/

  const Tar_header& header() const { return header_; }
  void set_header(const Tar_header& header) { header_ = header; }

  //const std::vector<Content_block*>& content() const { return content_; }
  const char* content() const { return content_start_; }

  /*void add_content_block(Content_block* content_block) {
    content_.push_back(content_block);
  }*/

  void set_content_start(const char* content_start) { content_start_ = content_start; }

  //const char* content_start() { return content_.at(0)->block; }
  //const char* content_start() { return content_start_; }

  //const Content_block* last_block() { return content_.at(content_.size() - 1); }
  const char* last_block() { return content_start_ + (SECTOR_SIZE * num_content_blocks()); }

  std::string name() const { return std::string{header_.name}; }
  std::string mode() const { return std::string{header_.mode}; }
  std::string uid() const { return std::string{header_.uid}; }
  std::string gid() const { return std::string{header_.gid}; }
  long int size();
  std::string mod_time() const { return std::string{header_.mod_time, LENGTH_MTIME}; }
  std::string checksum() const { return std::string{header_.checksum}; }
  char typeflag() const { return header_.typeflag; }
  std::string linkname() { return std::string{header_.linkname}; }
  std::string magic() const { return std::string{header_.magic}; }
  std::string version() const { return std::string{header_.version, LENGTH_VERSION}; }
  std::string uname() const { return std::string{header_.uname}; }
  std::string gname() const { return std::string{header_.gname}; }
  std::string devmajor() const { return std::string{header_.devmajor}; }
  std::string devminor() const { return std::string{header_.devminor}; }
  std::string prefix() const { return std::string{header_.prefix}; }
  std::string pad() const { return std::string{header_.pad}; }

  bool is_ustar() const { return header_.magic == TMAGIC; }

  bool is_dir() const { return header_.typeflag == DIRTYPE; }

  bool typeflag_is_set() const { return header_.typeflag == ' '; }

  //bool is_empty() const { return content_.size() == 0; }
  bool is_empty() { return num_content_blocks() == 0; }
  bool has_content() const { return content_start_ not_eq nullptr; }

  //int num_content_blocks() const { return content_.size(); }
  int num_content_blocks() {
    int num_blocks = 0;

    if (not typeflag_is_set() or not is_dir()) {
      long int sz = size();

      if (sz % SECTOR_SIZE not_eq 0)
        num_blocks = (sz / SECTOR_SIZE) + 1;
      else
        num_blocks = (sz / SECTOR_SIZE);
    }

    return num_blocks;
  }

private:
  Tar_header& header_;

  //std::vector<Content_block*> content_;

  const char* content_start_ = nullptr;
};

// --------------------------------- Tar ---------------------------------

class Tar {

public:
  Tar() = default;
  int num_elements() const { return elements_.size(); }
  void add_element(const Element& element) { elements_.push_back(element); }
  const Element element(const std::string& path) const;
  const std::vector<Element>& elements() const { return elements_; }
  std::vector<std::string> element_names() const;

private:
  std::vector<Element> elements_;

};  // class Tar

// ------------------------------ Tar_reader ------------------------------

class Tar_reader {

public:
  uint32_t checksum(Element& element) { return crc32(element.content(), element.size()); }

  unsigned int decompressed_length(const char* data, size_t size);
  unsigned int decompressed_length(Element& element);

  Tar& read_binary_tar() {
    const char* bin_content   = &_binary_input_bin_start;
    const int   bin_size      = (intptr_t) &_binary_input_bin_size;

    return read_uncompressed(bin_content, bin_size);
  }

  Tar& read_binary_tar_gz() {
    const char* bin_content  = &_binary_input_bin_start;
    const int   bin_size     = (intptr_t) &_binary_input_bin_size;

    return decompress(bin_content, bin_size);
  }

  /**
    dlen is only given if the data is from an Element - see decompress(Element&)
  */
  Tar& decompress(const char* data, size_t size, unsigned int dlen = 0) {
    if (!has_uzlib_init) {
      uzlib_init();
      has_uzlib_init = true;
    }

    // If not decompressing an Element:
    if (dlen == 0)
      dlen = decompressed_length(data, size);

    TINF_DATA d;
    d.source = reinterpret_cast<const unsigned char*>(data);

    int res = uzlib_gzip_parse_header(&d);

    if (res != TINF_OK)
      throw Tar_exception(std::string{"Error parsing header: " + std::to_string(res)});

    uzlib_uncompress_init(&d, NULL, 0);

    // TODO: DELETE IF NEW
    auto* dest = new uint8_t[dlen];
    // unsigned char dest[dlen];
    // If read_uncompressed can be shortened the data can just be added to the tar_ here after decompression
    // instead of calling return read_uncompressed(reinterpret_cast<const char*>(dest), dlen);

    d.dest = dest;

    // decompress byte by byte or any other length
    d.destSize = DECOMPRESSION_SIZE;

    printf("Decompression started - waiting...\n");

    do {
      res = uzlib_uncompress_chksum(&d);
    } while (res == TINF_OK);

    if (res not_eq TINF_DONE)
      throw Tar_exception(std::string{"Error during decompression. Res: " + std::to_string(res)});

    printf("Decompressed %d bytes\n", d.dest - dest);

    return read_uncompressed(reinterpret_cast<const char*>(dest), dlen);

/*
    const char* content = reinterpret_cast<const char*>(dest);

    // Read the uncompressed data into tar_
    if (dlen == 0)
      throw Tar_exception("File is empty");

    if (dlen % SECTOR_SIZE not_eq 0)
      throw Tar_exception("Invalid size of tar file");

    // Go through the whole tar file block by block
    for (size_t i = 0; i < dlen; i += SECTOR_SIZE) {
      Tar_header* header = (Tar_header*) (content + i);
      Element element{*header};

      if (element.name().empty()) {
        // Empty header name - continue
        continue;
      }

      // Check if this is a directory or not (typeflag) (directories have no content)
      // If typeflag not set -> is not a directory and has content
      if (not element.typeflag_is_set() or not element.is_dir()) {
        i += SECTOR_SIZE; // move past header
        element.set_content_start(content + i);
        i += (SECTOR_SIZE * (element.num_content_blocks() - 1));  // move to the end of the element
      }

      tar_.add_element(element);
    }

    return tar_;
*/
  }

  /* When have a tar.gz file inside a tar file f.ex. */
  Tar& decompress(Element& element) {
    return decompress(element.content(), element.size(), decompressed_length(element));
  }

  Tar& read_uncompressed(const char* data, size_t size);

private:
  Tar tar_;

};  // < class Tar_reader

};  // namespace tar

#endif
