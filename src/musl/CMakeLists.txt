
set(MUSL_OBJECTS
  futex.cpp close.cpp fcntl.cpp lseek.cpp ioctl.cpp writev.cpp
  write.cpp brk.cpp madvise.cpp mmap.cpp mremap.cpp munmap.cpp
  lseek.cpp sched_getaffinity.cpp sysinfo.cpp prlimit64.cpp
  getrlimit.cpp sched_yield.cpp set_robust_list.cpp
  nanosleep.cpp open.cpp creat.cpp clock_gettime.cpp gettimeofday.cpp
  poll.cpp exit.cpp close.cpp set_tid_address.cpp
  pipe.cpp read.cpp readv.cpp getpid.cpp getuid.cpp mknod.cpp sync.cpp
  msync.cpp mincore.cpp syscall_n.cpp sigmask.cpp gettid.cpp
  socketcall.cpp rt_sigaction.cpp
  stat.cpp fstat.cpp fstatat.cpp
  access.cpp
  chmod.cpp
  chown.cpp
  cwd.cpp
  dup3.cpp # also dup, dup2
  execve.cpp
  fchmod.cpp
  fchmodat.cpp
  fchown.cpp
  fsync.cpp
  ftruncate.cpp
  getdents.cpp
  geteuid.cpp
  getgid.cpp
  getuid.cpp
  kill.cpp
  mkdir.cpp
  mkdirat.cpp
  mknodat.cpp
  mlock.cpp
  mprotect.cpp
  openat.cpp
  readlink.cpp
  rename.cpp
  rmdir.cpp
  select.cpp
  setgid.cpp
  setpgid.cpp
  setrlimit.cpp
  setsid.cpp
  setuid.cpp
  wait4.cpp
  umask.cpp
  uname.cpp
  utimensat.cpp
  unlink.cpp
)

add_library(musl_syscalls STATIC ${MUSL_OBJECTS})

install(TARGETS musl_syscalls DESTINATION lib)
