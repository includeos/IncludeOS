// #define SMP_DEBUG 1
#include <os>
#include <deque>
#include <fiber>
#include <smp>
#include <mutex>


inline void* get_rsp() {
  void* stack = 0;
#if defined(ARCH_x86_64)
  asm volatile ("mov %%rsp, %0" :"=r"(stack));
#elif defined(ARCH_i686)
  asm volatile ("mov %%esp, %0" :"=r"(stack));
#endif
  return stack;
};

// CONFIG
const int jobs = 32;
const bool work_hard = false;

// test MANY
const int num_strings = jobs;
std::atomic<int> jobs_remaining{};
const int strsize = 80;

std::array<std::array<char, strsize>, num_strings > src{};
std::array<std::array<char, strsize>, num_strings > dst{};


template <typename Q>
class Sync_queue {

public:
  using El = typename Q::value_type;

  Sync_queue() = default;
  ~Sync_queue() = default;

  Sync_queue(std::initializer_list<Fiber> fibers)
    : queue{fibers} {}

  El sync_get_next()
  {

    std::lock_guard<std::mutex> lock(queue_mutex);
    if (queue.empty())
      return nullptr;

    El f = std::move(queue.front());
    queue.pop_front();
    return std::move(f);
  }


  void sync_insert(El f)
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    queue.push_back(std::move(f));
  }

  auto size()
  {
    return queue.size();
  }


private:

  Q queue {};
  std::mutex queue_mutex;


};


Sync_queue<std::deque<std::unique_ptr<Fiber>>> smp_fibers;
std::array<int, 32> runners_per_cpu;
int worker_offset = 0;

// Copy a single byte then yield
void worker(int i)
{

  CPULOG("[ F%i::worker %i ] >> Starting <<  main F%i\n",
            Fiber::current()->id(), i, Fiber::main()->id());
  int pos = 0;
  for (auto& c : src[i]) {

    // Make it search for the char to make multicore worth it
    int tmp = c;

    if (work_hard) {
      volatile int tmp = -10000000;
      for (; tmp != c; tmp++);
    }

    dst[i][pos++] = tmp;

    Expects(Fiber::current());
    Expects(Fiber::current()->parent());
    Expects(Fiber::current() != Fiber::main());
    CPULOG("[ F%i::worker %i] @ pos %i yielding to F%i\n",
           Fiber::current()->id(), i, pos, Fiber::current()->parent()->id());

    Expects(Fiber::current() != Fiber::main());
    Expects(Fiber::main());
    Expects(Fiber::main()->id() == runners_per_cpu[SMP::cpu_id()]);
    Expects(Fiber::current()->id() == i + worker_offset);

    Fiber::yield();

    Expects(Fiber::current());
    Expects(Fiber::current()->parent());
    Expects(Fiber::current() != Fiber::main());
    Expects(Fiber::main());
    Expects(Fiber::main()->id() == runners_per_cpu[SMP::cpu_id()]);
    Expects(Fiber::current()->id() == i + worker_offset);

  }

  jobs_remaining--;
  CPULOG("[ F%i::worker %i ] DONE returning to F%i, main F%i\n",
         i, Fiber::current()->id(), Fiber::current()->parent()->id(), Fiber::main()->id());

}


void verify_work_done()
{
  int i{0};
  int pos{0};

  printf("Verifying results \n");
  for (auto& str : src) {
    printf("Src%i: ", i);
    for (auto& chr : str) {
      printf("%c", chr);
      Expects(dst[i][pos++] == chr);
    }

    printf("\nDst%i: ", i);
    for (auto& chr : dst[i]) {
      printf("%c", chr);
    }
    printf("\n");
    pos = 0;
    i++;
  }
}


void verify_work_not_done()
{
  int i{0};
  int pos{0};

  printf("Verifying results \n");
  for (auto& str : src) {
    printf("Src%i: ", i);
    for (auto& chr : str) {
      printf("%c", chr);
      Expects(dst[i][pos++] != chr);
    }

    printf("\nDst%i: ", i);
    for (auto& chr : dst[i]) {
      printf("%c", chr);
    }
    printf("\n");
    pos = 0;
    i++;
  }


}

void threadrunner (int i){

  int last_id = -1;

  while (jobs_remaining)
    {
      asm("pause");
      Expects(smp_fibers.size() <= num_strings);

      Expects(i == SMP::cpu_id());
      Expects(Fiber::current());
      Expects(Fiber::current() == Fiber::main());
      Expects(Fiber::current()->id() == runners_per_cpu[SMP::cpu_id()]);


      auto fiber = smp_fibers.sync_get_next();

      if (not fiber) {
        asm("pause");
        continue;
      }

      if (fiber->done()) {
        CPULOG("[R%i ] !!! ERROR !!! got DONE FIBER: F%i. Current fiber id: %i \n",
               Fiber::main()->id(), fiber->id(), Fiber::last_id());
        abort();
      }

      last_id = fiber->id();


      if (not fiber->started()) {
        CPULOG("[ R%i ] >>>> F%i FIRST RUN <<< \n",
               Fiber::main()->id(), fiber->id());

        fiber->start();
        Expects(fiber->id() == last_id);
      } else {
        CPULOG("[ R%i ] >>>> Resuming F%i  <<< \n",
                  Fiber::main()->id(), fiber->id());
        fiber->resume();
        Expects(fiber->id() == last_id);
      }

      Expects(fiber);
      CPULOG("[ R%i ] F%i yield. %i jobs left, %li fibers in queue \n",
                Fiber::main()->id(), fiber->id(), jobs_remaining.load(), smp_fibers.size());

      if (not fiber->done()) {
        smp_fibers.sync_insert(std::move(fiber));

        // Fiber is moved now, can't be touched
        CPULOG("[ R%i ] checked back F%i. Jobs left: %i, Fibers left: %li, _current: F%i \n",
               Fiber::main()->id(), last_id, jobs_remaining.load(), smp_fibers.size(),
               Fiber::current()->id());

      }else {
        CPULOG("[ R%i ] F%i is DONE, releasing. Jobs left: %i, Fibers left: %li, _current: F%i \n",
               Fiber::main()->id(), fiber->id(), jobs_remaining.load(), smp_fibers.size(),
               Fiber::current()->id());
        Expects(fiber);
      }

      Expects(Fiber::main());
      Expects(Fiber::current());

      if (Fiber::main()->id() != Fiber::current()->id()) {
        CPULOG("\n[ R%i ] Current: %i \n",
                  Fiber::main()->id(), Fiber::current()->id());
      }

      Expects(Fiber::main()->id() == Fiber::current()->id());
      Expects(i == SMP::cpu_id());
      Expects(Fiber::current()->id() == runners_per_cpu[SMP::cpu_id()]);

      asm("pause");

    };

  //CPULOG("[ R%i ] Done \n", Fiber::current()->id(), );
}

