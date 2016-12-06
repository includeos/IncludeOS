// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef POSIX_TAR_H
#define POSIX_TAR_H

#ifdef __cplusplus
extern "C" {
#endif

#define TMAGIC    "ustar"
#define TMAGLEN   6
#define TVERSION  "00"
#define TVERSLEN  2

#define REGTYPE   '0'
#define AREGTYPE  '\0'
#define LNKTYPE   '1'
#define SYMTYPE   '2'
#define CHRTYPE   '3'
#define BLKTYPE   '4'
#define DIRTYPE   '5'
#define FIFOTYPE  '6'
#define CONTTYPE  '7'

#define TSUID   04000
#define TSGID   02000
#define TSVTX   01000
#define TUREAD  00400
#define TUWRITE 00200
#define TUEXEC  00100
#define TGREAD  00040
#define TGWRITE 00020
#define TGEXEC  00010
#define TOREAD  00004
#define TOWRITE 00002
#define TOEXEC  00001

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

#ifdef __cplusplus
}
#endif
#endif
