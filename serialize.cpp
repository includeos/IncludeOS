
#include "balancer.hpp"

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace liu;
void Balancer::serialize(Storage& store, const buffer_t*)
{
  nodes.serialize(store);
}

void Nodes::serialize(Storage& store)
{
  /*
  nodevec_t nodes;
  int64_t   session_total = 0;
  int       session_cnt = 0;
  int       session_timeouts = 0;
  int       conn_iterator = 0;
  int       algo_iterator = 0;
  std::deque<Session> sessions;
  std::deque<int> free_sessions;
  */
  const auto tot_sessions = sessions.size() - free_sessions.size();

  LBOUT("Serialize %llu sessions\n", tot_sessions);
  store.add<size_t>(100, tot_sessions);

  size_t alive = 0;
  for(auto& session : sessions)
  {
    if(session.is_alive())
    {
      session.serialize(store);
      ++alive;
    }
  }
  assert(alive == tot_sessions
    && "Mismatch between number of said serialized sessions and the actual number serialized.");
}


void Session::serialize(Storage& store)
{
  /*
  Nodes&    parent;
  const int self;
  int       timeout_timer;
  tcp_ptr   incoming;
  tcp_ptr   outgoing;
  */
  store.add_connection(101, incoming);
  store.add_connection(102, outgoing);
}

void Balancer::deserialize(Restore& store)
{
  nodes.deserialize(netin, netout, store);
}

void Nodes::deserialize(netstack_t& in, netstack_t& out, Restore& store)
{
  auto& tcp_in  = in.tcp();
  auto& tcp_out = out.tcp();
  const auto tot_sessions = store.as_type<size_t>(); store.go_next();
  LBOUT("Deserialize %llu sessions\n", tot_sessions);
  for(auto i = 0; i < static_cast<int>(tot_sessions); i++)
  {
    auto incoming = store.as_tcp_connection(tcp_in); store.go_next();
    auto outgoing = store.as_tcp_connection(tcp_out); store.go_next();

    create_session(false /* no readq atm */, incoming, outgoing);
  }
}
