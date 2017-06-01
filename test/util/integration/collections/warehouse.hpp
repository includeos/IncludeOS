// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef WAREHOUSE_HPP
#define WAREHOUSE_HPP

#include <cstdint>
#include <util/collections.hpp>

///
/// Representation of warehouse locations
///
enum class Warehouse : uint8_t {
  NORWAY,
  GREECE,
  USA,
  CHINA,
  JAPAN,
  CANADA
};

///
/// Representation of widget colours
///
enum class Colour : uint8_t {
  WHITE,
  BLACK,
  GREEN,
  BROWN,
  RED,
  ORANGE,
};

///
/// This class is used to store information about each widget
/// stored in a warehouse
///
class Widget {
public:
  ///
  /// Constructor
  ///
  /// @param name
  ///   The name of the widget
  ///
  /// @param weight
  ///   The weight of the widget
  ///
  /// @param
  ///   The colour of the widget
  ///
  Widget(std::string name, const double weight, const Colour colour)
    : name_{std::move(name)}
    , weight_{weight}
    , colour_{colour}
  {}

  ///
  /// Get the name of the widget
  ///
  /// @return The name of the widget
  ///
  const std::string& name() const noexcept
  { return name_; }

  ///
  /// Get the weight of the widget
  ///
  /// @return The weight of the widget
  ///
  double weight() const noexcept
  { return weight_; }

  ///
  /// Get the colour of the widget
  ///
  /// @return The colour of the widget
  ///
  Colour colour() const noexcept
  { return colour_; }

private:
  std::string name_;
  double      weight_ {};
  Colour      colour_ {};
}; //< class Widget

///
/// Representation of an inventory database
///
static std::map<Warehouse, std::vector<Widget>> inventory_database
{
  {Warehouse::NORWAY,{{"w1",3.8,Colour::WHITE},{"w4",8.2,Colour::RED},{"w5",5.5,Colour::BROWN},{"w4",8.2,Colour::RED}}},
  {Warehouse::GREECE,{{"w2",1.7,Colour::BLACK},{"w6",8.3,Colour::ORANGE},{"w5",5.5,Colour::BROWN}}},
  {Warehouse::USA,{{"w3",4.4,Colour::GREEN},{"w4",8.2,Colour::RED},{"w5",5.5,Colour::BROWN}}},
  {Warehouse::CHINA,{{"w4",8.2,Colour::RED},{"w2",1.7,Colour::BLACK},{"w6",8.3,Colour::ORANGE}}},
  {Warehouse::JAPAN,{{"w5",5.5,Colour::BROWN},{"w6",8.3,Colour::ORANGE},{"w5",5.5,Colour::BROWN}}},
  {Warehouse::CANADA,{{"w6",8.3,Colour::ORANGE},{"w4",8.2,Colour::RED},{"w1",3.8,Colour::WHITE}}},
};

///
/// Function used as an API endpoint to get the total weight
/// for the widgets in a particular warehouse
///
/// @param warehouse
///   Specification of which warehouse to query
///
/// @param predicate
///   A callable object used to determine which widget to get the
///   the weight of
///
/// @return The total weight of all the widgets based on the predicate
///
template<typename P>
auto get_weight(const Warehouse warehouse, P&& predicate) {
  using namespace collections;
  return map(
            filter(inventory_database[warehouse],
                  [p = std::forward<P>(predicate)](const auto& w){return p(w);}),
            [](const auto& w){return w.weight();}) | sum;
}

#endif //< WAREHOUSE_HPP
