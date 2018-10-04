// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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
