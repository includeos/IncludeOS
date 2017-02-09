#include "ircd.hpp"

// do timeout checks for N clients at a time
static const int MAX_CLIENTS = 200;

void IrcServer::timeout_handler(int)
{
  /// occasionally auto-connect to missing servers
  call_remote_servers();
  
  // nothing to do with no clients
  if (UNLIKELY(clients.empty())) return;
  
  // number of iterations to do
  size_t counter = 
      (MAX_CLIENTS > clients.size()) ? clients.size() : MAX_CLIENTS;
  
  // 16-second slice
  const long now = create_timestamp();
  this->cheapstamp = now;
  
  // create ping request
  char pingreq[32];
  int len = snprintf(pingreq, sizeof(pingreq), "PING :%ld\r\n", now);
  // ping timeout QUIT message
  const std::string reason = "Ping timeout: " + std::to_string(ping_timeout()) + " seconds";
  
  while (counter--)
  {
    Client& client = clients.get(to_current);
    
    if (client.is_alive())
    {
      // registered clients get longer timeouts
      const int ctimer = client.is_reg() ? ping_timeout() : short_ping_timeout();
      
      const long ts = client.get_timeout_ts();
      // if the clients slice is in the ping area
      if (now >= ts + ctimer)
      {
        if (!client.is_warned()) {
          // send ping request
          client.send_raw(pingreq, len);
          client.set_to_stamp(now);
          client.set_warned(true);
        }
        else {
          // kick client for being unresponsive
          client.kill(false, reason);
        }
      }
    }
    
    // next client
    to_current = (to_current + 1) % clients.size();
  }
}
