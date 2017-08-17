#include <kernel/os.hpp>
#include "liveupdate.hpp"
#include "common.hpp"
using namespace liu;

static std::vector<int64_t> timestamps;
static buffer_t bloberino;
static bool is_saved = false;
//static const size_t NUM_ITEMS = 1024*1024 * 45;

static void boot_save(Storage& storage, const buffer_t* blob)
{
  timestamps.push_back(OS::micros_since_boot());
  storage.add_vector(0, timestamps);
  storage.add_int(1, true);
  //assert(blob != nullptr);
  //storage.add_buffer(2, *blob);
  /*
  for (int i = 0; i < NUM_ITEMS; i++) {
    storage.add_int(69, i);
  }*/
  //std::vector<char> data(NUM_ITEMS);
  //storage.add_buffer(69, std::move(data));
  if (is_saved == false) {
    *(size_t*) SIZE_LOCATION = blob->size();
    memcpy(DATA_LOCATION, blob->data(), blob->size());
  }
}
static void boot_resume_all(Restore& thing)
{
  timestamps = thing.as_vector<int64_t>(); thing.go_next();
  // calculate time spent
  auto t1 = timestamps.back();
  auto t2 = OS::micros_since_boot();
  // set final time
  timestamps.back() = t2 - t1;
  // retrieve old blob
  is_saved = thing.as_int(); thing.go_next();
  //bloberino = thing.as_buffer(); thing.go_next();
  /*
  // retrieve and validate "items"
  while (thing.get_id() == 69) {
    assert(thing.is_int());
    int value = thing.as_int();
    static int next = 0;
    assert(value == next++);
    thing.go_next();
  }*/
  //assert(thing.get_id() == 69);
  //auto buffer = thing.as_buffer(); thing.go_next();

  thing.pop_marker();
}

LiveUpdate::storage_func begin_test_boot()
{
  bool resumed = LiveUpdate::resume(LIVEUPD_LOCATION, boot_resume_all);
  if (resumed)
  {
    if (timestamps.size() >= 30)
    {
      printf("First sample: %llu\n", timestamps.front());
      // calculate median by sorting
      std::sort(timestamps.begin(), timestamps.end());
      auto median = timestamps[timestamps.size()/2];
      // show information
      printf("Median boot time over %lu samples: %llu micros\n",
              timestamps.size(), median);
      for (auto& stamp : timestamps) {
        printf("%lld\n", stamp);
      }
      OS::shutdown();
    }
    else {
      char*   LOC1 = (char*) DATA_LOCATION;
      size_t* LOC0 = (size_t*) SIZE_LOCATION;
      if (bloberino.empty()) {
        bloberino.assign(LOC1, LOC1 + *LOC0);
      }
      // immediately liveupdate
      LiveUpdate::begin(LIVEUPD_LOCATION, bloberino, boot_save);
    }
  }
  // wait for update
  return boot_save;
}
