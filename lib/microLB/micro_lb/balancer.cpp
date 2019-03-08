#include "balancer.hpp"
#include <net/tcp/stream.hpp>
#include <os.hpp>

#define MAX_OUTGOING_ATTEMPTS    100
// checking if nodes are dead or not
#define ACTIVE_INITIAL_PERIOD     8s
#define ACTIVE_CHECK_PERIOD      30s
// connection attempt timeouts
#define CONNECT_TIMEOUT          10s
#define CONNECT_THROW_PERIOD     20s

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf("MICROLB: "); printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace std::chrono;

// NOTE: Do NOT move microLB::Balancer while in operation!
// It uses tons of delegates that capture "this"
namespace microLB
{
  Balancer::Balancer(const bool da) : nodes {*this, da}  {}
  Balancer::~Balancer()
  {
    queue.clear();
    nodes.close_all_sessions();
    if (tls_free) tls_free();
  }
  int Balancer::wait_queue() const {
    return this->queue.size();
  }
  int Balancer::connect_throws() const {
    return this->throw_counter;
  }
  pool_signal_t Balancer::get_pool_signal()
  {
    return {this, &Balancer::handle_queue};
  }
  void Balancer::incoming(net::Stream_ptr conn)
  {
      assert(conn != nullptr);
      queue.emplace_back(std::move(conn));
      LBOUT("Queueing connection (q=%lu)\n", queue.size());
      // IMPORTANT: try to handle queue, in case its ready
      // don't directly call handle_connections() from here!
      this->handle_queue();
  }
  void Balancer::handle_queue()
  {
    // check waitq
    while (nodes.pool_size() > 0 && queue.empty() == false)
    {
      auto& client = queue.front();
      assert(client.conn != nullptr);
      if (client.conn->is_connected()) {
        try {
          // NOTE: explicitly want to copy buffers
          net::Stream_ptr rval =
              nodes.assign(std::move(client.conn));
          if (rval == nullptr) {
            // done with this queue item
            queue.pop_front();
          }
          else {
            // put connection back in queue item
            client.conn = std::move(rval);
          }
        } catch (...) {
          queue.pop_front(); // we have no choice
          throw;
        }
      }
      else {
        queue.pop_front();
      }
    } // waitq check
    // check if we need to create more connections
    this->handle_connections();
  }
  void Balancer::handle_connections()
  {
    LBOUT("Handle_connections. %lu waiting \n", queue.size());
    // stop any rethrow timer since this is a de-facto retry
    if (this->throw_retry_timer != Timers::UNUSED_ID) {
        Timers::stop(this->throw_retry_timer);
        this->throw_retry_timer = Timers::UNUSED_ID;
    }

    // prune dead clients because the "number of clients" is being
    // used in a calculation right after this to determine how many
    // nodes to connect to
    auto new_end = std::remove_if(queue.begin(), queue.end(),
        [](Waiting& client) {
          return client.conn == nullptr || client.conn->is_connected() == false;
        });
    queue.erase(new_end, queue.end());

    // calculating number of connection attempts to create
    int np_connecting = nodes.pool_connecting();
    int estimate = queue.size() - (np_connecting + nodes.pool_size());
    estimate = std::min(estimate, MAX_OUTGOING_ATTEMPTS);
    estimate = std::max(0, estimate - np_connecting);
    // create more outgoing connections
    LBOUT("Estimated connections needed: %d\n", estimate);
    if (estimate > 0)
    {
      try {
        nodes.create_connections(estimate);
      }
      catch (std::exception& e)
      {
        this->throw_counter++;
        // assuming the failure is due to not enough eph. ports
        this->throw_retry_timer = Timers::oneshot(CONNECT_THROW_PERIOD,
        [this] (int) {
            this->throw_retry_timer = Timers::UNUSED_ID;
            this->handle_connections();
        });
      }
    } // estimate
  } // handle_connections()

#if !defined(LIVEUPDATE)
  //if we dont support liveupdate then do nothing
  void init_liveupdate() {}
#endif

