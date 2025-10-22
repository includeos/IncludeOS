#include <common.cxx>
#include <smp_utils>
#include <thread>
#include <barrier>

CASE("lock/unlock")
{
  Spinlock s;
  EXPECT(!s.is_locked());
  s.lock();
  EXPECT(s.is_locked());
  s.unlock();
  EXPECT(!s.is_locked());
}

CASE("std::lock_guard")
{
  Spinlock s;
  EXPECT(!s.is_locked());
  {
    std::lock_guard<Spinlock> lock(s);
    EXPECT(s.is_locked());
  }
  // Should be unlocked when lock_guard is out of scope
  EXPECT(!s.is_locked());
}

CASE("try_lock")
{
  Spinlock s;

  // Lock should not be taken
  EXPECT(!s.is_locked());
  // Try to lock, should succeed
  EXPECT(s.try_lock());

  // Should be locked now
  EXPECT(s.is_locked());
  // try_lock should now fail
  EXPECT(!s.try_lock());
  // We should still be locked, unlock
  EXPECT(s.is_locked());
  s.unlock();

  // Check that we're unlocked
  EXPECT(!s.is_locked());
  // try_lock should succeed again
  EXPECT(s.try_lock());
  // ... and the spinlock should be locked
  EXPECT(s.is_locked());
}

CASE("try_lock_for")
{
  Spinlock s;

  // Lock should not be taken
  EXPECT(!s.is_locked());

  // Try to lock, should succeed immediately
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    EXPECT(s.try_lock_for(std::chrono::seconds(1)));
    auto t1 = std::chrono::high_resolution_clock::now();
    EXPECT(t1 - t0 < std::chrono::milliseconds(250));
    EXPECT(s.is_locked());
  }

  // Now try again, this should fail, but take time
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    EXPECT(!s.try_lock_for(std::chrono::milliseconds(250)));
    auto t1 = std::chrono::high_resolution_clock::now();
    EXPECT(t1 - t0 > std::chrono::milliseconds(250));
    EXPECT(s.is_locked());
  }
}

CASE("try_lock_until")
{
  Spinlock s;

  // Lock should not be taken
  EXPECT(!s.is_locked());

  // Try to lock, should succeed immediately
  {
    auto t = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10);
    auto t0 = std::chrono::high_resolution_clock::now();
    EXPECT(s.try_lock_until(t));
    auto t1 = std::chrono::high_resolution_clock::now();
    EXPECT(t1 - t0 < std::chrono::milliseconds(250));
    EXPECT(s.is_locked());
  }

  // Try to lock, this should fail, but take time
  {
    auto t = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(250);
    auto t0 = std::chrono::high_resolution_clock::now();
    EXPECT(!s.try_lock_until(t));
    auto t1 = std::chrono::high_resolution_clock::now();
    EXPECT(t1 - t0 > std::chrono::milliseconds(250));
    EXPECT(s.is_locked());
  }
}

CASE("multithreaded - basic")
{
  Spinlock s;
  int val = 0;
  s.lock();

  std::thread t1([&](void)->void {
      s.lock();
      val++;
      s.unlock();
  });

  std::thread t2([&](void)->void {
      s.lock();
      val++;
      s.unlock();
  });


  EXPECT(val == 0);

  s.unlock();

  t1.join();
  t2.join();

  EXPECT(val == 2);
}

CASE("multithreaded - extended")
{
  Spinlock s;
  volatile int val = 0;
  s.lock();

  std::barrier barrier_start{ 3 };

  std::thread t1([&](void)->void {
      barrier_start.arrive_and_wait();
      for (int i = 0; i < 100000; i++) {
        s.lock();
        val++;
        s.unlock();
      }
  });

  std::thread t2([&](void)->void {
      barrier_start.arrive_and_wait();
      for (int i = 0; i < 100000; i++) {
        s.lock();
        val++;
        s.unlock();
      }
  });

  std::thread t3([&](void)->void {
      barrier_start.arrive_and_wait();
      for (int i = 0; i < 100000; i++) {
        s.lock();
        val++;
        s.unlock();
      }
  });

  EXPECT(val == 0);

  s.unlock();

  t1.join();
  t2.join();
  t3.join();

  EXPECT(val == 300000);
}
