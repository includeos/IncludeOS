#include <common.cxx>
#include <net/inet>
#include <net/interfaces>
#include <hw/async_device.hpp>

static std::unique_ptr<hw::Async_device<UserNet>> dev1 = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev2 = nullptr;

static void setup_inet()
{
  dev1 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev2 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev1->connect(*dev2);
  dev2->connect(*dev1);

  auto& inet_server = net::Interfaces::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  auto& inet_client = net::Interfaces::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});
}

CASE("Setup networks")
{
  setup_inet();
}

#include <chrono>
static inline auto now() {
  using namespace std::chrono;
  return duration_cast< milliseconds >(system_clock::now().time_since_epoch());
}

CASE("TCP benchmark")
{
  static const size_t CHUNK_SIZE = 1024 * 1024;
  static const size_t NUM_CHUNKS = 2; // smaller for coverage
  static std::chrono::milliseconds time_start;

  auto& inet_server = net::Interfaces::get(0);
  auto& inet_client = net::Interfaces::get(1);
  static bool done = false;

  // Set up a TCP server on port 80
  auto& server = inet_server.tcp().listen(80);
  // the shared buffer
  auto buf = net::tcp::construct_buffer(CHUNK_SIZE);

  // Add a TCP connection handler
  server.on_connect(
  [] (net::tcp::Connection_ptr conn) {

    conn->on_read(CHUNK_SIZE, [conn] (auto buf) {
        static size_t count_bytes = 0;

        assert(buf->size() <= CHUNK_SIZE);
        count_bytes += buf->size();

        if (count_bytes >= NUM_CHUNKS * CHUNK_SIZE) {

          auto timediff = now() - time_start;
          assert(count_bytes == NUM_CHUNKS * CHUNK_SIZE);

          double time_sec = timediff.count()/1000.0;
          double mbps = ((count_bytes * 8) / (1024.0 * 1024.0)) / time_sec;

          printf("Server received %zu Mb in %f sec. - %f Mbps \n",
                 count_bytes / (1024 * 1024), time_sec,  mbps);
          done = true;

          conn->close();
        }
      });
  });

  printf("Measuring memory <-> memory bandwidth...\n");
  time_start = now();
  inet_client.tcp().connect({net::ip4::Addr{"10.0.0.42"}, 80},
    [buf](auto conn)
    {
      if (not conn)
        std::abort();

      for (size_t i = 0; i < NUM_CHUNKS; i++)
        conn->write(buf);
    });

  while (!done)
  {
    Events::get().process_events();
  }
}
