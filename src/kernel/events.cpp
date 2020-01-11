
#include <kernel/events.hpp>
#include <arch.hpp>
#include <algorithm>
#include <cassert>
#include <statman>
#include <smp>
//#define DEBUG_SMP

static std::vector<Events> managers;
SMP_RESIZE_EARLY_GCTOR(managers);

Events& Events::get(int cpuid)
{
  return managers.at(cpuid);
}
Events& Events::get()
{
  return PER_CPU(managers);
}

void Events::init_local()
{
  if (SMP::cpu_id() == 0)
  {
    // prevent legacy IRQs from being free for taking
    for (int evt = 0; evt < 32; evt++)
        event_subs[evt] = true;
  }
}

uint8_t Events::subscribe(event_callback func)
{
  for (int evt = 0; evt < NUM_EVENTS; evt++) {
    if (event_subs[evt] == false) {
      subscribe(evt, func);
      return evt;
    }
  }
  throw std::out_of_range("No more free events");
}
void Events::subscribe(uint8_t evt, event_callback func)
{
  // Mark as subscribed to
  event_subs[evt] = true;
  // Set (new) callback for event
  callbacks[evt] = func;
  // add to sublist if not there already
  auto it = std::find(sublist.begin(), sublist.end(), evt);
  if (it == sublist.end())
  {
    sublist.push_back(evt);
#ifdef DEBUG_SMP
    SMP::global_lock();
    printf("Subscribed to intr=%u irq=%u on cpu %d\n",
           IRQ_BASE + evt, evt, SMP::cpu_id());
    SMP::global_unlock();
#endif
    // enable IRQ in hardware
    __arch_subscribe_irq(evt);
  }
}
void Events::unsubscribe(uint8_t evt)
{
  event_subs[evt] = false;
  callbacks[evt] = nullptr;
  for (auto it = sublist.begin(); it != sublist.end(); ++it) {
    if (*it == evt) {
      sublist.erase(it); return;
    }
  }
  throw std::out_of_range("Event was not in sublist?");
}

void Events::defer(event_callback callback)
{
  auto ev = subscribe(nullptr);
  subscribe(ev, event_callback::make_packed(
    [this, ev, callback] () {
      callback();
      // NOTE: we cant unsubscribe before after callback(),
      // because unsubscribe() deallocates event storage
      this->unsubscribe(ev);
    }));
  // and trigger it once
  event_pend[ev] = true;
}

void Events::process_events()
{
  bool handled_any;
  do {
    handled_any = false;

    for (const uint8_t intr : sublist)
    if (event_pend[intr])
    {
      event_pend[intr] = false;
      // call handler
#ifdef DEBUG_SMP
      if (intr != 0) {
        SMP::global_lock();
        printf("[cpu%d] Calling handler for intr=%u irq=%u\n",
                SMP::cpu_id(), IRQ_BASE + intr, intr);
        SMP::global_unlock();
      }
#endif
      callbacks[intr]();
      // increment events handled
      handled_array[intr]++;
      handled_any = true;
    }
  } while (handled_any);
}