std::atomic<int> active_runners{0};


void many_fibers() {
  printf("\n============================================== \n");
  printf("         Many fibers over SMP \n");
  printf("============================================== \n");


  for (int i = 1; i < SMP::cpu_count(); ++i) {
    active_runners++;
    int cpu = SMP::active_cpus(i);
    SMP::add_task([cpu]{

        CPULOG("[ SMP::add_task %i ] starting \n", cpu);

        Fiber runner { threadrunner , cpu};
        auto runner_id_ = runner.id();
        runners_per_cpu[SMP::cpu_id()] = runner_id_;
        CPULOG("[ SMP::add_task %i ] NEW RUNNER: id %i, th_cnt: %i\n",
               cpu, runner.id(), active_runners.load());
        runner.start();
        Expects(runner.id() == runner_id_);
        active_runners--;
        /*
        asm("hlt");
        CPULOG("[ SMP::add_task %i ] runner id %i done. Runners left: %i\n",
        i, runner.id(), active_runners.load());*/
      }, cpu);

  }


  int string_idx = 0;
  worker_offset = Fiber::last_id();
  // Populate source strings and fiber pool
  for (auto& str : src) {
      for (auto& chr : str)
        chr = 'A' + rand() % ('Z' - 'A');

      smp_fibers.sync_insert(std::make_unique<Fiber>(worker, string_idx++));
      jobs_remaining++;
      CPULOG("Init F%i. Jobcount %i \n", Fiber::last_id(), jobs_remaining.load() );
  }

  verify_work_not_done();

  printf("Added %lu smp_fibers \n", smp_fibers.size());

  SMP::signal();

  CPULOG("Testing SMP. Found %i cores \n", SMP::cpu_count());
  CPULOG("BSP Threadcount: %i \n", active_runners.load());


  while(jobs_remaining or active_runners) {
    asm("pause");
  }

  CPULOG("All %i jobs done %i remaining. %i threads remain. \n",
            num_strings, jobs_remaining.load(), active_runners.load());

  verify_work_done();

  printf("\n%i strings copied bytewise OK using %i fibers over %i CPU's\n",
         num_strings, jobs, SMP::cpu_count());

}


const int jobs_pr_run = 10;
std::atomic<int> jobs_left;
std::atomic<int> runners_left;

void basic_runner(int id) {
  CPULOG("<runner %i> starting. Current: %i \n", id, Fiber::current()->id());
  Expects(id == SMP::cpu_id());


  for (int i = 0; i < jobs_pr_run; i++)
    jobs_left--;
  CPULOG("<runner %i> F%i DONE. rsp: %p \n",
         id, Fiber::main()->id(), get_rsp());

}

void fiber_on_other_cpus(){
  printf("\n============================================== \n");
  printf("          Fiber on other CPU's \n");
  printf("============================================== \n");

  auto jobs = SMP::cpu_count();

  jobs_left = jobs * jobs_pr_run;
  runners_left = jobs;

  for (int i = 0; i < SMP::cpu_count(); i++) {
    SMP::add_task([]{
        Fiber runner {basic_runner, SMP::cpu_id()};
        auto my_runner = runner.id();
        CPULOG("[ F%i ] START worker %i \n", runner.id(), SMP::cpu_id() );
        runner.start();

        Expects(my_runner == runner.id());

        CPULOG("[ F%i ] DONE runners left: %i iterations left: %i \n",
               runner.id(), runners_left.load(), jobs_left.load() - 1);
        Expects(Fiber::main() == nullptr);
        Expects(Fiber::current() == nullptr);

        // Decrement last as this is our barrier
        runners_left--;
      }, i);
  }

  SMP::signal();

  // Spin until all cores done
  while (jobs_left or runners_left)
    asm("pause");

  Expects(Fiber::current() == nullptr);
  Expects(Fiber::main() == nullptr);

  CPULOG("Basic test COMPLETE (%i iterations left) \n", jobs_left.load());
  asm("pause");

}

// TODO: Implement SMP::Spinlock
struct Mylock {
  int i = 0;
  void lock() {
    i = 1;
  }
  void unlock() {
    i = 0;
  }

  bool locked()
  { return i == 1; }

} L1;

void fiber_smp_test() {
  printf("\n============================================== \n");
  printf("            Fiber SMP test\n");
  printf("============================================== \n");


  {
    std::lock_guard<Mylock> lock(L1);
    fiber_on_other_cpus();
    many_fibers();
    //printf("Spinlock is %s \n", L1.locked() ? "LOCKED" : "OPEN");
  }

  //printf("Spinlock is %s \n", L1.locked() ? "LOCKED" : "OPEN");


}
