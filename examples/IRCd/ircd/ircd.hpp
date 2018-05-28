#pragma once
#include <net/inet>
#include <rtc>
#include <vector>
#include <liveupdate>

#include "common.hpp"
#include "client.hpp"
#include "channel.hpp"
#include "server.hpp"

#include "perf_array.hpp"

#define STAT_TOTAL_CONNS           0
#define STAT_TOTAL_USERS           1
#define STAT_LOCAL_USERS           2
#define STAT_REGGED_USERS          3
#define STAT_OPERATORS             4
#define STAT_CHANNELS              5
#define STAT_MAX_USERS             6

namespace liu {
  struct Storage;
  struct Restore;
}
struct RemoteServer {
  std::string    sname;
  std::string    spass;
  net::IP4::addr address;
  uint16_t       port;
};

class IrcServer {
public:
  using Connection = net::tcp::Connection_ptr;
  using Network    = net::Inet;
  typedef std::function<const std::string&()> motd_func_t;

  IrcServer(
      Network& cli_inet,
      uint16_t client_port,
      Network& srv_inet,
      uint16_t server_port,
      uint16_t id,
      const std::string& name,
      const std::string& network);

  const std::string& name() const noexcept {
    return server_name;
  }
  const std::string& network() const noexcept {
    return network_name;
  }
  std::string version() const noexcept {
    return IRC_SERVER_VERSION;
  }
  void set_motd(motd_func_t func) noexcept {
    this->motd_func = std::move(func);
  }
  const std::string& get_motd() const noexcept {
    return motd_func();
  }
  // date server was created
  const std::string& created() const noexcept {
    return created_string;
  }
  // uptime in seconds
  long uptime() const noexcept {
    return create_timestamp() - this->created_ts;
  }
  // server id / token
  uint8_t server_id() const noexcept {
    return this->srv_id;
  }
  char token() const noexcept {
    return 'A' + this->srv_id;
  }

  /// clients
  perf_array<Client, clindex_t> clients;
  void free_client(Client&);
  // stats keeping
  void new_registered_client();

  /// channels
  perf_array<Channel, chindex_t> channels;
  // create channel on server
  chindex_t create_channel(const std::string&);
  // opposite of create channel
  void free_channel(Channel&);
  // true if the string is a valid channel name
  static bool is_channel(const std::string& param) {
    if (param.empty()) return false;
    return Channel::is_channel_identifier(param[0]);
  }

  /// remote servers
  perf_array<Server, int> servers;
  void add_remote_server(RemoteServer as)
  {
    remote_server_list.push_back(as);
  }
  bool accept_remote_server(const std::string& name, const std::string& pass) const noexcept;
  void call_remote_servers();
  void kill_remote_clients_on(sindex_t, const std::string&);
  void begin_netburst(Server&);

  /// broadcassts
  // message local operators
  void wallops(const std::string&);
  // propagate message globally
  void broadcast(net::tcp::buffer_t, size_t);
  // send message to all users visible to user, including user
  void user_bcast(clindex_t user, const char* buffer, size_t len);
  void user_bcast(clindex_t user, const std::string& from, uint16_t tk, const std::string&);
  // send message to all users visible to user, except user
  void user_bcast_butone(clindex_t user, const char* buffer, size_t len);
  void user_bcast_butone(clindex_t user, const std::string& from, uint16_t tk, const std::string&);

  void sbcast(const std::string&);
  void sbcast_butone(sindex_t, const std::string&);

  // stats / counters
  void inc_counter(uint8_t counter) {
    statcounters[counter]++;
  }
  void dec_counter(uint8_t counter) {
    statcounters[counter]--;
  }
  int get_counter(uint8_t c) {
    return statcounters[c];
  }
  void set_counter(uint8_t c, int val) {
    statcounters[c] = val;
  }

  // server configuration stuff
  constexpr static
  uint8_t nick_minlen() noexcept {
    return 1;
  }
  constexpr static
  uint8_t nick_maxlen() noexcept {
    return 9;
  }
  constexpr static
  uint8_t chan_minlen() noexcept {
    return 1;
  }
  constexpr static
  uint8_t chan_maxlen() noexcept {
    return 16;
  }
  constexpr static
  uint8_t chan_max() noexcept {
    return 8;
  }
  constexpr static
  uint8_t client_maxchans() noexcept {
    return 10;
  }

  constexpr static
  std::chrono::seconds ping_timeout() noexcept {
    return std::chrono::seconds(60);
  }
  constexpr static
  std::chrono::seconds short_ping_timeout() noexcept {
    return std::chrono::seconds(5);
  }

  size_t clis() const noexcept {
    return clients.size();
  }
  size_t club() const noexcept {
    size_t sum = clients.size() * sizeof(Client);
    for (auto& cl : clients) {
      sum += cl.club();
    }
    return sum;
  }

  // create a now() timestamp
  long create_timestamp() const noexcept {
    return RTC::now();
  }

  void print_stuff() {
    int i = 0;
    int hmm = 0;
    for (auto& cl : clients) {

      if (cl.get_conn()->sendq_size() == 0) continue;

      printf("CL[%04d] sendq: %u b sendq rem: %u can send: %d queued: %d b\t",
          i++,
          cl.get_conn()->sendq_size(),
          cl.get_conn()->sendq_remaining(),
          cl.get_conn()->can_send(),
          cl.get_conn()->is_queued());
      printf(" %s\n", cl.get_conn()->state().to_string().c_str());
      if (cl.get_conn()->sendq_size()) hmm++;
    }
    printf("HMM: %d  TOTAL: %lu\n", hmm, clients.size());
  }

  Network& client_stack() noexcept { return cli_inet; }
  Network& server_stack() noexcept { return srv_inet; }

  static void init();
  static std::unique_ptr<IrcServer> from_config();
private:
  size_t to_current = 0;
  bool  init_liveupdate();
  // liveupdate serialization
  void serialize(liu::Storage& storage, const liu::buffer_t*);
  void deserialize(liu::Restore& thing);

  Network&    cli_inet;
  Network&    srv_inet;
  uint16_t    srv_id = 0;
  std::string server_name;
  std::string network_name;

  // server callbacks
  motd_func_t motd_func = nullptr;

  // network
  std::vector<RemoteServer> remote_server_list;

  // performance stuff
  long cheapstamp;

  // statistics
  std::string created_string;
  long        created_ts;
  int statcounters[8] {0};
};
