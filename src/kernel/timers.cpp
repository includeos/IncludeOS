#include <kernel/timers.hpp>

#include <kernel/os.hpp>
#include <service>
#include <smp>
#include <statman>
#include <map>
#include <vector>

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

struct alignas(SMP_ALIGN) timer_system
{
  bool     is_running  = false;
  uint32_t dead_timers = 0;
  Timers::start_func_t arch_start_func;
  Timers::stop_func_t  arch_stop_func;
  std::vector<Timer>   timers;
  std::vector<id_t>    free_timers;
  // timers sorted by timestamp
  std::multimap<duration_t, id_t> scheduled;
  /** Stats */
  int64_t*  oneshot_started;
  int64_t*  oneshot_stopped;
  uint32_t* periodic_started;
  uint32_t* periodic_stopped;
};

static std::array<timer_system, SMP_MAX_CORES> systems;

static inline timer_system& get() {
  return PER_CPU(systems);
}

void Timers::init(const start_func_t& start, const stop_func_t& stop)
{
  auto& system = get();
  // architecture specific start and stop functions
  system.arch_start_func = start;
  system.arch_stop_func  = stop;
  
  std::string CPU = "cpu" + std::to_string(SMP::cpu_id());
  system.oneshot_started = (int64_t*) &Statman::get().create(Stat::UINT64, CPU + ".timers.oneshot_started").get_uint64();
  system.oneshot_stopped = (int64_t*) &Statman::get().create(Stat::UINT64, CPU + ".timers.oneshot_stopped").get_uint64();
  system.periodic_started = &Statman::get().create(Stat::UINT32, CPU + ".timers.periodic_started").get_uint32();
  system.periodic_stopped = &Statman::get().create(Stat::UINT32, CPU + ".timers.periodic_stopped").get_uint32();
}

bool Timers::is_ready()
{
  return signal_ready;
}
void Timers::ready()
{
  signal_ready = true;
  // begin processing timers if any are queued
  if (get().is_running == false) {
    timers_handler();
  }
  // call Service::ready(), because timer system is ready!
  Service::ready();
}

id_t Timers::periodic(duration_t when, duration_t period, handler_t handler)
{
  auto& system = get();
  id_t id;

  if (UNLIKELY(system.free_timers.empty()))
  {
    if (LIKELY(system.dead_timers))
    {
      // look for dead timer
      auto it = system.scheduled.begin();
      while (it != system.scheduled.end()) {
        // take over this timer, if dead
        id_t id = it->second;

        if (system.timers[id].deferred_destruct)
        {
          system.dead_timers--;
          // remove from schedule
          system.scheduled.erase(it);
          // reset timer
          system.timers[id].reset();
          // reuse timer
          new (&system.timers[id]) Timer(period, handler);
          sched_timer(when, id);

          // Stat increment timer started
          if (system.timers[id].is_oneshot())
            (*system.oneshot_started)++;
          else
            (*system.periodic_started)++;

          return id;
        }
        ++it;
      }
    }
    id = system.timers.size();
    // occupy new slot
    system.timers.emplace_back(period, handler);
  }
  else {
    // get free timer slot
    id = system.free_timers.back();
    system.free_timers.pop_back();

    // occupy free slot
    new (&system.timers[id]) Timer(period, handler);
  }

  // immediately schedule timer
  sched_timer(when, id);

  // Stat increment timer started
  if (system.timers[id].is_oneshot())
    (*system.oneshot_started)++;
  else
    (*system.periodic_started)++;

  return id;
}

void Timers::stop(id_t id)
{
  auto& system = get();
  if (LIKELY(system.timers[id].deferred_destruct == false))
  {
    // mark as dead already
    system.timers[id].deferred_destruct = true;
    // free resources immediately
    system.timers[id].callback.reset();
    system.dead_timers++;

    if (system.timers[id].is_oneshot())
      (*system.oneshot_stopped)++;
    else
      (*system.periodic_stopped)++;
  }
}

size_t Timers::active()
{
  auto& system = get();
  return system.scheduled.size();
}

/// time functions ///

static inline std::chrono::microseconds now() noexcept
{
  return microseconds(OS::micros_since_boot());
}

/// scheduling ///

void Timers::timers_handler()
{
  auto& system = get();
  // assume the hardware timer called this function
  system.is_running = false;

  while (LIKELY(!system.scheduled.empty()))
  {
    auto it = system.scheduled.begin();
    auto when = it->first;
    id_t id   = it->second;

    // remove dead timers
    if (system.timers[id].deferred_destruct)
    {
      system.dead_timers--;
      // remove from schedule
      system.scheduled.erase(it);
      // delete timer
      system.timers[id].reset();
      system.free_timers.push_back(id);
    }
    else
    {
      auto ts_now = now();

      if (ts_now >= when) {
        // erase immediately
        system.scheduled.erase(it);

        // call the users callback function
        system.timers[id].callback(id);
        // if the timers struct was modified in callback, eg. due to
        // creating a timer, then the timer reference below would have
        // been invalidated, hence why its BELOW, AND MUST STAY THERE
        auto& timer = system.timers[id];

        // oneshot timers are automatically freed
        if (timer.deferred_destruct || timer.is_oneshot())
        {
          timer.reset();
          if (timer.deferred_destruct) system.dead_timers--;
          system.free_timers.push_back(id);
        }
        else if (timer.is_oneshot() == false)
        {
          // if the timer is recurring, we will simply reschedule it
          // NOTE: we are carefully using (when + period) to avoid drift
          system.scheduled.
            emplace(std::piecewise_construct,
                    std::forward_as_tuple(when + timer.period),
                    std::forward_as_tuple(id));
        }

      } else {
        // not yet time, so schedule it for later
        system.is_running = true;
        system.arch_start_func(when - ts_now);
        // exit early, because we have nothing more to do,
        // and there is a deferred handler
        return;
      }
    }
  }
  // stop hardware timer, since no timers are enabled
  system.arch_stop_func();
}
static void sched_timer(duration_t when, id_t id)
{
  auto& system = get();
  system.scheduled.
    emplace(std::piecewise_construct,
            std::forward_as_tuple(now() + when),
            std::forward_as_tuple(id));

  // dont start any hardware until after calibration
  if (UNLIKELY(!signal_ready)) return;

  // if the hardware timer is not running, try starting it
  if (UNLIKELY(system.is_running == false)) {
    Timers::timers_handler();
    return;
  }
  // if the scheduled timer is the new front, restart timer
  auto it = system.scheduled.begin();
  if (it->second == id)
      Timers::timers_handler();
}
