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

#ifndef POSIX_NL_TYPES_H
#define POSIX_NL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define NL_SETD       1
#define NL_CAT_LOCALE 1

typedef void *nl_catd;
typedef int   nl_item;

int       catclose(nl_catd);
char     *catgets(nl_catd, int, int, const char *);
nl_catd   catopen(const char *, int);

#ifdef __cplusplus
}
#endif

#endif // < POSIX_NL_TYPES_H
