
#include <posix/unix_fd.hpp>
#include <sys/un.h>
#include <fs/vfs.hpp>

long Unix_FD::set_impl_if_needed(const struct sockaddr* addr, socklen_t addrlen)
{
  (void) type_;
  if((addr == nullptr and addrlen == 0) and impl != nullptr)
    return 0;

  if(UNLIKELY(addr == nullptr))
    return -EINVAL;

  const auto& un_addr = reinterpret_cast<const sockaddr_un&>(*addr);

  if(un_addr.sun_family != AF_UNIX)
    return -EAFNOSUPPORT;

  const std::string path{un_addr.sun_path};

  try {
    auto& ent = fs::VFS::get<Impl>(path);
    impl = &ent;
    return 0;
  }
  catch (const fs::Err_not_found& /*e*/) {
    //printf("no ent: %s\n", e.what());
    return -ENOENT;
  }
}

long Unix_FD::connect(const struct sockaddr* addr, socklen_t addrlen)
{
  auto res = set_impl_if_needed(addr, addrlen);
  return (res < 0) ? res : impl->connect(addr, addrlen);
}

ssize_t Unix_FD::sendto(const void* buf, size_t len, int fl,
                             const struct sockaddr* addr,
                             socklen_t addrlen)
{
  auto res = set_impl_if_needed(addr, addrlen);
  return (res < 0) ? res : impl->sendto(buf, len, fl, addr, addrlen);
}

int Unix_FD::close()
{
  if(impl)
  {
    auto res = impl->close();
    impl = nullptr;
    return res;
  }
  return 0;
}
