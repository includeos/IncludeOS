// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


#pragma once
#include <list>
#include <memory>
#include <iostream>

class PacketStore
{
public:
  std::shared_ptr<net::Packet> getPacket()
  {
    if (_queue.empty())
    {
      std::cout << "<PacketStore> Error: out of packets" << std::endl;
      panic("PacketStore: Out of packets");
    }
    
    auto elt = *_queue.begin();
    _queue.pop_front();
    _queue.push_back(elt);
    return elt;
  }
  
  PacketStore(uint16_t n, uint32_t size)
    : _n(n), _bufsize(size), _pool(nullptr)
  {       
    this->_pool = new uint8_t[n * size];
    
    for(int i = 0; i < _n; i++)
    {
      _queue.push_back
        (std::make_shared<net::Packet>(&_pool[i * size], size, net::Packet::AVAILABLE));
    }
    
    std::cout << "<PacketStore> Allocated " << n*size << " byte buffer pool for packets" << std::endl;
  }
  
private:
	uint16_t _n;
	uint32_t _bufsize;
	uint8_t* _pool;
	std::list<std::shared_ptr<net::Packet> > _queue;
};
