#include "server.hpp"

Server::Server(sindex_t idx, IrcServer& srv, Connection tcpconn)
  : self(idx), server(srv), conn(tcpconn)
{
  
}
