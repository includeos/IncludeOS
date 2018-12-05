#pragma once
#include <net/inet>
#include <liveupdate>

namespace microLB
{
  typedef net::Inet netstack_t;
  typedef net::tcp::Connection_ptr tcp_ptr;
  typedef std::vector<net::tcp::buffer_t> queue_vector_t;
  typedef delegate<void()> pool_signal_t;

  typedef delegate<void(net::Stream_ptr)> node_connect_result_t;
  typedef std::chrono::milliseconds timeout_t;
  typedef delegate<void(timeout_t, node_connect_result_t)> node_connect_function_t;

  struct Waiting {
    Waiting(net::Stream_ptr);
    Waiting(liu::Restore&, net::TCP&);
    void serialize(liu::Storage&);

    net::Stream_ptr conn;
    queue_vector_t readq;
    int total = 0;
  };

  struct Nodes;
  struct Session {
    Session(Nodes&, int idx, net::Stream_ptr in, net::Stream_ptr out);
    bool is_alive() const;
    void serialize(liu::Storage&);

    Nodes&     parent;
    const int  self;
    net::Stream_ptr incoming;
    net::Stream_ptr outgoing;
  };

  struct Balancer;
  struct Node {
    Node(Balancer&, net::Socket, node_connect_function_t,
         bool do_active = true, int idx = -1);

    auto address() const noexcept { return m_socket; }
    int  connection_attempts() const noexcept { return this->connecting; }
    int  pool_size() const noexcept { return pool.size(); }
    bool is_active() const noexcept { return active; };
    bool active_check() const noexcept { return do_active_check; }

    void restart_active_check();
    void perform_active_check(int);
    void stop_active_check();
    void connect();
    net::Stream_ptr get_connection();

  private:
    node_connect_function_t m_connect = nullptr;
    pool_signal_t           m_pool_signal = nullptr;
    std::vector<net::Stream_ptr> pool;
    net::Socket m_socket;
    [[maybe_unused]] int m_idx;
    bool       active = false;
    const bool do_active_check;
    signed int active_timer = -1;
    signed int connecting = 0;
  };

  struct Nodes {
    typedef std::deque<Node> nodevec_t;
    typedef nodevec_t::iterator iterator;
    typedef nodevec_t::const_iterator const_iterator;

    Nodes(bool ac) : do_active_check(ac) {}

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
    Session& create_session(net::Stream_ptr inc, net::Stream_ptr out);
    void     close_session(int);
    Session& get_session(int);
    void     close_all_sessions();

    void serialize(liu::Storage&);
    void deserialize(netstack_t& in, netstack_t& out, liu::Restore&);
    // make the microLB more testable
    delegate<void(int idx, int current, int total)> on_session_close = nullptr;

  private:
    nodevec_t nodes;
    int64_t   session_total = 0;
    int       session_cnt = 0;
    int       conn_iterator = 0;
    int       algo_iterator = 0;
    const bool do_active_check;
    std::deque<Session> sessions;
    std::deque<int> free_sessions;
  };

  struct Balancer {
    Balancer(bool active_check);
    ~Balancer();
    static Balancer* from_config();

    // Frontend/Client-side of the load balancer
    void open_for_tcp(netstack_t& interface, uint16_t port);
    void open_for_s2n(netstack_t& interface, uint16_t port, const std::string& cert, const std::string& key);
    void open_for_ossl(netstack_t& interface, uint16_t port, const std::string& cert, const std::string& key);
    // Backend/Application side of the load balancer
    static node_connect_function_t connect_with_tcp(netstack_t& interface, net::Socket);

    int  wait_queue() const;
    int  connect_throws() const;

    // add a client stream to the load balancer
    // NOTE: the stream must be connected prior to calling this function
    void incoming(net::Stream_ptr);

    void serialize(liu::Storage&, const liu::buffer_t*);
    void resume_callback(liu::Restore&);

    Nodes nodes;
    pool_signal_t get_pool_signal();

  private:
    void handle_connections();
    void handle_queue();
    void init_liveupdate();
    void deserialize(liu::Restore&);
    std::vector<net::Socket> parse_node_confg();

    std::deque<Waiting> queue;
    int throw_retry_timer = -1;
    int throw_counter = 0;
    // TLS stuff (when enabled)
    void* tls_context = nullptr;
    delegate<void()> tls_free = nullptr;
  };

  template <typename... Args>
  inline void Nodes::add_node(Args&&... args) {
    nodes.emplace_back(std::forward<Args> (args)...,
                       this->do_active_check, nodes.size());
  }
}
