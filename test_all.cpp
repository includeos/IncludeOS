#include <net/inet4>
#include <timers>
#include <util/crc32.hpp>
#include <cstdio>
#include "liveupdate.hpp"
#include "common.hpp"
using namespace liu;

typedef net::tcp::Connection_ptr Connection_ptr;
static std::vector<Connection_ptr> saveme;
static std::vector<std::string>    savemsg;

static const uint16_t TERM_PORT = 6667;

void setup_terminal_connection(Connection_ptr conn)
{
  saveme.push_back(conn);
  // retrieve binary
  conn->on_read(512,
  [conn] (net::tcp::buffer_t buf, size_t n)
  {
    std::string str((const char*) buf.get(), n);
    printf("Received message %u: %s",
          (uint32_t) savemsg.size()+1, str.c_str());
    // save for later as strings
    savemsg.push_back(str);
  });
  conn->on_close(
  [conn] {
    printf("Terminal %s closed\n", conn->to_string().c_str());
  });
}

void setup_terminal(net::Inet<net::IP4>& inet)
{
  // mini terminal
  printf("Setting up terminal on port %u\n", TERM_PORT);

  auto& term = inet.tcp().listen(TERM_PORT);
  term.on_connect(
  [] (auto conn) {
    setup_terminal_connection(conn);
    // write a string to change the state
    char BUFFER_CHAR = 'A';
    static uint32_t crc = CRC32_BEGIN();
    static const int LEN = 4096;
    auto buf = net::tcp::buffer_t(new uint8_t[LEN], std::default_delete<uint8_t[]>());
    memset(buf.get(), BUFFER_CHAR, LEN);

    conn->on_write(
    [conn, buf] (size_t) {
      crc = crc32(crc, (char*) buf.get(), LEN);
      printf("[%p] CRC32: %08x   %s\n", buf.get(), CRC32_VALUE(crc), conn->to_string().c_str());
    });

    for (int i = 0; i < 1000; i++) {
      conn->write(buf, LEN);
    }
  });
}

static void test_all_save(liu::Storage& storage, const liu::buffer_t* final_blob);
static void strings_and_buffers(Restore&);
static void the_timing(Restore&);
static void restore_term(Restore&);
static void saved_message(Restore&);
static void on_update_area(Restore&);
static void on_missing(Restore&);

LiveUpdate::storage_func begin_test_all(net::Inet<net::IP4>& inet)
{
  /// attempt to resume (if there is anything to resume)
  LiveUpdate::on_resume(0,   strings_and_buffers);
  LiveUpdate::on_resume(100, the_timing);
  LiveUpdate::on_resume(665, saved_message);
  LiveUpdate::on_resume(666, restore_term);
  LiveUpdate::on_resume(999, on_update_area);
  // begin restoring saved data
  if (LiveUpdate::resume(LIVEUPD_LOCATION, on_missing) == false) {
    printf("* Not restoring data, because no update has happened\n");
    // .. logic for when there is nothing to resume yet
  }

  // listen for telnet clients
  setup_terminal(inet);
  // show profile stats for boot
  //printf("%s\n", ScopedProfiler::get_statistics().c_str());
  //printf("This is the known good version :)\n");
  //panic("Oh noes! :(\n");
  return test_all_save;
}

static std::vector<double> timestamps;

void test_all_save(liu::Storage& storage, const liu::buffer_t* final_blob)
{
  storage.add_int(0, 1234);
  storage.add_int(0, 5678);

  storage.add_string(1, "Some string :(");
  storage.add_string(1, "Some other string :(");

  const char buffer[] = "Just some random buffer";
  storage.add_buffer(1, buffer, sizeof(buffer));

  std::vector<std::string> strvec;
  strvec.push_back("|String 1|");
  strvec.push_back("|String 2 is slightly longer|");
  storage.add_vector<std::string> (1, strvec);

  // store current timestamp using same ID = 100
  int64_t ts = OS::cycles_since_boot();
  storage.add<int64_t>(100, ts);

  // store vector of timestamps
  storage.add_vector<double> (100, timestamps);

  // where the update was stored last
  storage.add_buffer(999, *final_blob);

  // messages received from terminals
  storage.add_vector<std::string> (665, savemsg);

  // open terminals
  for (auto conn : saveme)
    if (conn->is_connected())
      storage.add_connection(666, conn);
}

void strings_and_buffers(liu::Restore& thing)
{
  int v1 = thing.as_int();      thing.go_next();
  assert(v1 == 1234);

  int v2 = thing.as_int();      thing.go_next();
  assert(v2 == 5678);

  auto str = thing.as_string(); thing.go_next();
  assert(str == "Some string :(");

  str = thing.as_string();      thing.go_next();
  assert(str == "Some other string :(");

  auto buffer = thing.as_buffer(); thing.go_next();
  // there is an extra zero at the end of the buffer
  str = std::string(buffer.data(), buffer.size()-1);
  assert(str == "Just some random buffer");

  auto vec = thing.as_vector<std::string> (); thing.go_next();
  assert(vec.size() == 2);
  assert(vec[0] == "|String 1|");
  assert(vec[1] == "|String 2 is slightly longer|");
}
void saved_message(liu::Restore& thing)
{
  auto vec = thing.as_vector<std::string> ();
  for (auto& str : vec)
  {
    static int n = 0;
    printf("[%d] %.*s", ++n, (int) str.size(), str.c_str());
    // re-save it
    //savemsg.push_back(str);
  }
}
void on_missing(liu::Restore& thing)
{
  printf("Missing resume function for %u\n", thing.get_id());
}

void the_timing(liu::Restore& thing)
{
  auto t1 = thing.as_type<int64_t>();
  auto t2 = OS::cycles_since_boot();
  //printf("! CPU ticks after: %lld  (CPU freq: %f)\n", t2, OS::cpu_freq().count());

  using namespace std::chrono;
  double  div  = OS::cpu_freq().count() * 1000.0;
  double  time = (t2-t1) / div;

  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
             "Boot time %.2f ms\n", time);

  savemsg.emplace_back(buffer, len);
  // verify that the next id is still same as current
  assert(thing.next_id() == thing.get_id());
  // next thing with pre-update timestamp
  thing.go_next();

  // restore timestamp vector
  timestamps = thing.as_vector<double> ();  thing.go_next();
  // add new update time
  timestamps.push_back(time);
  // median boot time over many updates
  std::sort(timestamps.begin(), timestamps.end());
  double median = timestamps[timestamps.size()/2];

  printf(">> %u timestamps, median TS: %.2f ms\n",
        (uint32_t) timestamps.size(), median);

}
void restore_term(liu::Restore& thing)
{
  auto& stack = net::Inet4::stack<0> ();
  // restore connection to terminal
  auto conn = thing.as_tcp_connection(stack.tcp());
  setup_terminal_connection(conn);
  printf("Restored terminal connection to %s\n", conn->remote().to_string().c_str());

  // send all the messages so far
  //for (auto msg : savemsg)
  //  conn->write(msg);
}

void on_update_area(liu::Restore& thing)
{
  auto updloc = thing.as_buffer();
  //printf("Reloading from %p:%d\n", updloc.buffer, updloc.length);

  // we are perpetually updating ourselves
  using namespace std::chrono;
  Timers::oneshot(milliseconds(250),
  [updloc] (auto) {
    printf("* Re-running previous update at %p vs heap %p\n",
            updloc.data(), heap_end);
    liu::LiveUpdate::begin(LIVEUPD_LOCATION, std::move(updloc), test_all_save);
  });
}
