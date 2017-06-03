#include <kernel/os.hpp>
#include <net/inet4>
#include <statman>
#include <timers>
#include "liveupdate.hpp"
#include "common.hpp"
using namespace liu;
using namespace net;

static tcp::Connection_ptr conn = nullptr;
static net::TCP*        tcp_ptr = nullptr;
static buffer_t bloberino;
static void setup_callbacks(tcp::Connection_ptr);
static void open_for_business(net::TCP& tcp, uint16_t port);
static bool updated_yet = false;

struct measurement_t
{
  int64_t  received = 0;
  int64_t  ts = 0;
};
static measurement_t measurement;

void start_measure() {
  //printf("*** Starting measurements...\n");
  measurement.received = 0;
  measurement.ts       = OS::micros_since_boot();
  updated_yet = false;
}

void stop_measure() {
  auto   diff   = OS::micros_since_boot() - measurement.ts;
  double durs   = ((double)diff) / 1000 / 1000;
  double mbits  = (measurement.received/(1024*1024)*8) / durs;
  //printf("Duration: %.2f s - Payload: %lld/%u MB - %.2f MBit/s\n",
  //        durs, measurement.received/(1024*1024), SIZE/(1024*1024), mbits);
  printf("%f\n", mbits);
  if (updated_yet) {
    auto data = std::to_string(mbits) + "\n";
    auto& inet = net::Inet4::ifconfig();
    auto& sock = inet.udp().bind(667);
    sock.sendto({10,0,0,1}, 667, data.data(), data.size());
  }
}

#include <util/crc32.hpp>
static void tcpflow_save(Storage& storage, const buffer_t* blob)
{
  if (conn == nullptr)
      throw std::runtime_error("Not connected");

  storage.add_connection(0, conn);
  storage.add_buffer(1, *blob);
  storage.add(2, measurement);
  storage.put_marker(10);
}

static void tcpflow_resume(Restore& thing)
{
  conn = thing.as_tcp_connection(*tcp_ptr);
  setup_callbacks(conn);
  thing.go_next();
  bloberino = thing.as_buffer();
  thing.go_next();
  measurement = thing.as_type<measurement_t> ();
  thing.pop_marker(10);
  updated_yet = true;
}

void setup_callbacks(tcp::Connection_ptr conn)
{
  ::conn = conn;
  // receiving data
  conn->on_read(16384, [] (auto buf, size_t n)
  {
    measurement.received += n;
    if (measurement.received >= 512*1024*1024)
    {
      // record measurement
      stop_measure();
      // close this shit down
      ::conn->close();
      // reopen transfer port
      open_for_business(*tcp_ptr, 1337);
    }
  });
}

#include "server.hpp"
static void setup_liveupdate_server(net::Inet<net::IP4>& inet)
{
  // listen for live updates
  server(inet, 666,
  [] (liu::buffer_t& buffer) {
    // set blob
    printf("LiveUpdate blob received! Send file now...\n");
    bloberino = buffer;
  });
}
static void begin_live_updating(int)
{
  assert(bloberino.size() > 0);
  printf("* Live updating from %p (len=%u)\n",
          bloberino.data(), (uint32_t) bloberino.size());
  try
  {
    // run live update process
    liu::LiveUpdate::begin(LIVEUPD_LOCATION, bloberino, tcpflow_save);
  }
  catch (std::exception& err)
  {
    printf("Live Update location: %p\n", LIVEUPD_LOCATION);
    show_heap_stats();
    printf("Live update failed:\n%s\n", err.what());
  }
}

void open_for_business(net::TCP& tcp, uint16_t port)
{
  tcp.listen(port,
  [&tcp, port] (auto conn) {
    // close port
    assert(tcp.close({0, port}));
    if (!bloberino.empty()) {
      // auto-liveupdate after duration
      //Timers::oneshot(std::chrono::milliseconds(400), begin_live_updating);
    }
    // begin experiment
    setup_callbacks(conn);
    start_measure();
  });
}

LiveUpdate::storage_func begin_test_tcpflow(net::Inet<net::IP4>& inet)
{
  tcp_ptr = &inet.tcp();

  bool resumed = LiveUpdate::resume(LIVEUPD_LOCATION, tcpflow_resume);
  if (resumed == false)
  {
    // server for bloberino
    setup_liveupdate_server(inet);
    // setup listener for file data
    open_for_business(inet.tcp(), 1337);
  }

  // wait for update
  return tcpflow_save;
}
