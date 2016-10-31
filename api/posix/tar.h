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

#ifdef __cplusplus
}
#endif
#endif
