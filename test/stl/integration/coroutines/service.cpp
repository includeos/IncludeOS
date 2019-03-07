// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
   A very superficial test to verify that basic STL is working
   This is useful when we mess with / replace STL implementations

**/
#define SMP_DEBUG 1

#include <os>
#include <iostream>
#include <experimental/coroutine>
#include <smp>
#include <vector>

using namespace std;
template<typename T>
struct smp_future {
  struct promise_type;
  using handle_type = std::experimental::coroutine_handle<promise_type>;
  handle_type coro;

#ifdef INCLUDEOS_SMP_ENABLE
  std::atomic<int> done {false};
#else
  int done {false};
#endif

  smp_future(const smp_future &s) = delete;
  smp_future(smp_future&& s)
    : coro(s.coro) {
    //std::cout << "smp_future moving " << std::endl;
    s.coro = nullptr;
  }

  smp_future& operator=(const smp_future&) = delete;
  smp_future& operator=(smp_future&& s)
  {
    coro = s.coro;
    s.coro = nullptr;
    return *this;
  }

  smp_future(handle_type h)
    : coro(h) {


    //std::cout << "smp_future created" << std::endl;
  }

  ~smp_future() {
    //std::cout << "Smp_Future deleted" << std::endl;
    if (coro) coro.destroy();
  }

  T get() {
    coro.resume();
    return coro.promise().value;
  }

  bool await_ready() const {
    const auto ready = coro.done();
    CPULOG("await_ready: result %s \n", ready ? "is ready" : "isn't ready");
    return coro.done();
  }

  void await_suspend(std::experimental::coroutine_handle<> awaiting) {
    CPULOG("await_suspend: spinwaiting for coro \n");
    while(!done);
    CPULOG("await_suspend: spinwaiting done, resuming awaiting coro \n");
    awaiting.resume();

  }

  auto await_resume() {
    const auto r = coro.promise().value;
    CPULOG("await_resume: value is returned, returning %i \n", r);
    return r;
  }


  struct promise_type {
    //std::shared_ptr<T> ptr;
    T value{};


    promise_type() {
      //std::cout << "Promise created" << std::endl;
    }
    ~promise_type() {
      //std::cout << "Promise died" << std::endl;
    }
    auto get_return_object() {
      //std::cout << "Send back a smp_future" << std::endl;
      return smp_future<T>{handle_type::from_promise(*this)};;
    }
    auto initial_suspend() {
      //std::cout << "Started the coroutine, don't stop now!" << std::endl;
      return std::experimental::suspend_always{};
    }
    auto return_value(T v) {
      //std::cout << "Got an answer of " << v << std::endl;
      value = v;
      return std::experimental::suspend_never{};
    }
    auto final_suspend() {
      //std::cout << "Finished the coro" << std::endl;
      return std::experimental::suspend_always{};
    }


    void unhandled_exception() {
      std::exit(1);
    }
  };
};


smp_future<int> answer(int i) {
  CPULOG("Computing answer for %i \n", i);
  co_return i * 2;
}

smp_future<int> reduce() {


  std::vector< decltype(answer(10)) > futures;

  futures.emplace_back(answer(2));
  futures.emplace_back(answer(4));
  futures.emplace_back(answer(8));
  futures.emplace_back(answer(16));

  std::cout << "Created " << futures.size()
            << " coroutines, let's get a values" << std::endl;

  int cpu = 1;
  for (auto& i : futures) {

#ifdef INCLUDEOS_SMP_ENABLE
    SMP::add_task([&i]() {
        CPULOG("Resuming coroutine \n");
        i.coro();
        CPULOG("Coroutine done. \n");
        i.done.store(true);
      }, []{}, cpu);
#else
    i.coro();
    i.done = true;
#endif
    cpu++;
  }
  SMP::signal();

  CPULOG("Created %li coroutines \n", futures.size());
  asm("pause");

  int sum = 0;
  for  (auto&& f : futures){
      auto v = co_await f;
      CPULOG("Coroutine value is: %i \n", v);
      //std::cout << "Coroutine value  is: " << v << std::endl;
      sum += v;
  }

  std::cout << "All done" << std::endl;
  co_return sum;
}

smp_future<int> reduce1 () {
  co_return co_await answer(1);
}

void Service::start(const std::string&)
{

  std::cout << "Starting coroutines" << std::endl;
  auto sum = reduce().get();
  std::cout << "Sum of coroutine results: " << sum << std::endl;
  Expects(sum == 60);
  exit(0);
}
