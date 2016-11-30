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
#ifndef TAR_HEADER_HPP
#define TAR_HEADER_HPP

#include <tar.h>  // posix

#include <string>

/*const int OFFSET_NAME = 0;
const int OFFSET_MODE = 100;
const int OFFSET_UID = 108;
const int OFFSET_GID = 116;
const int OFFSET_SIZE = 124;
const int OFFSET_MTIME = 136;
const int OFFSET_CHECKSUM = 148;
const int OFFSET_TYPEFLAG = 156;
const int OFFSET_LINKNAME = 157;
const int OFFSET_MAGIC = 257;
const int OFFSET_VERSION = 263;
const int OFFSET_UNAME = 265;
const int OFFSET_GNAME = 297;
const int OFFSET_DEVMAJOR = 329;
const int OFFSET_DEVMINOR = 337;
const int OFFSET_PREFIX = 345;
const int OFFSET_PAD = 500;

const int LENGTH_NAME = 100;
const int LENGTH_MODE = 8;
const int LENGTH_UID = 8;
const int LENGTH_GID = 8;
const int LENGTH_SIZE = 12;
const int LENGTH_MTIME = 12;
const int LENGTH_CHECKSUM = 8;
const int LENGTH_TYPEFLAG = 1;
const int LENGTH_LINKNAME = 100;
const int LENGTH_MAGIC = 6;
const int LENGTH_VERSION = 2;
const int LENGTH_UNAME = 32;
const int LENGTH_GNAME = 32;
const int LENGTH_DEVMAJOR = 8;
const int LENGTH_DEVMINOR = 8;
const int LENGTH_PREFIX = 155;
const int LENGTH_PAD = 12;*/

// Move to tar.h (posix):

#define OFFSET_NAME 0
#define OFFSET_MODE 100
#define OFFSET_UID 108
#define OFFSET_GID 116
#define OFFSET_SIZE 124
#define OFFSET_MTIME 136
#define OFFSET_CHECKSUM 148
#define OFFSET_TYPEFLAG 156
#define OFFSET_LINKNAME 157
#define OFFSET_MAGIC 257
#define OFFSET_VERSION 263
#define OFFSET_UNAME 265
#define OFFSET_GNAME 297
#define OFFSET_DEVMAJOR 329
#define OFFSET_DEVMINOR 337
#define OFFSET_PREFIX 345
#define OFFSET_PAD 500

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

struct __attribute__((packed)) Tar_header {
  char name[LENGTH_NAME];             // Name of header file entry
  char mode[LENGTH_MODE];             // Permission and mode bits
  char uid[LENGTH_UID];               // User ID of owner
  char gid[LENGTH_GID];               // Group ID of owner
  char size[LENGTH_SIZE];             // File size in bytes
  char mod_time[LENGTH_MTIME];        // Last modification time in numeric Unix time format
  char checksum[LENGTH_CHECKSUM];     // Checksum for header record
  char typeflag;                      // Type of header entry
  char linkname[LENGTH_LINKNAME];     // Target name of link
  char magic[LENGTH_MAGIC];           //
  char version[LENGTH_VERSION];       //
  char uname[LENGTH_UNAME];           // User name of owner
  char gname[LENGTH_GNAME];           // Group name of owner
  char devmajor[LENGTH_DEVMAJOR];     // Major number of character or block device
  char devminor[LENGTH_DEVMINOR];     // Minor number of character or block device
  char prefix[LENGTH_PREFIX];         //
  char pad[LENGTH_PAD];               //
  // Added:
  int first_block_index;
  int num_content_blocks;
};

// A Header represents a single header in a tar archive. Some fields may not be populated
class Header_cplusplus {

public:

  // Getters and setters:

  const std::string& name() { return name_; }
  void set_name(const std::string& name) { name_ = name; }

  const std::string& mode() { return mode_; }
  void set_mode(const std::string& mode) { mode_ = mode; }
/*
  uint64_t mode() { return mode_; }
  void set_mode(uint64_t mode) { mode_ = mode; }
*/

  int uid() { return uid_; }
  void set_uid(int uid) { uid_ = uid; }

  int gid() { return gid_; }
  void set_gid(int gid) { gid_ = gid; }

  const std::string& size() { return size_; }
  void set_size(const std::string& size) { size_ = size; }
/*
  int64_t size() { return size_; }
  void set_size(int64_t size) { size_ = size; }
*/

  const std::string& mod_time() { return mod_time_; }
  void set_mod_time(const std::string& mod_time) { mod_time_ = mod_time; }

  int64_t checksum() { return checksum_; }
  void set_checksum(int64_t checksum) { checksum_ = checksum; }

  char typeflag() { return typeflag_; }
  void set_typeflag(const char typeflag) { typeflag_ = typeflag; }

  const std::string& linkname() { return linkname_; }
  void set_linkname(const std::string& linkname) { linkname_ = linkname; }

  const std::string& magic() { return magic_; }
  void set_magic(const std::string& magic) { magic_ = magic; }

  const std::string version() { return version_; }
  void set_version(const std::string& version) { version_ = version; }

  const std::string& uname() { return uname_; }
  void set_uname(const std::string& uname) { uname_ = uname; }

  const std::string& gname() { return gname_; }
  void set_gname(const std::string& gname) { gname_ = gname; }

  int64_t devmajor() { return devmajor_; }
  void set_devmajor(int64_t devmajor) { devmajor_ = devmajor; }

  int64_t devminor() { return devminor_; }
  void set_devminor(int64_t devminor) { devminor_ = devminor; }

  const std::string prefix() { return prefix_; }
  void set_prefix(const std::string& prefix) { prefix_ = prefix; }

  const std::string pad() { return pad_; }
  void set_pad(const std::string& pad) { pad_ = pad; }

  bool isUstar() { return magic_ == TMAGIC; }

  bool isDir() { return typeflag_ == DIRTYPE; }

  bool isFile() { return typeflag_ == REGTYPE or typeflag_ == AREGTYPE; }

private:
  std::string name_;      // Name of header file entry
  std::string mode_;
  //int64_t mode_;        // Permission and mode bits
  int uid_;               // User ID of owner
  int gid_;               // Group ID of owner
  std::string size_;
  //int64_t size_;        // File size in bytes
  std::string mod_time_;  // Last modification time in numeric Unix time format
  int64_t checksum_;      // Checksum for header record
  char typeflag_;         // Type of header entry
  std::string linkname_;  // Target name of link
  std::string magic_;     //
  std::string version_;   //
  std::string uname_;     // User name of owner
  std::string gname_;     // Group name of owner
  int64_t devmajor_;      // Major number of character or block device
  int64_t devminor_;      // Minor number of character or block device
  std::string prefix_;    //
  std::string pad_;       //
};

#endif
