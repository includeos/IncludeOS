#include "balancer.hpp"

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

Balancer::Balancer(
       netstack_t& incoming, uint16_t in_port,
       netstack_t& outgoing, std::vector<net::Socket> nodelist, int max_pool)
  : netin(incoming), nodes(outgoing, max_pool)
{
  for (auto& addr : nodelist) {
    nodes.nodes.emplace_back(addr);
    nodes.nodes.back().pool_signal = {this, &Balancer::queue_check};
  }
  nodes.maintain_pool();

  netin.tcp().listen(in_port,
  [this] (tcp_ptr conn) {
    if (conn == nullptr) return;
    this->incoming(conn);
  });
}

int Balancer::wait_queue() const {
  return queue.size();
}
void Balancer::incoming(tcp_ptr conn)
{
  if (nodes.assign(conn) == false) {
      queue.push_back(conn);
      LBOUT("Queueing connection (q=%lu)\n", queue.size());
      //nodes.pool_dynsize++;
      nodes.maintain_pool();
  }
}
void Balancer::queue_check()
{
  if (queue.empty() == false)
  {
    nodes.assign(queue.front());
    queue.pop_front();
  }
}

Nodes::Nodes(netstack_t& out, int sz)
    : netout(out), pool_dynsize(sz)
{}
bool Nodes::assign(tcp_ptr conn)
{
  for (size_t i = 0; i < nodes.size(); i++) {
    // algorithm here //
    iterator = (iterator + 1) % nodes.size();
    auto outgoing = nodes[iterator].get_connection();
    if (outgoing != nullptr)
    {
      assert(outgoing->is_connected());
      LBOUT("Assigning client to node %d (%s)\n",
            iterator, outgoing->to_string().c_str());
      this->create_session(conn, outgoing);
      return true;
    }
  }
  return false;
}
int Nodes::pool_size() const {
  return pool_dynsize;
}
int Nodes::pool_connections() const {
  int count = 0;
  for (auto& node : nodes) count += node.pool.size();
  return count;
}
void Nodes::maintain_pool()
{
  for (auto& node : nodes) {
    node.connect(netout, this->pool_size());
  }
}
int64_t Nodes::total_sessions() const {
  return session_total;
}
int Nodes::open_sessions() const {
  return session_cnt;
}
void Nodes::create_session(tcp_ptr client, tcp_ptr outgoing)
{
  int idx = -1;
  if (free_sessions.empty()) {
    idx = sessions.size();
    sessions.emplace_back(*this, idx, client, outgoing);
  } else {
    idx = free_sessions.back();
    new (&sessions[idx]) Session(*this, idx, client, outgoing);
    free_sessions.pop_back();
  }
  session_total++;
  session_cnt++;
  LBOUT("New session %d  (current = %d, total = %ld)\n",
        idx, session_cnt, session_total);
}
Session& Nodes::get_session(int idx)
{
  return sessions.at(idx);
}
void Nodes::close_session(int idx)
{
  auto& session = get_session(idx);
  if (session.incoming != nullptr) {
    auto conn = std::move(session.incoming);
    conn->reset_callbacks();
    if (conn->is_connected()) conn->close();
  }
  if (session.outgoing != nullptr) {
    auto conn = std::move(session.outgoing);
    conn->reset_callbacks();
    if (conn->is_connected()) conn->close();
  }
  free_sessions.push_back(session.self);
  session_cnt--;
  LBOUT("Session %d closed  (total = %d)\n", session.self, session_cnt);
}

void Node::connect(netstack_t& stack, const int MAX_POOL)
{
  int estimate = MAX_POOL - pool.size();
  LBOUT("Extending pool on %s with %d connections\n",
          this->addr.to_string().c_str(), estimate);
  for (int i = 0; i < estimate; i++)
  {
    try {
      stack.tcp().connect(this->addr,
      [this] (auto conn) {
        pool.push_back(conn);
        if (pool_signal) pool_signal();
      });
    } catch (std::exception&) {
      // probably ran out of eph ports
      return;
    }
  }
}
tcp_ptr Node::get_connection()
{
  while (pool.empty() == false) {
      auto conn = pool.back();
      pool.pop_back();
      if (conn->is_connected()) return conn;
  }
  return nullptr;
}

// use indexing to access Session because std::vector
Session::Session(Nodes& n, int idx, tcp_ptr inc, tcp_ptr out)
    : parent(n), self(idx), incoming(inc), outgoing(out)
{
  incoming->on_read(4096,
  [&nodes = n, idx] (auto buf, size_t len) mutable {
      nodes.get_session(idx).outgoing->write(std::move(buf), len);
  });
  incoming->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
  outgoing->on_read(4096,
  [&nodes = n, idx] (auto buf, size_t len) mutable {
      nodes.get_session(idx).incoming->write(std::move(buf), len);
  });
  outgoing->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
}
