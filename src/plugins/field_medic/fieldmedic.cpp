// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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


#include "fieldmedic.hpp"
#include <os>

//#include <atomic>
static int diag_failures  = 0;
static int diag_successes = 0;
static bool fieldmedic_registered = false;

#define DIAGNOSE(TEST, TEXT, ...) \
try {  \
  if (! TEST) throw(std::runtime_error(TEXT));                          \
  printf("%16s[%s] " TEXT "\n", "", "+",  ##__VA_ARGS__);               \
  diag_successes++;                                                     \
} catch (const std::runtime_error& e) {                                 \
  diag_failures++;                                                      \
  printf("%16s[ ] " TEXT " failed: %s\n", "", e.what());                \
}                                                                       \

#define MYINFO(X,...) INFO("Field Medic","⛑️  " X,##__VA_ARGS__)

extern "C" char get_single_tbss();
namespace medic{

  void init(){
    using namespace diag;
    MYINFO("Checking vital signs");

    /* TODO:
       DIAGNOSE(timers(),     "Timers active");
       DIAGNOSE(elf(),        "ELF binary intact");
       DIAGNOSE(virtmem(),    "Virtual memory active");
       DIAGNOSE(heap(),       "Heap fragments intact");
    */

    init_tls();
    DIAGNOSE(tls(),        "Thread local storage intact, single CPU");
    DIAGNOSE(stack(),      "Stack check");
    DIAGNOSE(exceptions(), "Exceptions test");

    if (diag_failures == 0){
      MYINFO("OS initialization: All checks passed ✅");
    } else {
      MYINFO("OS initialization: %i / %i checks failed", diag_failures, (diag_failures + diag_successes));
    }
  }

  __attribute__ ((constructor))
  void register_medic() {
    os::register_plugin(medic::init, "Field medic");
    fieldmedic_registered = true;
  }


  namespace diag
  {
    extern thread_local medic::diag::Tl_bss_arr __tl_bss;
    extern thread_local medic::diag::Tl_data_arr __tl_data;

    /** Verify TLS data from a different translation unit */
    bool tls() {
      for (auto& c : __tl_bss) {
        if(c != '!')
          throw medic::diag::Error("unexpected .tbss value");
      }
      for (auto& i : __tl_data) {
        if (i != 42)
          return false;
      }
      return true;
    }
  }
}

extern "C" char get_single_tbss(){
  return medic::diag::__tl_bss[0];
}

extern "C" int get_single_tdata(){
  return medic::diag::__tl_data[0];
}


using namespace medic::diag;

void kernel::diag::post_service() noexcept {
  MYINFO("Service finished. Diagnosing.");
  DIAGNOSE(fieldmedic_registered && diag_successes > 0 && diag_failures == 0,
             "Field medic plugin active");
  DIAGNOSE(invariant_post_bss(), "Post .bss invariant still holds");
  DIAGNOSE(invariant_post_machine_init(), "Post machine init invariant still holds");
  DIAGNOSE(invariant_post_init_libc(), "Post init libc invariant still holds");

  if (diag_failures == 0){
    MYINFO("Diagnose complete. Healthy ✅");
  } else {
    MYINFO("Diagnose complete: %i / %i checks failed", diag_failures, (diag_failures + diag_successes));
  }
}
