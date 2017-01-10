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

#include <iostream>
#include <os>

#include "authors.hpp"
#include "warehouse.hpp"

int main()
{
  //-----------------------------------------------------------------
  // Author List Example
  //
  // Demonstrate printing an author list in alphabetical order
  //
  // Behind the scene, a mapping takes place to determine the width
  // of the fields for proper formatting (authors.hpp)
  //-----------------------------------------------------------------
  Author_list list;

  list.add("Alfred", "alfred@includeos.com", "Norway")
      .add("Alf", "alf@includeos.com", "Norway")
      .add("Andreas", "andreas@includeos.com", "Norway")
      .add("AnnikaH", "annikah@includeos.com", "Norway")
      .add("Ingve", "ingve@includeos.com", "Norway")
      .add("Martin", "martin@includeos.com", "Norway")
      .add("Rico", "rico@includeos.com", "Trinidad");

  std::cout << "Author Listing:\n\n"
            << list << '\n';

  //-----------------------------------------------------------------
  // Warehouse Example
  //
  // Behind the scene, a filter-map-reduce takes place to achieve the
  // result (warehouse.hpp)
  //-----------------------------------------------------------------
  std::cout << "JSON Document [weight of all the red widgets from warehouse in Norway]:\n"
            << ("{\"WW\": " + std::to_string(get_weight(Warehouse::NORWAY,
                                             [](const auto& w){return w.colour() == Colour::RED;})) + "}")
            << '\n';
}
