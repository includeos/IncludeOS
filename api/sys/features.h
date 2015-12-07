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

#ifndef SYS_FEATURES_H
#define SYS_FEATURES_H
  
  // Newlib needs this switch to enable clock_gettime etc.
  // Also, we'll need posix timers sooner or later
#define _POSIX_TIMERS 1

#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(A, B) 0 /* Nei */
#endif


/** Apparently this is necessary in order to build libc++.
    @todo : It creates a warning when building os.a;
    find another way to provide it to libc++.
 */
#ifndef __GNUC_PREREQ__
#define __GNUC_PREREQ__(A, B) __GNUC_PREREQ(A, B)
#endif

#define __GLIBC_PREREQ__(A, B) 1 /* Jo.  */
#define __GLIBC_PREREQ(A, B) 1 /* Jo.  */

#endif
