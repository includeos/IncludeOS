#include <os>
#include <cassert>
#include <smp>

void Service::start()
{
  INFO("service", "Testing spinlocks!");

  Spinlock s;

  INFO2("Basic lock/unlock tests");

  if (s.is_locked()) {
    os::panic("Lock was locked at init");
  }

  s.lock();

  if (!s.is_locked()) {
    os::panic("Lock was not locked after call to lock()");
  }

  s.unlock();

  if (s.is_locked()) {
    os::panic("Lock was locked after call to unlock()");
  }

  INFO2("Testing try_lock*");
  if (!s.try_lock()) {
    os::panic("try_lock() didn't succeed");
  }

  // This will timeout
  INFO2("Waiting for try_lock_for to timeout");
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    if (s.try_lock_for(std::chrono::milliseconds(250))) {
      os::panic("try_lock_for() got lock, expected timeout");
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    if (t1 - t0 < std::chrono::milliseconds(250)) {
      os::panic("try_lock_for() returned earlier than timeout");
    }
  }

  s.unlock();

  // We should get this immediately
  INFO2("Testing try_lock_for on unlocked lock");
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    if (!s.try_lock_for(std::chrono::seconds(3))) {
      os::panic("try_lock_for didn't get lock");
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    if (t1 - t0 > std::chrono::milliseconds(250)) {
      os::panic("try_lock_for() didn't return immediately when lock was unlocked");
    }
  }

  s.unlock();

  // std::lock_guard test
  INFO2("Testing std::lock_guard");
  {
    std::lock_guard<Spinlock> lock(s);
    if (!s.is_locked()) {
      os::panic("std::lock_guard didn't lock");
    }
  }
  if (s.is_locked()) {
    os::panic("std::lock_guard didn't unlock");
  }

  exit(0);
}
