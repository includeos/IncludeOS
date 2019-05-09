#include <os>
#include <net/interfaces>
#include <net/ws/connector.hpp>
#include <memdisk>
#include <https>
#include <deque>

// configuration
static const bool ENABLE_TLS    = true;
static const bool USE_BOTAN_TLS = false;
//static_assert(SMP_MAX_CORES > 1 || TCP_OVER_SMP == false, "SMP must be enabled");

struct alignas(SMP_ALIGN) HTTP_server
{
  http::Server* server = nullptr;
  net::Stream::buffer_t buffer = nullptr;
  net::WS_server_connector* ws_serve = nullptr;
};
static SMP::Array<HTTP_server> httpd;

bool accept_client(net::Socket remote, std::string origin)
{
  /*
  printf("Verifying origin: \"%s\"\n"
         "Verifying remote: \"%s\"\n",
         origin.c_str(), remote.to_string().c_str());
  */
  (void) origin;
  return remote.address() == net::ip4::Addr(10,0,0,1);
}

void websocket_service(net::TCP& tcp, uint16_t port)
{
  if (ENABLE_TLS)
  {
    if (USE_BOTAN_TLS)
    {
      auto& filesys = fs::memdisk().fs();
      // load CA certificate
      auto ca_cert = filesys.stat("/test.der");
      // load CA private key
      auto ca_key  = filesys.stat("/test.key");
      // load server private key
      auto srv_key = filesys.stat("/server.key");

      PER_CPU(httpd).server = new http::Botan_server(
            "blabla", ca_key, ca_cert, srv_key, tcp);
    }
    else
    {
      PER_CPU(httpd).server = new http::OpenSSL_server(
            "/test.pem", "/test.key", tcp);
    }
  }
  else
  {
    PER_CPU(httpd).server = new http::Server(tcp);
  }

  // buffer used for testing
  PER_CPU(httpd).buffer = net::Stream::construct_buffer(1200);

  // Set up server connector
  PER_CPU(httpd).ws_serve = new net::WS_server_connector(
    [&tcp] (net::WebSocket_ptr ws)
    {
      assert(SMP::cpu_id() == tcp.get_cpuid());
      // sometimes we get failed WS connections
      if (ws == nullptr) return;

      auto wptr = ws.release();
      // if we are still connected, attempt was verified and the handshake was accepted
      assert (wptr->is_alive());
      wptr->on_read =
      [] (auto message) {
        printf("WebSocket on_read: %.*s\n", (int) message->size(), message->data());
      };
      wptr->on_close =
      [wptr] (uint16_t) {
        delete wptr;
      };

      //socket->write("THIS IS A TEST CAN YOU HEAR THIS?");
      for (int i = 0; i < 1500; i++)
          wptr->write(PER_CPU(httpd).buffer, net::op_code::BINARY);

      wptr->close();
    },
    accept_client);
  PER_CPU(httpd).server->on_request(*PER_CPU(httpd).ws_serve);
  PER_CPU(httpd).server->listen(port);
  /// server ///
}

static void tcp_service(net::TCP& tcp)
{
  SMP_PRINT("On CPU %d with stack %p\n", SMP::cpu_id(), &tcp);

  // start a websocket server on @port
  websocket_service(tcp, 8000);
}

void Service::start()
{
  extern void create_network_device(int N, const char* ip);
  create_network_device(0, "10.0.0.1/24");

  auto& inet = net::Interfaces::get(0);
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

  // echo server
  inet.tcp().set_MSL(std::chrono::seconds(3));
  inet.tcp().listen(7,
    [] (auto conn)
    {
      auto* stream = new net::tcp::Stream(conn);

      stream->on_close(
        [stream] () {
          printf("TCP stream on_close\n");
          delete stream;
        });

      stream->write("Hei\n");
      delete stream;
    });

  // Read-only filesystem
  fs::memdisk().init_fs(
  [] (fs::error_t err, fs::File_system&) {
    assert(!err);
  });

  // run websocket server locally
  websocket_service(inet.tcp(), 8000);
}

#include <profile>
static void print_heap_info()
{
  const std::string heapinfo = HeapDiag::to_string();
  printf("%s\n", heapinfo.c_str());
  StackSampler::print(10);
}

void Service::ready()
{
  //auto stats = ScopedProfiler::get_statistics();
  //printf("%.*s\n", stats.size(), stats.c_str());
  using namespace net;

  printf("Size of TCP connection: 1x %zu 1000x %zu kB\n", sizeof(tcp::Connection), (1000*sizeof(tcp::Connection))/1024);
  printf("Size of TLS stream:     1x %u 1000x %u kB\n", 1024*100, 1000*100);
  printf("Size of messages:       1x %u 1000x %u kB\n", 1200*1500, (1000*1500*1200)/1024);

  using namespace std::chrono;
  Timers::periodic(1s, [] (int) {
    //print_heap_info();
  });

  StackSampler::begin();
}
