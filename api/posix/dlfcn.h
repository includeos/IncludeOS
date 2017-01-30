// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef POSIX_DLFCN_H
#define POSIX_DLFCN_H

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY   1   // Relocations are performed at an implementation-dependent time.
#define RTLD_NOW    2   // Relocations are performed when the object is loaded.
#define RTLD_GLOBAL 3   // All symbols are available for relocation processing of other modules.
#define RTLD_LOCAL  4   // All symbols are not made available for relocation processing by other modules.

void  *dlopen(const char *, int);
void  *dlsym(void *, const char *);
int    dlclose(void *);
char  *dlerror(void);

#ifdef __cplusplus
}
#endif

#endif // < POSIX_DLFCN_H
