#include "balancer.hpp"

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

Balancer::Balancer(
       netstack_t& incoming, uint16_t in_port,
       netstack_t& outgoing, std::vector<net::Socket> nodelist,
       const int POOL_BASE)
  : netin(incoming), nodes(outgoing, POOL_BASE)
{
  for (auto& addr : nodelist) {
    nodes.nodes.emplace_back(addr);
    nodes.nodes.back().pool_signal = {this, &Balancer::queue_check};
  }
  // initial pool
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
  if (nodes.assign(conn, {}) == false) {
      queue.emplace_back(conn);
      LBOUT("Queueing connection (q=%lu)\n", queue.size());
  }
  // make sure connection pool is healthy
  nodes.maintain_pool();
}
void Balancer::queue_check(bool success)
{
  LBOUT("Connection attempt %s\n", ((success) ? "success" : "failed"));

  // one less TCP connection connecting out
  nodes.connect_ended();
  // if the connect succeeded
  if (success)
  {
    // check waitq
    while (nodes.pool_connections() > 0 && queue.empty() == false)
    {
      auto item = std::move(queue.front());
      queue.pop_front();
      if (item.conn->is_connected()) {
        if (nodes.assign(item.conn, std::move(item.buffers)) == false) {
          queue.push_back(std::move(item));
        }
      }
    } // waitq check
  } // succcess
}

Waiting::Waiting(tcp_ptr incoming)
  : conn(incoming), total(0)
{
  // queue incoming data from clients not yet
  // assigned to a node
  conn->on_read(READQ_PER_CLIENT,
  [this] (auto buf, size_t len) {
    // prevent buffer bloat attack
    total += len;
    if (total > MAX_READQ_PER_NODE) {
      conn->abort();
    }
    else {
      LBOUT("*** Queued %lu bytes\n", len);
      buffers.emplace_back(buf, len);
    }
  });
}

Nodes::Nodes(netstack_t& out, const int POOL_SZ)
    : netout(out), POOL_SIZE(POOL_SZ)
{}
bool Nodes::assign(tcp_ptr conn, queue_vector_t readq)
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
      // flush readq
      for (auto& buffer : readq) {
        LBOUT("*** Flushing %lu bytes\n", buffer.second);
        outgoing->write(std::move(buffer.first), buffer.second);
      }
      return true;
    }
  }
  return false;
}
int Nodes::pool_size() const {
  return POOL_SIZE;
}
int Nodes::pool_connections() const {
  int count = 0;
  for (auto& node : nodes) count += node.pool.size();
  return count;
}
void Nodes::connect_ended()
{
  assert(this->connecting > 0);
  this->connecting --;
  // re-grow connection pool
  this->maintain_pool();
}
void Nodes::maintain_pool()
{
  // each time a connect is resolved, maintain pool if necessary
  int estimate = pool_size() - (this->connecting + pool_connections());
  LBOUT("Estimate need %d new connections\n", estimate);
  if (estimate <= 0) return;

  LBOUT("*** Extending pools with %d conns (current=%d)\n",
        estimate, this->pool_connections());
  try {
    for (int i = 0; i < estimate; i++)
    {
      connecting++;
      nodes.at(pool_iterator).connect(netout);
      pool_iterator = (pool_iterator + 1) % nodes.size();
    }
  } catch (std::exception&) {
    // probably ran out of eph ports
    return;
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

void Node::connect(netstack_t& stack)
{
  stack.tcp().connect(this->addr,
  [this] (auto conn) {
    // connection may be null, apparently
    if (conn != nullptr)
    {
      LBOUT("Connected to %s  (%ld total)\n",
              addr.to_string().c_str(), pool.size());
      pool.push_back(conn);
      pool_signal(true);
    }
    else {
      pool_signal(false);
    }
  });
}
tcp_ptr Node::get_connection()
{
  while (pool.empty() == false) {
      auto conn = pool.back();
      assert(conn != nullptr);
      pool.pop_back();
      if (conn->is_connected()) return conn;
  }
  return nullptr;
}

// use indexing to access Session because std::vector
Session::Session(Nodes& n, int idx, tcp_ptr inc, tcp_ptr out)
    : parent(n), self(idx), incoming(inc), outgoing(out)
{
  incoming->on_read(READQ_PER_CLIENT,
  [&nodes = n, idx] (auto buf, size_t len) mutable {
    assert(len > 0);
      auto& remote = nodes.get_session(idx).outgoing;
      remote->write(std::move(buf), len);
  });
  incoming->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
  outgoing->on_read(4096,
  [&nodes = n, idx] (auto buf, size_t len) mutable {
    assert(len > 0);
      auto& remote = nodes.get_session(idx).incoming;
      remote->write(std::move(buf), len);
  });
  outgoing->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
}
