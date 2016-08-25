#include "ircd.hpp"

static const int PING_TIMER = 5;
static const int SHORT_PING_TIMER = 1;
// do timeout checks for N clients at a time
static const int MAX_CLIENTS = 1000;

void IrcServer::timeout_handler(uint32_t)
{
  // nothing to do with no clients
  if (UNLIKELY(clients.empty())) return;
  
  // number of iterations to do
  size_t counter = 
      (MAX_CLIENTS > clients.size()) ? clients.size() : MAX_CLIENTS;
  
  // 16-second slice
  const long now = create_timestamp() >> 4;
  // create ping request
  char pingreq[32];
  int len = snprintf(pingreq, sizeof(pingreq), 
            "PING :%ld\r\n", now);
  
  while (counter--)
  {
    Client& client = clients[to_current];
    
    if (client.is_alive())
    {
      // registered clients get longer timeouts
      const int ctimer = client.is_reg() ? PING_TIMER : SHORT_PING_TIMER;
      
      const long ts = client.get_timeout_ts() >> 4;
      // if the clients slice is in the ping area
      if (now == ts + ctimer)
      {
        // send ping request
        client.send_raw(pingreq, len);
      }
      else if (now >= ts + 2*ctimer)
      {
        // kick client for being unresponsive
        client.kill(false, "Ping timeout");
      }
    }
    
    // next client
    to_current = (to_current + 1) % clients.size();
  }
}
