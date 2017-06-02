#include <kernel/os.hpp>
#include "liveupdate.hpp"
#include "common.hpp"
using namespace liu;

static std::vector<double> timestamps;
static buffer_t bloberino;

void boot_save(Storage& storage, const buffer_t* blob)
{
  storage.add_vector(0, timestamps);
  storage.add<int64_t> (1, OS::cycles_since_boot());
  assert(blob != nullptr);
  storage.add_buffer(2, *blob);
}
void boot_resume_all(Restore& thing)
{
  timestamps = thing.as_vector<double>(); thing.go_next();
  // calculate time spent
  auto t1 = thing.as_type<int64_t>(); thing.go_next();
  auto t2 = OS::cycles_since_boot();
  double time = (t2-t1) / (OS::cpu_freq().count() * 1000.0);
  // add time
  timestamps.push_back(time);
  // retrieve old blob
  bloberino = thing.as_buffer(); thing.go_next();
  thing.pop_marker();
}

LiveUpdate::storage_func begin_test_boot()
{
  bool resumed = LiveUpdate::resume(LIVEUPD_LOCATION, boot_resume_all);
  if (resumed)
  {
    // calculate median by sorting
    std::sort(timestamps.begin(), timestamps.end());
    double median = timestamps[timestamps.size()/2];
    // show information
    printf("Median boot time over %lu samples: %f millis\n",
            timestamps.size(), median);
    if (timestamps.size() >= 30)
    {
      for (auto& stamp : timestamps) {
        printf("%f\n", stamp);
      }
      OS::shutdown();
    }
    else {
      // immediately liveupdate
      LiveUpdate::begin(LIVEUPD_LOCATION, bloberino, boot_save);
    }
  }
  // wait for update
  return boot_save;
}
