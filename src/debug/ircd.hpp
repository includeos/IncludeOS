#pragma once
#include <net/inet4>
#include <vector>

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
extern std::vector<Client> clients;
