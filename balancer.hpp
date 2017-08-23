#pragma once
#include <net/inet4>
#define READQ_PER_CLIENT    4096
#define MAX_READQ_PER_NODE  8192
#define READQ_FOR_NODES     8192

#define INITIAL_SESSION_TIMEOUT   5s
#define ROLLING_SESSION_TIMEOUT  60s

typedef net::Inet<net::IP4> netstack_t;
typedef net::tcp::Connection_ptr tcp_ptr;
typedef std::vector< std::pair<net::tcp::buffer_t, size_t> > queue_vector_t;
typedef delegate<void(bool)> pool_signal_t;

struct Waiting {
  Waiting(tcp_ptr);

  tcp_ptr conn;
  queue_vector_t buffers;
  int total = 0;
};

struct Nodes;
struct Session {
  Session(Nodes&, int idx, tcp_ptr inc, tcp_ptr out);
  void handle_timeout();

  Nodes&    parent;
  const int self;
  int       timeout_timer;
  tcp_ptr   incoming;
  tcp_ptr   outgoing;
};

struct Node {
  Node(netstack_t& stk, net::Socket a, pool_signal_t sig)
    : stack(stk), addr(a), pool_signal(sig) {}

  auto address() const noexcept { return this->addr; }
  int  connection_attempts() const noexcept { return this->connecting; }
  int  pool_size() const noexcept { return pool.size(); }

  void maintain_pool(const int);
  tcp_ptr get_connection();

private:
  void connect();

  netstack_t& stack;
  net::Socket addr;
  pool_signal_t pool_signal;
  signed int  connecting = 0;
  std::vector<tcp_ptr> pool;
};

struct Nodes {
  Nodes(const int POOL);

  int64_t total_sessions() const;
  int open_sessions() const;
  int pool_size() const;
  int pool_connecting() const;
  int pool_connections() const;

  template <typename... Args>
  void add_node(Args&&... args);
  bool assign(tcp_ptr, queue_vector_t);
  void maintain_pools();
  void create_session(tcp_ptr inc, tcp_ptr out);
  Session& get_session(int);
  void close_session(int);

private:
  std::vector<Node> nodes;
  const int POOL_SIZE;
  int64_t   session_total = 0;
  int       session_cnt = 0;
  int       iterator = -1;
  std::vector<Session> sessions;
  std::vector<int> free_sessions;
};

struct Balancer {
  Balancer(netstack_t& in, uint16_t port,
           netstack_t& out, std::vector<net::Socket> nodes, int max_pool);

  void incoming(tcp_ptr);
  int  wait_queue() const;

  netstack_t& netin;
  Nodes nodes;

private:
  void handle_waitq(bool result);

  std::deque<Waiting> queue;
};

template <typename... Args>
inline void Nodes::add_node(Args&&... args) {
  nodes.emplace_back(std::forward<Args> (args)...);
}
