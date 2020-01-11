#include <os.hpp>
#include <liveupdate>
#include <timers>
#include <statman>
#include <system_log>
#include <profile>
using namespace liu;

static std::vector<uint64_t> timestamps;
static uint8_t blob_data[1024*1024*16];
static size_t         blob_size = 0;

static void boot_save(Storage& storage)
{
  timestamps.push_back(os::nanos_since_boot());
  storage.add_vector(0, timestamps);
  // store binary blob for later
  auto blob = LiveUpdate::binary_blob();
  assert(blob.first != nullptr && blob.second != 0);
  storage.add_buffer(2, blob.first, blob.second);

  auto& stm = Statman::get();
  // increment number of updates performed
  try {
    ++stm.get_by_name("system.updates");
  }
  catch (const std::exception& e)
  {
    ++stm.create(Stat::UINT32, "system.updates");
  }
  stm.store(3, storage);
}
static void boot_resume_all(Restore& thing)
{
  timestamps = thing.as_vector<uint64_t>(); thing.go_next();
  // calculate time spent
  auto t1 = timestamps.back();
  auto t2 = os::nanos_since_boot();
  // set final time
  timestamps.back() = t2 - t1;
  // retrieve binary blob
{
  PROFILE("Retrieve ELF binary");
  memcpy(blob_data, thing.data(), thing.length());
  blob_size = thing.length();
  thing.go_next();
}
  // statman
  auto& stm = Statman::get();
  stm.restore(thing); thing.go_next();
  auto& stat = stm.get_by_name("system.updates");
  assert(stat.get_uint32() > 0);
  thing.pop_marker();
}

LiveUpdate::storage_func begin_test_boot()
{
  if (LiveUpdate::resume("test", boot_resume_all))
  {
    // OS must be able to tell it was live updated each time
    assert(LiveUpdate::os_is_liveupdated());

    if (timestamps.size() >= 30)
    {
      // calculate median by sorting
      std::sort(timestamps.begin(), timestamps.end());
      auto median = timestamps[timestamps.size()/2];
      // show information
      printf("Median boot time over %lu samples: %.2f millis\n",
              timestamps.size(), median / 1000000.0);
	  printf("Best time: %.2f millis  Worst time: %.2f millis\n",
	          timestamps.front() / 1000000.0, timestamps.back() / 1000000.0);
      /*
      for (auto& stamp : timestamps) {
        printf("%lld\n", stamp);
      }
      */
      for (auto& stat : Statman::get()) {
        printf("%s: %s\n", stat.name(), stat.to_string().c_str());
      }

      printf("Verifying that timers are started...\n");

      using namespace std::chrono;
      Timers::oneshot(5ms,[] (int) {
		extern int16_t __startup_was_fast;
		printf("Startup was fast: %d\n", __startup_was_fast);
        printf("SUCCESS\n");
        SystemLog::print_to(os::default_stdout);
      });
      return nullptr;
    }
    else {
      // immediately liveupdate
      LiveUpdate::exec(blob_data, blob_size, "test", boot_save);
    }
  }
  // wait for update
  return boot_save;
}
