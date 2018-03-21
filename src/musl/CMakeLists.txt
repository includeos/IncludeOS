
set(MUSL_OBJECTS
  futex.cpp close.cpp fcntl.cpp lseek.cpp ioctl.cpp writev.cpp
  write.cpp brk.cpp madvise.cpp mmap.cpp mremap.cpp munmap.cpp
  lseek.cpp sched_getaffinity.cpp sysinfo.cpp prlimit64.cpp
  getrlimit.cpp sched_yield.cpp set_robust_list.cpp
  nanosleep.cpp open.cpp creat.cpp clock_gettime.cpp gettimeofday.cpp
  poll.cpp exit.cpp close.cpp set_tid_address.cpp
  pipe.cpp read.cpp readv.cpp getpid.cpp mknod.cpp sync.cpp msync.cpp
  mincore.cpp syscall_n.cpp sigmask.cpp gettid.cpp tkill.cpp
  stat.cpp fstat.cpp fstatat.cpp
  chmod.cpp
  cwd.cpp
  fchmod.cpp
  fchmodat.cpp
  getdents.cpp
  mkdir.cpp
  mkdirat.cpp
  mknodat.cpp
  openat.cpp
  umask.cpp
  utimensat.cpp
  uname.cpp
  access.cpp
)

add_library(musl_syscalls STATIC ${MUSL_OBJECTS})
add_dependencies(musl_syscalls PrecompiledLibraries)

install(TARGETS musl_syscalls DESTINATION includeos/${ARCH}/lib)
