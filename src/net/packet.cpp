// Part of the IncludeOS unikernel - www.includeos.org
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

//#define DEBUG

#include <os>
#include <net/packet.hpp>

using namespace net;


Packet::~Packet(){
  debug("<Packet> DESTRUCT packet, buf@%p \n", buf_);
  release_(buf_, capacity_);
}


Packet::Packet(uint8_t* buf, size_t bufsize, size_t datalen, release_del rel) 
  : buf_(buf),  capacity_(bufsize), size_(datalen), next_hop4_(), release_(rel)
{
  debug("<Packet> CONSTRUCT packet, buf@%p \n",buf);
}
    

IP4::addr Packet::next_hop(){
  return next_hop4_;
}

void Packet::next_hop(IP4::addr ip){
  next_hop4_ = ip;
}


int Packet::set_size(size_t s){
  // Upstream packets have length set by the interface
  // if(_status == UPSTREAM ) return 0; ... well. DNS reuses packet.
  if(s > capacity_)
    return 0;
  size_ = s;
  return size_;
}

void net::default_release(buffer b, size_t){
  (void) b;
  debug("<Packet DEFAULT RELEASE> Ignoring buffer.");
}


