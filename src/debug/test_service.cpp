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

#include <hw/apic.hpp>

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

static const uintptr_t ELF_START = 0x200000;
extern "C" {
  char _TEXT_START_;
}

#include <iostream>
#include "../../vmbuild/elf.h"

void Service::start()
{
  // parse ELF header
  const uintptr_t HDR_SIZE = (uintptr_t) &_TEXT_START_ - ELF_START;
  
  printf("headers size: %u\n", HDR_SIZE);
  
  // read symtab
  auto elf_header = (Elf32_Ehdr*) ELF_START;
  
  using namespace std;
  cout << "Reading ELF headers...\n";
  cout << "Signature: ";

  for(int i {0}; i < EI_NIDENT; ++i) {
    cout << elf_header->e_ident[i];
  }
  
  cout << "\nType: " << ((elf_header->e_type == ET_EXEC) ? " ELF Executable\n" : "Non-executable\n");
  cout << "Machine: ";

  switch (elf_header->e_machine) {
  case (EM_386):
    cout << "Intel 80386\n";
    break;
  case (EM_X86_64):
    cout << "Intel x86_64\n";
    break;
  default:
    cout << "UNKNOWN (" << elf_header->e_machine << ")\n";
    break;
  } //< switch (elf_header->e_machine)

  cout << "Version: "                   << elf_header->e_version      << '\n';
  cout << "Entry point: 0x"             << hex << elf_header->e_entry << '\n';
  cout << "Number of program headers: " << elf_header->e_phnum        << '\n';
  cout << "Program header offset: "     << elf_header->e_phoff        << '\n';
  cout << "Number of section headers: " << elf_header->e_shnum        << '\n';
  cout << "Section header offset: "     << elf_header->e_shoff        << '\n';
  cout << "Size of ELF-header: "        << elf_header->e_ehsize << " bytes\n";
  
  
  
  return;
  
  void begin_work();
  begin_work();
  
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
  hw::APIC::add_task(
  [i] {
    __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    printf("completed task %d\n", completed);
    completed++;
    
    if (completed % TASKS == 0) {
      printf("All jobs are done now, compl = %d\t", completed);
      printf("bits = %#x\n", job);
      begin_work();
    }
  });
  // start working on tasks
  hw::APIC::work_signal();
}
