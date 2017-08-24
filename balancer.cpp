#include "balancer.hpp"

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace std::chrono;

Balancer::Balancer(
       netstack_t& incoming, uint16_t in_port,
       netstack_t& outgoing)
  : nodes(), netin(incoming)
{
  auto nodelist = parse_node_confg();
  for (auto& addr : nodelist) {
    nodes.add_node(outgoing, addr, pool_signal_t{this, &Balancer::handle_queue});
  }

  netin.tcp().listen(in_port,
  [this] (auto conn) {
    if (conn != nullptr) this->incoming(conn);
  });
}

int Balancer::wait_queue() const {
  return queue.size();
}
void Balancer::incoming(tcp_ptr conn)
{
    queue.emplace_back(conn);
    LBOUT("Queueing connection (q=%lu)\n", queue.size());
    // see if LB needs more connections out
    this->handle_connections();
}
void Balancer::handle_queue()
{
  // check waitq
  while (nodes.pool_size() > 0 && queue.empty() == false)
  {
    auto& client = queue.front();
    if (client.conn->is_connected()) {
      // NOTE: explicitly want to copy buffers
      if (nodes.assign(client.conn, client.readq)) {
        queue.pop_front();
      }
    }
  } // waitq check
  // check if we need to create more connections
  this->handle_connections();
}
void Balancer::handle_connections()
{
  // stop any rethrow timer since this is a de-facto retry
  if (this->rethrow_timer != Timers::UNUSED_ID) {
      Timers::stop(this->rethrow_timer);
      this->rethrow_timer = Timers::UNUSED_ID;
  }
  // calculating number of connection attempts to create
  int np_connecting = nodes.pool_connecting();
  int estimate = queue.size() - (np_connecting + nodes.pool_size());
  estimate = std::min(estimate, MAX_OUTGOING_ATTEMPTS);
  estimate = std::max(0, estimate - np_connecting);
  // create more outgoing connections
  if (estimate > 0)
  {
    try {
      nodes.create_connections(estimate);
    }
    catch (std::exception& e)
    {
      // assuming the failure is due to not enough eph. ports
      this->rethrow_timer = Timers::oneshot(CONNECT_THROW_PERIOD,
      [this] (int) {
          this->rethrow_timer = Timers::UNUSED_ID;
          this->handle_connections();
      });
    }
  } // estimate
} // handle_connections()

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
      readq.emplace_back(buf, len);
    }
  });
}

