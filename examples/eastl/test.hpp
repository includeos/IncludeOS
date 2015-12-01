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

#pragma once

#include <string>
#include <vector>

class TestItem
{
public:
  TestItem(
      const std::string& name, 
      int stage, 
      int stages, 
      const std::string& message = "")
  {
    this->name   = name;
    this->stage  = stage;
    this->stages = stages;
    this->message = message;
  }
  
  bool passed() const
  {
    return stages == stage;
  }
  std::string report() const
  {
    return "[" + name + "] " +
      (passed() ? "passed" : "failed") + " "
      "stage " + std::to_string(stage) + " / " +
      std::to_string(stages);
  }
  std::string getName() const
  {
    return name;
  }
  std::string getMessage() const
  {
    return message;
  }
  int getStage() const
  {
    return stage;
  }
  int getNoStages() const
  {
    return stages;
  }
  
private:
  std::string name;
  int stage;
  int stages;
  std::string message;
};

class TestResult
{
public:
  typedef std::vector<TestItem> resv_t;
  
  template <typename... Args>
  void emplace_back(Args&&... args)
  {
    items.emplace_back(args...);
  }
  
  resv_t::const_iterator begin() const
  {
    return items.begin();
  }
  resv_t::const_iterator end() const
  {
    return items.end();
  }
  
private:
  resv_t items;
};
