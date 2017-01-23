// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define PCV(x) print_conf_value(x, #x)

static void print_conf_value(int name, const char* sym) {
  errno = 0;
  long value = sysconf(name);
  if (value == -1) {
    errno == 0 ? printf("%s (%d): No limit\n", sym, name)
      : printf("%s sysconf error: %s\n", sym, strerror(errno));
  }
  else {
    printf("%s (%d): %ld\n", sym, name, value);
  }
}

void test_sysconf() {
  PCV(_SC_AIO_LISTIO_MAX);
  PCV(_SC_AIO_MAX);
  PCV(_SC_AIO_PRIO_DELTA_MAX);
  PCV(_SC_ARG_MAX);
  PCV(_SC_ATEXIT_MAX);
  PCV(_SC_BC_BASE_MAX);
  PCV(_SC_BC_DIM_MAX);
  PCV(_SC_BC_SCALE_MAX);
  PCV(_SC_BC_STRING_MAX);
  PCV(_SC_CHILD_MAX);
  PCV(_SC_CLK_TCK);
  PCV(_SC_COLL_WEIGHTS_MAX);
  PCV(_SC_DELAYTIMER_MAX);
  PCV(_SC_EXPR_NEST_MAX);
  PCV(_SC_HOST_NAME_MAX);
  PCV(_SC_IOV_MAX);
  PCV(_SC_LINE_MAX);
  PCV(_SC_LOGIN_NAME_MAX);
  PCV(_SC_NGROUPS_MAX);
  PCV(_SC_GETGR_R_SIZE_MAX);
  PCV(_SC_GETPW_R_SIZE_MAX);
  PCV(_SC_MQ_OPEN_MAX);
  PCV(_SC_MQ_PRIO_MAX);
  PCV(_SC_OPEN_MAX);
  PCV(_SC_PAGE_SIZE);
  PCV(_SC_PAGESIZE);
  PCV(_SC_THREAD_DESTRUCTOR_ITERATIONS);
  PCV(_SC_THREAD_KEYS_MAX);
  PCV(_SC_THREAD_STACK_MIN);
  PCV(_SC_THREAD_THREADS_MAX);
  PCV(_SC_RE_DUP_MAX);
  PCV(_SC_RTSIG_MAX);
  PCV(_SC_SEM_NSEMS_MAX);
  PCV(_SC_SEM_VALUE_MAX);
  PCV(_SC_SIGQUEUE_MAX);
  PCV(_SC_STREAM_MAX);
  PCV(_SC_SYMLOOP_MAX);
  PCV(_SC_TIMER_MAX);
  PCV(_SC_TTY_NAME_MAX);
  PCV(_SC_TZNAME_MAX);
  PCV(_SC_ADVISORY_INFO);
  PCV(_SC_BARRIERS);
  PCV(_SC_ASYNCHRONOUS_IO);
  PCV(_SC_CLOCK_SELECTION);
  PCV(_SC_CPUTIME);
  PCV(_SC_FSYNC);
  PCV(_SC_IPV6);
  PCV(_SC_JOB_CONTROL);
  PCV(_SC_MAPPED_FILES);
  PCV(_SC_MEMLOCK);
  PCV(_SC_MEMLOCK_RANGE);
  PCV(_SC_MEMORY_PROTECTION);
  PCV(_SC_MESSAGE_PASSING);
  PCV(_SC_MONOTONIC_CLOCK);
  PCV(_SC_PRIORITIZED_IO);
  PCV(_SC_PRIORITY_SCHEDULING);
  PCV(_SC_RAW_SOCKETS);
  PCV(_SC_READER_WRITER_LOCKS);
  PCV(_SC_REALTIME_SIGNALS);
  PCV(_SC_REGEXP);
  PCV(_SC_SAVED_IDS);
  PCV(_SC_SEMAPHORES);
  PCV(_SC_SHARED_MEMORY_OBJECTS);
  PCV(_SC_SHELL);
  PCV(_SC_SPAWN);
  PCV(_SC_SPIN_LOCKS);
  PCV(_SC_SPORADIC_SERVER);
  PCV(_SC_SS_REPL_MAX);
  PCV(_SC_SYNCHRONIZED_IO);
  PCV(_SC_THREAD_ATTR_STACKADDR);
  PCV(_SC_THREAD_ATTR_STACKSIZE);
  PCV(_SC_THREAD_CPUTIME);
  PCV(_SC_THREAD_PRIO_INHERIT);
  PCV(_SC_THREAD_PRIO_PROTECT);
  PCV(_SC_THREAD_PRIORITY_SCHEDULING);
  PCV(_SC_THREAD_PROCESS_SHARED);
  #ifdef _SC_THREAD_ROBUST_PRIO_INHERIT
  PCV(_SC_THREAD_ROBUST_PRIO_INHERIT);
  #endif
  #ifdef _SC_THREAD_ROBUST_PRIO_PROTECT
  PCV(_SC_THREAD_ROBUST_PRIO_PROTECT);
  #endif
  PCV(_SC_THREAD_SAFE_FUNCTIONS);
  PCV(_SC_THREAD_SPORADIC_SERVER);
  PCV(_SC_THREADS);
  PCV(_SC_TIMEOUTS);
  PCV(_SC_TIMERS);
  PCV(_SC_TRACE);
  PCV(_SC_TRACE_EVENT_FILTER);
  PCV(_SC_TRACE_EVENT_NAME_MAX);
  PCV(_SC_TRACE_INHERIT);
  PCV(_SC_TRACE_LOG);
  PCV(_SC_TRACE_NAME_MAX);
  PCV(_SC_TRACE_SYS_MAX);
  PCV(_SC_TRACE_USER_EVENT_MAX);
  PCV(_SC_TYPED_MEMORY_OBJECTS);
  PCV(_SC_VERSION);
  #ifdef _SC_V7_ILP32_OFF32
  PCV(_SC_V7_ILP32_OFF32);
  #endif
  #ifdef _SC_V7_ILP32_OFFBIG
  PCV(_SC_V7_ILP32_OFFBIG);
  #endif
  #ifdef _SC_V7_LP64_OFF64
  PCV(_SC_V7_LP64_OFF64);
  #endif
  #ifdef _SC_V7_LPBIG_OFFBIG
  PCV(_SC_V7_LPBIG_OFFBIG);
  #endif
  PCV(_SC_V6_ILP32_OFF32);
  PCV(_SC_V6_ILP32_OFFBIG);
  PCV(_SC_V6_LP64_OFF64);
  PCV(_SC_V6_LPBIG_OFFBIG);
  PCV(_SC_2_C_BIND);
  PCV(_SC_2_C_DEV);
  PCV(_SC_2_CHAR_TERM);
  PCV(_SC_2_FORT_DEV);
  PCV(_SC_2_FORT_RUN);
  PCV(_SC_2_LOCALEDEF);
  PCV(_SC_2_PBS);
  PCV(_SC_2_PBS_ACCOUNTING);
  PCV(_SC_2_PBS_CHECKPOINT);
  PCV(_SC_2_PBS_LOCATE);
  PCV(_SC_2_PBS_MESSAGE);
  PCV(_SC_2_PBS_TRACK);
  PCV(_SC_2_SW_DEV);
  PCV(_SC_2_UPE);
  PCV(_SC_2_VERSION);
  PCV(_SC_XOPEN_CRYPT);
  PCV(_SC_XOPEN_ENH_I18N);
  PCV(_SC_XOPEN_REALTIME);
  PCV(_SC_XOPEN_REALTIME_THREADS);
  PCV(_SC_XOPEN_SHM);
  PCV(_SC_XOPEN_STREAMS);
  PCV(_SC_XOPEN_UNIX);
  #ifdef _SC_XOPEN_UUCP
  PCV(_SC_XOPEN_UUCP);
  #endif
  PCV(_SC_XOPEN_VERSION);
}