void Nodes::create_connections(int total)
{
  // temporary iterator
  for (int i = 0; i < total; i++)
  {
    // look for next active node up to *size* times
    for (size_t i = 0; i < nodes.size(); i++)
    {
      conn_iterator = (conn_iterator + 1) % nodes.size();
      if (nodes[conn_iterator].is_active()) break;
    }
    // only connect if node is determined active, to prevent
    // building up connect attempts on just one node
    if (nodes[conn_iterator].is_active()) {
      nodes[conn_iterator].connect();
    }
  }
}
bool Nodes::assign(tcp_ptr conn, queue_vector_t& readq)
{
  for (size_t i = 0; i < nodes.size(); i++)
  {
    auto outgoing = nodes[algo_iterator].get_connection();
    // algorithm here //
    algo_iterator = (algo_iterator + 1) % nodes.size();
    // check if connection was retrieved
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
size_t Nodes::size() const noexcept {
  return nodes.size();
}
Nodes::const_iterator Nodes::begin() const {
  return nodes.cbegin();
}
Nodes::const_iterator Nodes::end() const {
  return nodes.cend();
}
int Nodes::pool_connecting() const {
  int count = 0;
  for (auto& node : nodes) count += node.connection_attempts();
  return count;
}
int Nodes::pool_size() const {
  int count = 0;
  for (auto& node : nodes) count += node.pool_size();
  return count;
}
int32_t Nodes::open_sessions() const {
  return session_cnt;
}
int64_t Nodes::total_sessions() const {
  return session_total;
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
void Nodes::close_session(int idx)
{
  auto& session = sessions.at(idx);
  // disable timeout timer
  Timers::stop(session.timeout_timer);
  session.timeout_timer = Timers::UNUSED_ID;
  // close connections
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
  // free session
  free_sessions.push_back(session.self);
  session_cnt--;
  LBOUT("Session %d closed  (total = %d)\n", session.self, session_cnt);
}

Node::Node(netstack_t& stk, net::Socket a, pool_signal_t sig)
  : stack(stk), addr(a), pool_signal(sig)
{
  // periodically connect to node and determine if active
  // however, perform first check immediately
  this->active_timer = Timers::periodic(0s, ACTIVE_CHECK_PERIOD,
  [this] (int) {
    this->perform_active_check();
  });
}
void Node::perform_active_check()
{
  try {
    this->stack.tcp().connect(this->addr,
    [this] (auto conn) {
      this->active = (conn != nullptr);
      // if we are connected, its alive
      if (conn != nullptr)
      {
        // hopefully put this to good use
        pool.push_back(conn);
        // stop any active check
        this->stop_active_check();
        // signal change in pool
        this->pool_signal();
      }
      else {
        // if no periodic check is being done right now,
        // start doing it (after initial delay)
        this->restart_active_check();
      }
    });
  } catch (std::exception& e) {
    // do nothing, because might just be eph.ports used up
  }
}
void Node::restart_active_check()
{
  // set as inactive
  this->active = false;
  // begin checking active again
  if (this->active_timer == Timers::UNUSED_ID)
  {
    this->active_timer = Timers::periodic(
      ACTIVE_INITIAL_PERIOD, ACTIVE_CHECK_PERIOD,
    [this] (int) {
      this->perform_active_check();
    });
  }
}
void Node::stop_active_check()
{
  // set as active
  this->active = true;
  // stop active checking for now
  if (this->active_timer != Timers::UNUSED_ID) {
    Timers::stop(this->active_timer);
    this->active_timer = Timers::UNUSED_ID;
  }
}
void Node::connect()
{
  auto outgoing = this->stack.tcp().connect(this->addr);
  // connecting to node atm.
  this->connecting++;
  // retry timer when connect takes too long
  int fail_timer = Timers::oneshot(CONNECT_TIMEOUT,
  [this, outgoing] (int)
  {
    // close connection
    outgoing->abort();
    // no longer connecting
    assert(this->connecting > 0);
    this->connecting --;
    // restart active check
    this->restart_active_check();
    // signal change in pool
    this->pool_signal();
  });
  // add connection to pool on success, otherwise.. retry
  outgoing->on_connect(
  [this, fail_timer] (auto conn)
  {
    // stop retry timer
    Timers::stop(fail_timer);
    // no longer connecting
    assert(this->connecting > 0);
    this->connecting --;
    // connection may be null, apparently
    if (conn != nullptr && conn->is_connected())
    {
      LBOUT("Connected to %s  (%ld total)\n",
              addr.to_string().c_str(), pool.size());
      this->pool.push_back(conn);
      // stop any active check
      this->stop_active_check();
      // signal change in pool
      this->pool_signal();
    }
    else {
      this->restart_active_check();
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
      else conn->close();
  }
  return nullptr;
}

// use indexing to access Session because std::vector
Session::Session(Nodes& n, int idx, tcp_ptr inc, tcp_ptr out)
    : parent(n), self(idx), incoming(inc), outgoing(out)
{
  this->timeout_timer = Timers::oneshot(INITIAL_SESSION_TIMEOUT,
  [&nodes = n, idx] (int) {
    nodes.close_session(idx);
  });
  incoming->on_read(READQ_PER_CLIENT,
  [this] (auto buf, size_t len) {
      this->handle_timeout();
      this->outgoing->write(std::move(buf), len);
  });
  incoming->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
  outgoing->on_read(READQ_FOR_NODES,
  [this] (auto buf, size_t len) {
      this->handle_timeout();
      this->incoming->write(std::move(buf), len);
  });
  outgoing->on_close(
  [&nodes = n, idx] () mutable {
      nodes.close_session(idx);
  });
}
void Session::handle_timeout()
{
  // stop old timer
  Timers::stop(this->timeout_timer);
  // create new timeout
  this->timeout_timer = Timers::oneshot(ROLLING_SESSION_TIMEOUT,
  [&nodes = parent, idx = self] (int) {
    nodes.close_session(idx);
  });
}
