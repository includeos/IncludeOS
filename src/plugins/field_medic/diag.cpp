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

#include <stdexcept>
#include <cstdlib>
#include <os>

#include "fieldmedic.hpp"

namespace medic {
namespace diag {

  thread_local std::array<char, diag::bufsize> __tl_bss;
  thread_local std::array<int, 256> __tl_data {
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42};


  static void verify_tls() {
    int correct = 0;
    for (auto& c : __tl_bss) {
      if (c == '!') correct++;
    }

    for (auto& i : __tl_data) {
      if (i == 42) correct++;
    }

    auto expected =  diag::bufsize + __tl_data.size();
    if (correct != expected)
      throw Error("TLS not initialized correctly");
  }

  static int stack_check(int N)
  {
    std::array<char, diag::bufsize> frame_arr;
    memset(frame_arr.data(), '!', diag::bufsize);

    static volatile int random1 = rand();
    if (N) {
      volatile int local = N;
      volatile auto res = stack_check(N - 1);
      Expects (res == random1 + N - 1);
    }

    verify_tls();

    for (auto &c : frame_arr) {
      Expects(c == '!');
    }

    return random1 + N;
  }

  template <typename Err>
  static volatile int throw_at(volatile int N) {
    std::array<char, diag::bufsize> frame_arr;
    memset(frame_arr.data(), '!', diag::bufsize);

    if (N) {
      volatile int i = throw_at<Err>(N - 1);
    }

    bool ok = true;
    verify_tls();

    for (auto &c : frame_arr) {
      if (c != '!') ok = false;
      Expects(c == '!');
    }

    if (ok)
      throw Err();

    return -1;
  }

  bool exceptions()
  {
    tls();

    try {
      throw_at<Error>(100);
    }
    catch (const Error& e)
    {
      tls();
      return true;
    }

    return false;
  }

  void init_tls(){
    memset(__tl_bss.data(), '!', diag::bufsize);
  }

  bool stack()
  {
    tls();

    auto check1 = stack_check(10);
    auto check2 = stack_check(100);
    return check1 == check2 - 90;
  }

}
}
