#include <kernel/timer.hpp>

#include <map>
#include <vector>

typedef Timers::id_t        id_t;
typedef Timers::timestamp_t timestamp_t;
typedef Timers::handler_t   handler_t;
void sched_timer(timestamp_t when, id_t id);

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
id_t Timers::start(timestamp_t when, timestamp_t period, const handler_t& handler)
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
  scheduled.erase(it);
}

/// time functions ///
#include <kernel/os.hpp>

timestamp_t now()
{
  return (timestamp_t) OS::uptime();
}

void sched_timer(timestamp_t when, id_t id)
{
  /*
  scheduled.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(when));
  */
  scheduled.insert(
      std::forward_as_tuple(now() + when, id));
}

/// scheduling ///
