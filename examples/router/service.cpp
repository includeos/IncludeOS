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

#include <service>
#include <net/router>

void Service::start()
{
  auto& router = net::get_router();

  auto& eth0 = net::Super_stack::get<net::IP4>(0);
  auto& eth1 = net::Super_stack::get<net::IP4>(1);

  eth0.set_forward_delg(router.forward_delg());
  eth1.set_forward_delg(router.forward_delg());
}
