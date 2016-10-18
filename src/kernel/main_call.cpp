// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <regex>


__attribute__((weak))
extern "C" int main(int , const char** );

__attribute__((weak))
void Service::start(const std::string& st) {

  printf("Service arg: %s \n", st.c_str());

  // We'll mangle this copy
  std::string s{st};

  std::regex words_regex("[^\\s]+");
  auto words_begin = std::sregex_iterator(s.begin(), s.end(), words_regex);
  auto words_end = std::sregex_iterator();

  // There is at least one arg (binary name)
  int argc = std::max(std::distance(words_begin, words_end), 1);
  const char* args[argc];

  // Populate argv
  int i = 0;
  for (auto word = words_begin; word != words_end; word++) {
    std::cout << "Stub arg " << i << " " << std::smatch(*word).str() << "\n";
    args[i++] = s.data() + word->position();

  }

  // Zero-terminate all words
  for (auto cit = s.begin(); cit != s.end(); cit++) {
    auto next = cit + 1;

    if (next == s.end())
      break;

    if (not std::isspace(*cit) and std::isspace(*next))
      *next = 0;
  }

  int exit_status = main(argc, args);
  INFO("main","returned with status %i", exit_status);
  //exit(exit_status);
}
