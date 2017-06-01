// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <util/collections.hpp>

const std::map<int,int> mappings {
  {1,2},
  {3,4},
  {5,6},
  {7,8},
  {9,10}
};

CASE("Extract keys from a std::map") {
  const std::vector<int> test_vector {1,3,5,7,9};
  const auto keys = collections::keys(mappings);
  EXPECT(test_vector == keys);
}

CASE("Extract keys from a std::map in reverse order") {
  const std::vector<int> test_vector {9,7,5,3,1};
  const auto keys = collections::keys(mappings, true);
  EXPECT(test_vector == keys);
}

CASE("Extract values from a std::map") {
  const std::vector<int> test_vector {2,4,6,8,10};
  const auto values = collections::values(mappings);
  EXPECT(test_vector == values);
}

CASE("Extract values from a std::map in reverse order") {
  const std::vector<int> test_vector {10,8,6,4,2};
  const auto values = collections::values(mappings, true);
  EXPECT(test_vector == values);
}

CASE("Make a std::map from two std::vector objects") {
  const std::vector<int> keys   {1,3,5,7,9};
  const std::vector<int> values {2,4,6,8,10};
  const auto test_map = collections::make_map(keys, values);
  EXPECT(test_map == mappings);
}

CASE("Add a set of values to a std::vector object") {
  const std::vector<int> test_vector {1,3,5,12,15};
  std::vector<int> data {1,3,5};
  collections::add_all(data, 12.5, 15);
  EXPECT(test_vector == data);
}

CASE("Create a std::vector object (make_vector)") {
  const std::vector<double> test_vector {12.5,4.2,5};
  const auto data = collections::make_vector(12.5,4.2,5);
  EXPECT(test_vector == data);
}

CASE("Get the min element from a std::vector object") {
  std::vector<int> data {1,5,3};
  const auto min = (data | collections::min_element);
  EXPECT(1 == min);
}

CASE("Get the max element from a std::vector object") {
  std::vector<int> data {1,5,3};
  const auto max = (data | collections::max_element);
  EXPECT(5 == max);
}

CASE("Filter a std::vector object") {
  const std::vector<std::string> test_vector {"Alfred"};
  std::vector<std::string> names {"Alfred","Ingve","Martin","Rico"};
  const auto CEO = collections::filter(names,[](const std::string& n)
  { return (not n.empty()) ? n[0] == 'A': false; });
  EXPECT(test_vector == CEO);
}

CASE("Convert a std::vector object to a std::string") {
  // To be a little fancy we'll:
  //
  // 1. Filter out all the even numbers
  // 2. Map the new sequence to a vector of vectors
  // 3. Convert the last vector to a std::string
  //
  std::vector<int> data {1,2,3,4,5,6,7,8,9};

  const auto information = collections::sequence_to_string(
                             collections::map(
                               collections::filter(data,[](const int n){ return (n % 2) == 0; }),
                                 [](const int n){ return std::vector<int>(n, n); }).back());

  const std::string test_string {"[8,8,8,8,8,8,8,8]"};

  EXPECT(test_string == information);
}

CASE("Map a sequence of elements ([std::string] -> [std::string::size_type])") {
  std::vector<std::string> names {"Rico", "Antonio", "Felix"};
  const auto name_lengths = collections::map(names, [](const auto& e)
  { return e.length(); });
  decltype(name_lengths) test_vector {4,7,5};
  EXPECT(test_vector == name_lengths);
}

CASE("Sort the elements in a std::vector object") {
  const std::vector<int> test_vector {5,12,19,22,22};
  std::vector<int> data {22, 5, 19, 12, 22};
  EXPECT(test_vector == (data | collections::sort));
}

CASE("Sort the elements in a std::vector object (stable_sort)") {
  const std::vector<int> test_vector {5,12,19,22,22};
  std::vector<int> data {22, 5, 19, 12, 22};
  EXPECT(test_vector == (data | collections::stable_sort));
}

CASE("Leave a std::vector object with unique elements") {
  const std::vector<int> test_vector {5,12,19,22};
  std::vector<int> data {22, 5, 19, 12, 22};
  EXPECT(test_vector == (data | collections::unique));
}

CASE("Check if a std::vector object is sorted") {
  std::vector<int> data {22, 5, 19, 12, 22};
  EXPECT(false == (data | collections::is_sorted));
  EXPECT(true  == (data | collections::sort | collections::is_sorted));
}

CASE("Get the sum of the contents in a std::vector object") {
  std::vector<int> data {22, 5, 19, 12, 22};

  const auto value = data
                   | collections::unique
                   | collections::sum;

  EXPECT(58 == value);
}

CASE("Swap the mappings in a std::map") {
  const std::map<int,int> test_map {
    {2,1},
    {4,3},
    {6,5},
    {8,7},
    {10,9}
  };

  EXPECT(test_map == collections::swap_mapping(mappings));
}
