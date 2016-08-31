#include <kernel/timers.hpp>

#include <map>
#include <vector>
#include <kernel/os.hpp>
#include <statman>

using namespace std::chrono;
typedef Timers::id_t       id_t;
typedef Timers::duration_t duration_t;
typedef Timers::handler_t  handler_t;

static void sched_timer(duration_t when, id_t id);

struct Timer
{
  Timer(duration_t p, handler_t cb)
    : period(p), callback(cb), deferred_destruct(false) {}

  bool is_alive() const noexcept {
    return deferred_destruct == false;
  }

  bool is_oneshot() const noexcept {
    return period.count() == 0;
  }

  void reset() {
    callback.reset();
    deferred_destruct = false;
  }

  duration_t period;
  handler_t  callback;
  bool deferred_destruct = false;
};

/**
 * 1. There are no restrictions on when timers can be started or stopped
 * 2. A period of 0 means start a one-shot timer
 * 3. Each timer is a separate object living in a "fixed" vector
 * 4. A dead timer is simply a timer which has its handler reset, as well as
 *     having been removed from schedule
 * 5. No timer may be scheduled more than once at a time, as that will needlessly
 *     inflate the schedule container, as well as complicate stopping timers
 * 6. Free timer IDs are retrieved from a stack of free timer IDs (or through
 *     expanding the "fixed" vector)
**/

static bool signal_ready = false;
static bool is_running   = false;
static uint32_t dead_timers = 0;
static Timers::start_func_t arch_start_func;
static Timers::stop_func_t  arch_stop_func;
static std::vector<Timer>   timers;
static std::vector<id_t>    free_timers;
// timers sorted by timestamp
static std::multimap<duration_t, id_t> scheduled;
/** Stats */
static uint64_t& oneshot_started_{Statman::get().create(Stat::UINT64, "timers.oneshot_started").get_uint64()};
static uint64_t& oneshot_stopped_{Statman::get().create(Stat::UINT64, "timers.oneshot_stopped").get_uint64()};
static uint32_t& periodic_started_{Statman::get().create(Stat::UINT32, "timers.periodic_started").get_uint32()};
static uint32_t& periodic_stopped_{Statman::get().create(Stat::UINT32, "timers.periodic_stopped").get_uint32()};

void Timers::init(const start_func_t& start, const stop_func_t& stop)
{
  // architecture specific start and stop functions
  arch_start_func = start;
  arch_stop_func  = stop;
}

void Timers::ready()
{
  assert(signal_ready == false);
  signal_ready = true;
  // begin processing timers if any are queued
  if (is_running == false) {
    timers_handler();
  }
}

id_t Timers::periodic(duration_t when, duration_t period, const handler_t& handler)
{
  id_t id;

  if (UNLIKELY(free_timers.empty())) {

    if (LIKELY(dead_timers)) {
      // look for dead timer
      auto it = scheduled.begin();
      while (it != scheduled.end()) {
        // take over this timer, if dead
        id_t id = it->second;

        if (timers[id].deferred_destruct) {
          dead_timers--;
          // remove from schedule
          scheduled.erase(it);
          // reset timer
          timers[id].reset();
          // reuse timer
          new (&timers[id]) Timer(period, handler);
          sched_timer(when, id);

          // Stat increment timer started
          if(timers[id].is_oneshot())
            oneshot_started_++;
          else
            periodic_started_++;

          return id;
        }
        ++it;
      }
    }
    id = timers.size();
    // occupy new slot
    timers.emplace_back(period, handler);
  }
  else {
    // get free timer slot
    id = free_timers.back();
    free_timers.pop_back();

    // occupy free slot
    new (&timers[id]) Timer(period, handler);
  }

  // immediately schedule timer
  sched_timer(when, id);

  // Stat increment timer started
  if(timers[id].is_oneshot())
    oneshot_started_++;
  else
    periodic_started_++;

  return id;
}

void Timers::stop(id_t id)
{
  if (LIKELY(timers[id].deferred_destruct == false)) {
    // mark as dead already
    timers[id].deferred_destruct = true;
    // free resources immediately
    timers[id].callback.reset();
    dead_timers++;

    if (timers[id].is_oneshot())
      oneshot_stopped_++;
    else
      periodic_stopped_++;
  }
}

size_t Timers::active()
{
  return scheduled.size();
}

/// time functions ///

inline std::chrono::microseconds now() noexcept
{
  return microseconds(OS::micros_since_boot());
}

/// scheduling ///

void Timers::timers_handler()
{
  // assume the hardware timer called this function
  is_running = false;

  while (LIKELY(!scheduled.empty()))
  {
    auto it = scheduled.begin();
    auto when = it->first;
    id_t id   = it->second;

    // remove dead timers
    if (timers[id].deferred_destruct) {
      dead_timers--;
      // remove from schedule
      scheduled.erase(it);
      // delete timer
      timers[id].reset();
      free_timers.push_back(id);
    }
    else
    {
      auto ts_now = now();

      if (ts_now >= when) {
        // erase immediately
        scheduled.erase(it);

        // call the users callback function
        timers[id].callback(id);
        // if the timers struct was modified in callback, eg. due to
        // creating a timer, then the timer reference below would have
        // been invalidated, hence why its BELOW, AND MUST STAY THERE
        auto& timer = timers[id];

        // oneshot timers are automatically freed
        if (timer.deferred_destruct || timer.is_oneshot())
        {
          timer.reset();
          if (timer.deferred_destruct) dead_timers--;
          free_timers.push_back(id);
        }
        else if (timer.is_oneshot() == false)
        {
          // if the timer is recurring, we will simply reschedule it
          // NOTE: we are carefully using (when + period) to avoid drift
          scheduled.emplace(std::piecewise_construct,
                    std::forward_as_tuple(when + timer.period),
                    std::forward_as_tuple(id));
        }

      } else {
        // not yet time, so schedule it for later
        is_running = true;
        arch_start_func(when - ts_now);
        // exit early, because we have nothing more to do,
        // and there is a deferred handler
        return;
      }
    }
  }
  // stop hardware timer, since no timers are enabled
  arch_stop_func();
}
static void sched_timer(duration_t when, id_t id)
{
  scheduled.emplace(std::piecewise_construct,
            std::forward_as_tuple(now() + when),
            std::forward_as_tuple(id));

  // dont start any hardware until after calibration
  if (UNLIKELY(!signal_ready)) return;

  // if the hardware timer is not running, try starting it
  if (UNLIKELY(is_running == false)) {
    Timers::timers_handler();
    return;
  }
  // if the scheduled timer is the new front, restart timer
  auto it = scheduled.begin();
  if (it->second == id)
      Timers::timers_handler();
}
