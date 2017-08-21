#pragma once
#include <net/inet4>
#define READQ_PER_CLIENT    4096
#define MAX_READQ_PER_NODE  8192

typedef net::Inet<net::IP4> netstack_t;
typedef net::tcp::Connection_ptr tcp_ptr;
typedef std::vector< std::pair<net::tcp::buffer_t, size_t> > queue_vector_t;

struct Waiting {
  Waiting(tcp_ptr);

  tcp_ptr conn;
  queue_vector_t buffers;
  int total = 0;
};

struct Nodes;
struct Session {
  Session(Nodes&, int idx, tcp_ptr inc, tcp_ptr out);

  Nodes&    parent;
  const int self;
  tcp_ptr   incoming;
  tcp_ptr   outgoing;
};

struct Node {
  Node(net::Socket a) : addr(a) {}

  net::Socket addr;
  std::vector<tcp_ptr> pool;
  delegate<void()> pool_signal = nullptr;

  void connect(netstack_t&);
  tcp_ptr get_connection();
};

struct Nodes {
  Nodes(netstack_t& out, int sz);

  int64_t total_sessions() const;
  int open_sessions() const;
  int pool_size() const;
  int pool_connections() const;

  bool assign(tcp_ptr, queue_vector_t);
  void maintain_pool();
  void create_session(tcp_ptr inc, tcp_ptr out);
  Session& get_session(int);
  void close_session(int);

  netstack_t& netout;
  std::vector<Node> nodes;
  int pool_dynsize;

private:
  int64_t session_total = 0;
  int     session_cnt = 0;
  int     iterator = -1;
  int     pool_iterator = 0;
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
  void queue_check();

  std::deque<Waiting> queue;
  int64_t cps_total = 0;
  int64_t cps_last  = 0;
  int  pool_base_value;
  int  cps_timer = 0;
};