  Waiting::Waiting(net::Stream_ptr incoming)
    : conn(std::move(incoming)), total(0)
  {
    assert(this->conn != nullptr);
    assert(this->conn->is_connected());

    // Release connection if it closes before it's assigned to a node.
    this->conn->on_close([this](){
        LBOUT("Waiting issuing close\n");
        if (this->conn != nullptr)
          this->conn->reset_callbacks();
        this->conn = nullptr;
      });
  }

  void Nodes::create_connections(int total)
  {
    // temporary iterator
    for (int i = 0; i < total; i++)
    {
      bool dest_found = false;
      // look for next active node up to *size* times
      for (size_t i = 0; i < nodes.size(); i++)
      {
        const int iter = conn_iterator;
        conn_iterator = (conn_iterator + 1) % nodes.size();
        // if the node is active, connect immediately
        auto& dest_node = nodes[iter];
        if (dest_node.is_active()) {
          dest_node.connect();
          dest_found = true;
          break;
        }
      }
      // if no active node found, simply delegate to the next node
      if (dest_found == false)
      {
        // with active-checks we can return here later when we get a connection
        if (this->do_active_check) return;
        const int iter = conn_iterator;
        conn_iterator = (conn_iterator + 1) % nodes.size();
        nodes[iter].connect();
      }
    }
  }
  net::Stream_ptr Nodes::assign(net::Stream_ptr conn)
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
              algo_iterator, outgoing->to_string().c_str());
        //Should we some way hold track of the session object ?
        auto& session = this->create_session(
              std::move(conn), std::move(outgoing));

        return nullptr;
      }
    }
    return conn;
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
  int32_t Nodes::timed_out_sessions() const {
    return 0;
  }
  Session& Nodes::create_session(net::Stream_ptr client, net::Stream_ptr outgoing)
  {
    int idx = -1;
    if (free_sessions.empty()) {
      idx = sessions.size();
      sessions.emplace_back(*this, idx, std::move(client), std::move(outgoing));
    } else {
      idx = free_sessions.back();
      new (&sessions[idx]) Session(*this, idx, std::move(client), std::move(outgoing));
      free_sessions.pop_back();
    }
    session_total++;
    session_cnt++;
    LBOUT("New session %d  (current = %d, total = %ld)\n",
          idx, session_cnt, session_total);
    return sessions[idx];
  }
  Session& Nodes::get_session(int idx)
  {
    auto& session = sessions.at(idx);
    assert(session.is_alive());
    return session;
  }

  void Nodes::destroy_sessions()
  {
    for (auto& idx: closed_sessions)
    {
      auto &session=get_session(idx);

      // free session destroying potential unique ptr objects
      session.incoming = nullptr;
      auto out_tcp = dynamic_cast<net::tcp::Stream*>(session.outgoing->bottom_transport())->tcp();
      session.outgoing = nullptr;
      // if we don't have anything to write to the backend, abort it.
      if(not out_tcp->sendq_size())
        out_tcp->abort();
      free_sessions.push_back(session.self);
      LBOUT("Session %d destroyed  (total = %d)\n", session.self, session_cnt);
    }
    closed_sessions.clear();
  }
  void Nodes::close_session(int idx)
  {
    auto& session = get_session(idx);
    // remove connections
    session.incoming->reset_callbacks();
    session.outgoing->reset_callbacks();
    closed_sessions.push_back(session.self);

    destroy_sessions();

    session_cnt--;
    LBOUT("Session %d closed  (total = %d)\n", session.self, session_cnt);
    if (on_session_close) on_session_close(session.self, session_cnt, session_total);
  }
  void Nodes::close_all_sessions()
  {
    sessions.clear();
    free_sessions.clear();
  }

  Node::Node(Balancer& balancer, const net::Socket addr,
             node_connect_function_t func, bool da, int idx)
    : m_connect(func), m_socket(addr), m_idx(idx), do_active_check(da)
  {
    assert(this->m_connect != nullptr);
    this->m_pool_signal = balancer.get_pool_signal();
    // periodically connect to node and determine if active
    if (this->do_active_check)
    {
      // however, perform first check immediately
      this->active_timer = Timers::periodic(0s, ACTIVE_CHECK_PERIOD,
                           {this, &Node::perform_active_check});
    }
  }
  void Node::perform_active_check(int)
  {
    assert(this->do_active_check);
    try {
      this->connect();
    } catch (std::exception& e) {
      // do nothing, because might be eph.ports used up
      LBOUT("Node %d exception %s\n", this->m_idx, e.what());
    }
  }
  void Node::restart_active_check()
  {
    // set as inactive
    this->active = false;
    if (this->do_active_check)
    {
      // begin checking active again
      if (this->active_timer == Timers::UNUSED_ID)
      {
        this->active_timer = Timers::periodic(
          ACTIVE_INITIAL_PERIOD, ACTIVE_CHECK_PERIOD,
          {this, &Node::perform_active_check});

        LBOUT("Node %s restarting active check (and is inactive)\n",
              this->addr.to_string().c_str());
      }
      else
      {
        LBOUT("Node %s still trying to connect...\n",
              this->addr.to_string().c_str());
      }
    }
  }
  void Node::stop_active_check()
  {
    // set as active
    this->active = true;
    if (this->do_active_check)
    {
      // stop active checking for now
      if (this->active_timer != Timers::UNUSED_ID) {
        Timers::stop(this->active_timer);
        this->active_timer = Timers::UNUSED_ID;
      }
    }
    LBOUT("Node %d stopping active check (and is active)\n", this->m_idx);
  }
  void Node::connect()
  {
    // connecting to node atm.
    this->connecting++;
    this->m_connect(CONNECT_TIMEOUT,
      [this] (net::Stream_ptr stream)
      {
        // no longer connecting
        assert(this->connecting > 0);
        this->connecting --;
        // success
        if (stream != nullptr)
        {
          assert(stream->is_connected());
          LBOUT("Node %d connected to %s (%ld total)\n",
                this->m_idx, stream->remote().to_string().c_str(), pool.size());
          this->pool.push_back(std::move(stream));
          // stop any active check
          this->stop_active_check();
          // signal change in pool
          this->m_pool_signal();
        }
        else // failure
        {
          LBOUT("Node %d failed to connect out (%ld total)\n",
                this->m_idx, pool.size());
          // restart active check
          this->restart_active_check();
        }
      });
  }
  net::Stream_ptr Node::get_connection()
  {
    while (pool.empty() == false) {
        auto conn = std::move(pool.back());
        assert(conn != nullptr);
        pool.pop_back();
        if (conn->is_connected()) {
          return conn;
        }
        else
        {
          printf("CLOSING SINCE conn->connected is false\n");
          conn->close();
        }
    }
    return nullptr;
  }

  // use indexing to access Session because std::vector
  Session::Session(Nodes& n, int idx,
                   net::Stream_ptr inc, net::Stream_ptr out)
      : parent(n), self(idx), incoming(std::move(inc)),
                              outgoing(std::move(out))
  {
    incoming->on_data({this, &Session::flush_incoming});
    incoming->on_close(
    [&nodes = n, idx] () {
        nodes.close_session(idx);
    });

    outgoing->on_data({this, &Session::flush_outgoing});
    outgoing->on_close(
    [&nodes = n, idx] () {
        nodes.close_session(idx);
    });
  }
  bool Session::is_alive() const {
    return incoming != nullptr;
  }

  void Session::flush_incoming()
  {
    assert(this->is_alive());
    while((this->incoming->next_size() > 0) and this->outgoing->is_writable())
    {
      this->outgoing->write(this->incoming->read_next());
    }
  }

  void Session::flush_outgoing()
  {
    assert(this->is_alive());
    while((this->outgoing->next_size() > 0) and this->incoming->is_writable())
    {
      this->incoming->write(this->outgoing->read_next());
    }
  }
}
