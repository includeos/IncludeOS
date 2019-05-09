#include <kernel/timers.hpp>

#include <os.hpp>
#include <kernel/events.hpp>
#include <kernel/rtc.hpp>
#include <service>
#include <smp>
#include <statman>
#include <map>
#include <vector>

using namespace std::chrono;
typedef Timers::duration_t duration_t;
typedef Timers::handler_t  handler_t;

/// time functions ///

static inline auto now() noexcept
{
  return nanoseconds(RTC::nanos_now());
}

/// internal timer ///

struct SystemTimer
{
  SystemTimer(duration_t t, duration_t p, handler_t cb)
    : time(t), period(p), callback(std::move(cb)) {}

  SystemTimer(SystemTimer&& other)
    : time(other.time), period(other.period),
      callback(std::move(other.callback)),
      already_dead(other.already_dead) {}

  bool is_alive() const noexcept {
    return already_dead == false;
  }
  bool is_oneshot() const noexcept {
    return period.count() == 0;
  }
  void reset() {
    callback.reset();
    already_dead = false;
  }

  duration_t time;
  duration_t period;
  handler_t  callback;
  bool already_dead = false;
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
  void free_timer(Timers::id_t);
  void sched_timer(duration_t when, Timers::id_t);

  bool     is_running  = false;
  int      interrupt = 0;
  Timers::start_func_t arch_start_func;
  Timers::stop_func_t  arch_stop_func;
  std::vector<SystemTimer>  timers;
  std::vector<Timers::id_t> free_timers;
  // timers sorted by timestamp
  std::multimap<duration_t, Timers::id_t> scheduled;
  /** Stats */
  union {
    int64_t  i64 = 0;
    uint32_t u32;
  } dummy;
  int64_t*  oneshot_started = &dummy.i64;
  int64_t*  oneshot_stopped = &dummy.i64;
  uint32_t* periodic_started = &dummy.u32;
  uint32_t* periodic_stopped = &dummy.u32;
};
static SMP::Array<timer_system> systems;

void timer_system::free_timer(Timers::id_t id)
{
  this->timers[id].reset();
  this->free_timers.push_back(id);
}

static inline timer_system& get() {
  return PER_CPU(systems);
}

void Timers::init(const start_func_t& start, const stop_func_t& stop)
{
  auto& system = get();
  // event for processing timers
  system.interrupt = Events::get().subscribe(&Timers::timers_handler);
  // architecture specific start and stop functions
  system.arch_start_func = start;
  system.arch_stop_func  = stop;

  const std::string CPU = "cpu" + std::to_string(SMP::cpu_id());
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
  if (SMP::cpu_id() == 0)
  {
    // call Service::ready(), because timer system is ready!
    Service::ready();
  }
}

Timers::id_t Timers::periodic(duration_t when, duration_t period, handler_t handler)
{
  assert(handler != nullptr && "Callback function cannot be null");
  auto& system = get();
  Timers::id_t id;
  auto real_time = now() + when;

  if (UNLIKELY(system.free_timers.empty()))
  {
    id = system.timers.size();
    // occupy new slot
    system.timers.emplace_back(real_time, period, handler);
  }
  else {
    // get free timer slot
    id = system.free_timers.back();
    system.free_timers.pop_back();

    // occupy free slot
    new (&system.timers[id]) SystemTimer(real_time, period, handler);
  }

  // Stat increment timer started
  if (system.timers[id].is_oneshot()) {
    if (system.oneshot_started) (*system.oneshot_started)++;
  } else {
    if (system.periodic_started) (*system.periodic_started)++;
  }

  // immediately schedule timer
  system.sched_timer(real_time, id);
  return id;
}

void Timers::stop(Timers::id_t id)
{
  auto& system = get();
  if (UNLIKELY(system.timers.at(id).already_dead)) return;

  auto& timer = system.timers[id];
  // mark as dead already
  timer.already_dead = true;
  // free resources immediately
  timer.callback.reset();
  // search for timer in scheduled
  auto it = system.scheduled.find(timer.time);
  for (; it != system.scheduled.end(); ++it) {
    // found dead timer
    if (id == it->second) {
      // erase from schedule
      system.scheduled.erase(it);
      // free from system
      system.free_timer(id);
      break;
    }
  }
  // timer stats
  if (system.timers[id].is_oneshot())
    (*system.oneshot_stopped)++;
  else
    (*system.periodic_stopped)++;
}

size_t Timers::active() {
  return get().scheduled.size();
}
size_t Timers::existing() {
  return get().timers.size();
}
size_t Timers::free() {
  return get().free_timers.size();
}

/// scheduling ///

duration_t Timers::next()
{
  auto& system = get();
  if (LIKELY(!system.scheduled.empty()))
  {
    auto it   = system.scheduled.begin();
    auto when = it->first;
    auto diff = when - now();
    // avoid returning zero or negative diff
    if (diff < nanoseconds(1)) return nanoseconds(1);
    return diff;
  }
  return duration_t::zero();
}

void Timers::timers_handler()
{
  auto& system = get();
  // assume the hardware timer called this function
  system.is_running = false;

  while (LIKELY(!system.scheduled.empty()))
  {
    auto it         = system.scheduled.begin();
    auto when       = it->first;
    Timers::id_t id = it->second;

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
      if (timer.already_dead || timer.is_oneshot())
      {
        system.free_timer(id);
      }
      else
      {
        // if the timer is recurring, we will simply reschedule it
        // NOTE: we are carefully using (when + period) to avoid drift
        auto new_time = when + timer.period;
        // update timers self-time
        timer.time = new_time;
        // reschedule
        system.scheduled.
          emplace(std::piecewise_construct,
                  std::forward_as_tuple(new_time),
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
  // stop hardware timer, since no timers are enabled
  system.arch_stop_func();
}
void timer_system::sched_timer(duration_t when, Timers::id_t id)
{
  this->scheduled.
    emplace(std::piecewise_construct,
            std::forward_as_tuple(when),
            std::forward_as_tuple(id));

  // dont start any hardware until after calibration
  if (UNLIKELY(!signal_ready)) return;

  // if the hardware timer is not running, try starting it
  if (UNLIKELY(this->is_running == false)) {
    Events::get().trigger_event(this->interrupt);
    return;
  }
  // if the scheduled timer is the new front, restart timer
  auto it = this->scheduled.begin();
  if (it->second == id) {
    Events::get().trigger_event(this->interrupt);
  }
}
