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
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

// our VGA output module
#include <kernel/vga.hpp>
ConsoleVGA vga;

#include "ircsplit.hpp"
static const std::string SERVER_NAME = "irc.includeos.org";

class Client
{
public:
  using Connection = std::shared_ptr<net::TCP::Connection>;
  
  Client(size_t s, Connection c)
    : alive(true), regis(0), self(s), conn(c) {}
  
  bool is_alive() const
  {
    return alive;
  }
  bool is_reg() const
  {
    return regis == 3;
  }
  void remove()
  {
    alive = false; regis = 0;
  }
  
  void read(const char* buffer, size_t len);
  void split_message(const std::string&);
  void handle(const std::string&, const std::vector<std::string>&);
  
  void send(uint16_t numeric, std::string text);
  void send(std::string text);
  
  std::string userhost() const
  {
    return user + "@" + host;
  }
  std::string nickuserhost() const
  {
    return nick + "!" + userhost();
  }
  
private:
  void welcome(uint8_t);
  void auth_notice();
  
  bool        alive;
  uint8_t     regis;
  size_t      self;
  Connection  conn;
  std::string passw;
  std::string nick;
  std::string user;
  std::string host;
  
  std::string buffer;
  
};
std::vector<Client> clients;

void Client::split_message(const std::string& msg)
{
  std::string source;
  auto vec = split(msg, source);
  
  printf("[Client]: ");
  for (auto& str : vec)
  {
    printf("[%s]", str.c_str());
  }
  printf("\n");
  // ignore empty messages
  if (vec.size() == 0) return;
  // handle message
  handle(source, vec);
}

void Client::read(const char* buf, size_t len)
{
  while (len > 0)
  {
    int search = -1;
    
    for (size_t i = 0; i < len; i++)
    if (buf[i] == 13 || buf[i] == 10)
    {
      search = i; break;
    }
    // not found:
    if (search == -1)
    {
      // append entire buffer
      buffer.append(buf, len);
      break;
    }
    else
    {
      // found CR LF:
      if (search != 0)
      {
        // append to clients buffer
        buffer.append(buf, search);
        
        // move forward in socket buffer
        buf += search;
        // decrease len
        len -= search;
      }
      else
      {
        buf++; len--;
      }
      
      // parse message
      if (buffer.size())
      {
        split_message(buffer);
        buffer.clear();
      }
    }
  }
}

void Client::send(uint16_t numeric, std::string text)
{
  std::string num;
  num.reserve(128);
  num = std::to_string(numeric);
  num = std::string(3 - num.size(), '0') + num;
  
  num = ":" + SERVER_NAME + " " + num + " " + this->nick + " " + text + "\r\n";
  //printf("-> %s", num.c_str());
  conn->write(num.c_str(), num.size());
}
void Client::send(std::string text)
{
  std::string data = ":" + SERVER_NAME + " " + text + "\r\n";
  //printf("-> %s", data.c_str());
  conn->write(data.c_str(), data.size());
}

#define ERR_NOSUCHNICK     401
#define ERR_NOSUCHCMD      421
#define ERR_NEEDMOREPARAMS 461


void Client::handle(const std::string&,
                    const std::vector<std::string>& msg)
{
  #define TK_CAP    "CAP"
  #define TK_PASS   "PASS"
  #define TK_NICK   "NICK"
  #define TK_USER   "USER"
  
  const std::string& cmd = msg[0];
  
  if (this->is_reg() == false)
  {
    if (cmd == TK_CAP)
    {
      // ignored completely
    }
    else if (cmd == TK_PASS)
    {
      if (msg.size() > 1)
      {
        this->passw = msg[1];
      }
      else
      {
        send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
      }
    }
    else if (cmd == TK_NICK)
    {
      if (msg.size() > 1)
      {
        this->nick = msg[1];
        welcome(regis | 1);
      }
      else
      {
        send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
      }
    }
    else if (cmd == TK_USER)
    {
      if (msg.size() > 1)
      {
        this->user = msg[1];
        welcome(regis | 2);
      }
      else
      {
        send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
      }
    }
    else
    {
      send(ERR_NOSUCHCMD, cmd + " :Unknown command");
    }
  }
}

#define RPL_WELCOME   1
#define RPL_YOURHOST  2
#define RPL_CREATED   3
#define RPL_MYINFO    4
#define RPL_BOUNCE    5

void Client::welcome(uint8_t newreg)
{
  uint8_t oldreg = regis;
  bool regged = is_reg();
  regis = newreg;
  // not registered before, but registered now
  if (!regged && is_reg())
  {
    printf("* Registered: %s\n", nickuserhost().c_str());
    send(RPL_WELCOME, ":Welcome to the Internet Relay Network, " + nickuserhost());
    send(RPL_YOURHOST, ":Your host is " + SERVER_NAME + ", running v1.0");
  }
  else if (oldreg == 0)
  {
    auth_notice();
  }
}
void Client::auth_notice()
{
  send("NOTICE AUTH :*** Processing your connection..");
  send("NOTICE AUTH :*** Looking up your hostname...");
  //hostname_lookup()
  send("NOTICE AUTH :*** Checking Ident");
  //ident_check()
}

void Service::start()
{
  /// virtio-console testing ///
  /*
  auto con = hw::Dev::console<0, VirtioCon> ();
  
  // set secondary serial output to VGA console module
  printf("Attaching console...\n");
  OS::set_rsprint(
  [&con] (const char* data, size_t len)
  {
    con.write(data, len);
    //for (size_t i = 0; i < len; i++)
    //    OS::rswrite(data[i]);
  });*/
  
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config(
      {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  auto& tcp = inet->tcp();
  
  // IRCd default port
  auto& sock = tcp.bind(6667);
  sock.onConnect(
  [&sock] (auto csock)
  {
    printf("Received connection from %s\n",
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
      
    }).onDisconnect(
    [&client] (auto conn, std::string)
    {
      // remove client from various lists
      client.remove();
      /// inform others about disconnect
      //client.bcast(TK_QUIT, "Disconnected");
    });
    
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}
