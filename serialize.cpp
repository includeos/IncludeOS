
#include "balancer.hpp"

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace liu;

void Nodes::serialize(Storage& store)
{
  store.add<int64_t>(100, this->session_total);
  store.add_int(100, this->session_cnt);
  store.add_int(100, this->session_timeouts);
  store.put_marker(100);

  const int tot_sessions = sessions.size() - free_sessions.size();

  LBOUT("Serialize %llu sessions\n", tot_sessions);
  store.add_int(102, tot_sessions);

  int alive = 0;
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
  store.add_connection(120, incoming);
  store.add_connection(121, outgoing);
  store.put_marker(120);
}

void Nodes::deserialize(netstack_t& in, netstack_t& out, Restore& store)
{
  /// nodes member fields ///
  this->session_total = store.as_type<int64_t>(); store.go_next();
  this->session_cnt   = store.as_int();           store.go_next();
  this->session_timeouts = store.as_int();        store.go_next();
  store.pop_marker(100);

  /// sessions ///
  auto& tcp_in  = in.tcp();
  auto& tcp_out = out.tcp();
  const int tot_sessions = store.as_int(); store.go_next();
  LBOUT("Deserialize %llu sessions\n", tot_sessions);
  for(auto i = 0; i < static_cast<int>(tot_sessions); i++)
  {
    auto incoming = store.as_tcp_connection(tcp_in); store.go_next();
    auto outgoing = store.as_tcp_connection(tcp_out); store.go_next();
    store.pop_marker(120);

    create_session(true /* no readq atm */, incoming, outgoing);
  }
}

void Waiting::serialize(liu::Storage& store)
{
  store.add_connection(10, this->conn);
  store.add_int(11, (int) readq.size());
  for (auto& item : readq) {
    store.add_buffer(12, item.first.get(), item.second);
  }
  store.put_marker(10);
}
Waiting::Waiting(liu::Restore& store, net::TCP& stack)
{
  this->conn = store.as_tcp_connection(stack); store.go_next();
  int qsize = store.as_int(); store.go_next();
  for (int i = 0; i < qsize; i++)
  {
    auto buf = store.as_buffer(); store.go_next();
    auto sbuf = net::tcp::new_shared_buffer(buf.size());
    memcpy(sbuf.get(), buf.data(), buf.size());
    readq.emplace_back(std::move(sbuf), buf.size());
  }
  store.pop_marker(10);
}

void Balancer::serialize(Storage& store, const buffer_t*)
{
  store.add_int(0, this->throw_counter);
  store.put_marker(0);
  /// wait queue
  store.add_int(1, (int) queue.size());
  for (auto& client : queue) {
    client.serialize(store);
  }
  /// nodes
  nodes.serialize(store);
}
void Balancer::deserialize(Restore& store)
{
  this->throw_counter = store.as_int(); store.go_next();
  store.pop_marker(0);
  /// wait queue
  int wsize = store.as_int(); store.go_next();
  for (int i = 0; i < wsize; i++) {
    queue.emplace_back(store, this->netin.tcp());
  }
  /// nodes
  nodes.deserialize(netin, netout, store);
}
