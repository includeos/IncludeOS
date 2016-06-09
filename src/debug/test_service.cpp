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

#include <os>
#include <net/inet4>
#include "ircd.hpp"

#include <smp> // SMP class

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

extern "C" {
  char _TEXT_START_;
  char _EH_FRAME_START_;
  char _EH_FRAME_END_;
}

#include <iostream>
#include "../../vmbuild/elf.h"

///
/// https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-PDA/LSB-PDA/ehframechpt.html
///
struct CIE;
struct Frame;

union Entry {
  uint32_t length;
  uint64_t ext_length;
  
  size_t hdr_size() const {
    if (length != UINT_MAX) return 4;
    return 8;
  }
  size_t size() const {
    if (length != UINT_MAX) return length;
    return ext_length;
  }
  CIE* cie() const {
    return (CIE*) (((char*) this) + hdr_size());
  }
  Frame* frame() const {
    return (Frame*) (((char*) this) + hdr_size());
  }
};
struct CIE {
  uint32_t id;
  uint8_t  ver;
  char process_start[0];
  
  bool validate() const {
    return ver == 1 && id == 0;
  }
  
  void process();
  
} __attribute__((packed));

void CIE::process()
{
  printf("id: %u, ver: %u\n",
    id, ver);
  assert(validate());
  printf("CIE validated\n");
  
  char* augstr = process_start;
  auto auglen = strlen(augstr);
  printf("CIE aug len: %u, aug=%s\n", auglen, augstr);
}

struct Frame {
  uintptr_t cie_addr;
  uint32_t  pc_begin;
  uint32_t  pc_range;
  char      augdata[0];
  
  CIE* get_cie(Entry* entry) {
    return (CIE*)((char*)this - cie_addr + entry->hdr_size());
  }
};

void process_eh_frame(char* loc)
{
  bool is_cie = true;
  
  while (true)
  {
    auto* hdr = (Entry*) loc;
    printf("ENTRY  hdr: %u  size: %u\n", 
        hdr->hdr_size(), hdr->size());
    if (hdr->size() == 0) break;

    if (is_cie) {
      // process CIE information
      auto* cie = hdr->cie();
      cie->process();
      is_cie = false;
    }
    else
    {
      auto* frame = hdr->frame();
      printf("FDE  cie: %u  frame: %#x (%u bytes)\n",
          frame->cie_addr, frame->pc_begin, frame->pc_range);
      
      if (frame->cie_addr) {
        assert (frame->cie_addr && "CIE addr must not be zero");
        frame->get_cie(hdr)->process();
      }
    }
    
    // next entry
    loc += hdr->size() + hdr->hdr_size();
  }
}


#include <kernel/elf.hpp>

void Service::start()
{
  printf("name for this function is %s\n",
      Elf::resolve_symbol((uintptr_t) &Service::start).name.c_str());
  
  try {
    throw std::string("test");
  } catch (std::string err) {
    auto str = "thrown: " + err;
    printf("%s\n", str.c_str());
  }
  
  auto EH_FRAME_START = (uintptr_t) &_EH_FRAME_START_;
  auto EH_FRAME_END = (uintptr_t) &_EH_FRAME_END_;
  printf("eh_frame start: %#x\n", EH_FRAME_START);
  printf("eh_frame end:   %#x\n", EH_FRAME_END);
  printf("eh_frame size:  %u\n", EH_FRAME_END - EH_FRAME_START);
  
  //process_eh_frame(&_EH_FRAME_START_);
  
  print_backtrace();
  
  void begin_work();
  begin_work();
  return;
  
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS

  /*
  auto& tcp = inet->tcp();
  auto& server = tcp.bind(6667); // IRCd default port
  server.onConnect(
  [] (auto csock)
  {
    printf("*** Received connection from %s\n",
    csock->remote().to_string().c_str());

    /// create client ///
    size_t index = clients.size();
    clients.emplace_back(index, csock);

    auto& client = clients[index];

    // set up callbacks
    csock->onReceive(
    [&client] (auto conn, bool)
    {
    char buffer[1024];
    size_t bytes = conn->read(buffer, sizeof(buffer));

    client.read(buffer, bytes);

    });

    .onDisconnect(
    [&client] (auto conn, std::string)
    {
    // remove client from various lists
    client.remove();
    /// inform others about disconnect
    //client.bcast(TK_QUIT, "Disconnected");
    });
  });
  
  inet->dhclient()->set_silent(true);
  
  inet->on_config(
  [] (bool timeout)
  {
    if (timeout)
      printf("Inet::on_config: Timeout\n");
    else
      printf("Inet::on_config: DHCP Server acknowledged our request!\n");
    printf("Service IP address: %s, router: %s\n", 
      inet->ip_addr().str().c_str(), inet->router().str().c_str());
    
    using namespace net;
    const UDP::port_t port = 4242;
    auto& sock = inet->udp().bind(port);
    
    sock.on_read(
    [&sock] (UDP::addr_t addr, UDP::port_t port,
            const char* data, size_t len)
    {
      std::string strdata(data, len);
      CHECK(1, "Getting UDP data from %s:%d -> %s",
            addr.str().c_str(), port, strdata.c_str());
      // send the same thing right back!
      sock.sendto(addr, port, data, len,
      [&sock, addr, port]
      {
        // print this message once
        printf("*** Starting spam (you should see this once)\n");
        
        typedef std::function<void()> rnd_gen_t;
        auto next = std::make_shared<rnd_gen_t> ();
        
        *next =
        [next, &sock, addr, port] ()
        {
          // spam this message at max speed
          std::string text("Spamorino Cappucino\n");
          
          sock.sendto(addr, port, text.data(), text.size(),
          [next] { (*next)(); });
        };
        
        // start spamming
        (*next)();
      });
    }); // sock on_read
    
  }); // on_config
  */
  
  printf("*** TEST SERVICE STARTED *** \n");
}

void begin_work()
{
  static const int TASKS = 32;
  static uint32_t completed = 0;
  static uint32_t job;
  
  job = 0;
  
  // schedule tasks
  for (int i = 0; i < TASKS; i++)
  SMP::add_task(
  [i] {
    __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    printf("completed task %d\n", completed);
    completed++;
    
    if (completed % TASKS == 0) {
      printf("All jobs are done now, compl = %d\t", completed);
      printf("bits = %#x\n", job);
      print_backtrace();
    }
  });
  // start working on tasks
  SMP::start();
}
