#pragma once
#include <net/inet>
#include <net/stream.hpp>

#if defined(LIVEUPDATE)
  #include <liveupdate>
#endif

namespace microLB
{
  typedef net::Inet netstack_t;
  typedef net::tcp::Connection_ptr tcp_ptr;
  typedef std::vector<net::tcp::buffer_t> queue_vector_t;
  typedef delegate<void()> pool_signal_t;

  struct Waiting {
    Waiting(net::Stream_ptr);
#if defined(LIVEUPDATE)
    Waiting(liu::Restore&, net::TCP&);
    void serialize(liu::Storage&);
#endif

    net::Stream_ptr conn;
    queue_vector_t readq;
    int total = 0;
  };

  struct Nodes;
  struct Session {
    Session(Nodes&, int idx, bool talk, net::Stream_ptr in, net::Stream_ptr out);
    bool is_alive() const;
    void handle_timeout();
    void timeout(Nodes&);
#if defined(LIVEUPDATE)
    void serialize(liu::Storage&);
#endif
    Nodes&     parent;
    const int  self;
    int        timeout_timer;
    net::Stream_ptr incoming;
    net::Stream_ptr outgoing;
  };

  struct Node {
    Node(netstack_t& stk, net::Socket a, const pool_signal_t&);

    auto address() const noexcept { return this->addr; }
    int  connection_attempts() const noexcept { return this->connecting; }
    int  pool_size() const noexcept { return pool.size(); }
    bool is_active() const noexcept { return active; };

    void    restart_active_check();
    void    perform_active_check();
    void    stop_active_check();
    void    connect();
    net::Stream_ptr get_connection();

    netstack_t& stack;
  private:
    net::Socket addr;
    const pool_signal_t& pool_signal;
    std::vector<net::Stream_ptr> pool;
    bool        active = false;
    int         active_timer = -1;
    signed int  connecting = 0;
  };

  struct Nodes {
    typedef std::deque<Node> nodevec_t;
    typedef nodevec_t::iterator iterator;
    typedef nodevec_t::const_iterator const_iterator;
    Nodes() {}

    size_t   size() const noexcept;
    const_iterator begin() const;
    const_iterator end() const;

    int32_t open_sessions() const;
    int64_t total_sessions() const;
    int32_t timed_out_sessions() const;
    int  pool_connecting() const;
    int  pool_size() const;

    template <typename... Args>
    void add_node(Args&&... args);
    void create_connections(int total);
    // returns the connection back if the operation fails
    net::Stream_ptr assign(net::Stream_ptr, queue_vector_t&);
    Session& create_session(bool talk, net::Stream_ptr inc, net::Stream_ptr out);
    void     close_session(int, bool timeout = false);
    Session& get_session(int);
#if defined(LIVEUPDATE)
    void serialize(liu::Storage&);
    void deserialize(netstack_t& in, netstack_t& out, liu::Restore&);
#endif

  private:
    nodevec_t nodes;
    int64_t   session_total = 0;
    int       session_cnt = 0;
    int       session_timeouts = 0;
    int       conn_iterator = 0;
    int       algo_iterator = 0;
    std::deque<Session> sessions;
    std::deque<int> free_sessions;
  };

  struct Balancer {
    Balancer(netstack_t& in, uint16_t port, netstack_t& out);
    Balancer(netstack_t& in, uint16_t port, netstack_t& out,
             const std::string& cert, const std::string& key);
    static Balancer* from_config();

    int  wait_queue() const;
    int  connect_throws() const;
#if defined(LIVEUPDATE)
    void serialize(liu::Storage&, const liu::buffer_t*);
    void resume_callback(liu::Restore&);
#endif

    Nodes nodes;
    netstack_t& get_client_network() noexcept;
    netstack_t& get_nodes_network()  noexcept;
    const pool_signal_t& get_pool_signal() const;

  private:
    void incoming(net::Stream_ptr);
    void handle_connections();
    void handle_queue();
    void init_liveupdate();
#if defined(LIVEUPDATE)
    void deserialize(liu::Restore&);
#endif
    std::vector<net::Socket> parse_node_confg();

    netstack_t& netin;
    netstack_t& netout;
    pool_signal_t signal = nullptr;
    std::deque<Waiting> queue;
    int throw_retry_timer = -1;
    int throw_counter = 0;
    void* openssl_data = nullptr;
  };

  template <typename... Args>
  inline void Nodes::add_node(Args&&... args) {
    nodes.emplace_back(std::forward<Args> (args)...);
  }
}
