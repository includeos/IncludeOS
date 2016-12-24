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

#ifndef POSIX_CPIO_H
#define POSIX_CPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define C_IRUSR   0000400
#define C_IWUSR   0000200
#define C_IXUSR   0000100
#define C_IRGRP   0000040
#define C_IWGRP   0000020
#define C_IXGRP   0000010
#define C_IROTH   0000004
#define C_IWOTH   0000002
#define C_IXOTH   0000001
#define C_ISUID   0004000
#define C_ISGID   0002000
#define C_ISVTX   0001000
#define C_ISDIR   0040000
#define C_ISFIFO  0010000
#define C_ISREG   0100000
#define C_ISBLK   0060000
#define C_ISCHR   0020000
#define C_ISCTG   0110000
#define C_ISLNK   0120000
#define C_ISSOCK  0140000

#define MAGIC     "070707"

#ifdef __cplusplus
}
#endif
#endif
