#include <kernel/timer.hpp>

#include <map>
#include <vector>
#include <hw/apic.hpp>

typedef Timers::id_t        id_t;
typedef Timers::timestamp_t timestamp_t;
typedef Timers::handler_t   handler_t;

void sched_timer(timestamp_t when, id_t id);
void stop_timer();

struct Timer
{
  Timer(timestamp_t p, handler_t h)
    : period(p), handler(h), deferred_destruct(false) {}
  
  timestamp_t period;
  handler_t   handler;
  bool deferred_destruct = false;
};

static std::vector<Timer> timers;
static std::vector<id_t>  free_timers;
static bool  is_running = false;

// timers sorted by timestamp
static std::multimap<timestamp_t, id_t> scheduled;

static id_t create_timer(
    timestamp_t when, timestamp_t period, const handler_t& handler)
{
  id_t id;
  
  if (free_timers.empty()) {
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
  
  // immediately schedule the first timer
  sched_timer(when, id);
  return id;
}
id_t Timers::create(timestamp_t when, timestamp_t period, const handler_t& handler)
{
  return create_timer(when, period, handler);
}

void Timers::stop(id_t id)
{
  // mark as free
  free_timers.push_back(id);
  // remove from scheduling
  auto it = scheduled.find(id);
  if (it == scheduled.end())
      return;
  
  // in the special case where the timer id is the current front,
  // we will need to also stop/reset the hardware timer
  if (it == scheduled.begin()) {
    stop_timer();
  }
  
  scheduled.erase(it);
}

/// time functions ///
/// ARCHITECTURE SPECIFIC ///
#include <kernel/os.hpp>

static timestamp_t now()
{
  return (timestamp_t) OS::uptime();
}
static void arch_sched(timestamp_t when, handler_t cb)
{
  hw::APIC::sched_timer(when, cb);
}
static void arch_stop()
{
  hw::APIC::stop_timer();
}

void timers_handler()
{
  while (!scheduled.empty())
  {
    auto it = scheduled.begin();
    auto when   = it->first;
    auto ts_now = now();
    auto& timer = timers[it->second];
    
    if (ts_now >= when) {
      // erase immediately
      scheduled.erase(scheduled.begin());
      
      // call handler
      timer.handler();
      
      // if the timer died during handler, free its resources
      if (timer.deferred_destruct) {
        timer.deferred_destruct = false;
        timer.handler.reset();
      }
      else if (timer.period)
      {
        // or, if the timer is recurring, we will want to reschedule it
        is_running = true;
        arch_sched(timer.period, timer.handler);
      }
      
    } else {
      // not yet past time, so schedule it for later
      is_running = true;
      arch_sched(when - ts_now, timer.handler);
      // exit early, because we have nothing more to do, and there is a deferred handler
      return;
    }
  }
  // if we get here, the list is empty,
  // and if the hardware timer is running, stop it
  if (is_running) {
    is_running = false;
    arch_stop();
  }
}
void sched_timer(timestamp_t when, id_t id)
{
  scheduled.insert(std::forward_as_tuple(now() + when, id));
  
  if (is_running == false) {
    timers_handler();
  }
  
}
void stop_timer()
{
  arch_stop();
}

/// scheduling ///
