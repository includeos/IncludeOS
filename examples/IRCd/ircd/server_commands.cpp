#include "server.hpp"
#include <unordered_map>
#define BUFFER_SIZE   1024

typedef delegate<void(Server&, const std::vector<std::string>&)> command_func_t;
static std::unordered_map<std::string, command_func_t> funcs;

static void handle_ping(Server& server, const std::vector<std::string>& msg)
{
  if (msg.size() > 1) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer),
        "PONG :%s\r\n", msg[1].c_str());
    if (len > 0) server.send(buffer, len);
  }
}
static void handle_pong(Server&, const std::vector<std::string>&)
{
  // do nothing
}

void Server::handle_commands(const std::vector<std::string>& msg)
{
  auto it = funcs.find(msg[0]);
  if (it != funcs.end()) {
    it->second(*this, msg);
  }
  else {
    this->squit("Unknown command: " + msg[0]);
  }
}

void Server::init()
{
  funcs["PING"] = handle_ping;
  funcs["PONG"] = handle_pong;
}
