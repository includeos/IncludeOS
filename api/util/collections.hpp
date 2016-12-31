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

#pragma once
#ifndef UTIL_COLLECTIONS_HPP
#define UTIL_COLLECTIONS_HPP

#include <algorithm>
#include <deque>
#include <iosfwd>
#include <list>
#include <map>
#include <numeric>
#include <sstream>
#include <type_traits>
#include <vector>

namespace collections {

///
/// Internal namespace consisting of collection traits
///
inline namespace traits {

  template<typename>
  struct is_sequence
  : public std::false_type {};

  template<typename T>
  struct is_sequence<std::deque<T>>
  : public std::true_type {};

  template<typename T>
  struct is_sequence<std::list<T>>
  : public std::true_type {};

  template<typename T>
  struct is_sequence<std::vector<T>>
  : public std::true_type {};

  template<typename>
  struct has_random_iterator
  : public std::false_type {};

  template<typename T>
  struct has_random_iterator<std::deque<T>>
  : public std::true_type {};

  template<typename T>
  struct has_random_iterator<std::vector<T>>
  : public std::true_type {};

} //< namespace traits

///
/// Internal namespace consisting of operation tag dispatchers
///
inline namespace operation_tags {

  constexpr struct is_heap_tag {
    constexpr is_heap_tag() noexcept {};
  } is_heap;

  constexpr struct is_sorted_tag {
    constexpr is_sorted_tag() noexcept {};
  } is_sorted;

  constexpr struct make_heap_tag {
    constexpr make_heap_tag() noexcept {};
  } make_heap;

  constexpr struct max_element_tag {
    constexpr max_element_tag() noexcept {};
  } max_element;

  constexpr struct min_element_tag {
    constexpr min_element_tag() noexcept {};
  } min_element;

  constexpr struct next_permutation_tag {
    constexpr next_permutation_tag() noexcept {};
  } next_permutation;

  constexpr struct prev_permutation_tag {
    constexpr prev_permutation_tag() noexcept {};
  } prev_permutation;

  constexpr struct sort_tag {
    constexpr sort_tag() noexcept {};
  } sort;

  constexpr struct stable_sort_tag {
    constexpr stable_sort_tag() noexcept {};
  } stable_sort;

  constexpr struct sum_tag {
    constexpr sum_tag() noexcept {};
  } sum;

  constexpr struct unique_tag {
    constexpr unique_tag() noexcept {};
  } unique;

} //< namespace operation_tags

///
/// Operator to check if the specified sequence is heap
///
/// @param data_set
///   The specified sequence to check if it's a heap
///
/// @return true if it's heap, false otherwise
///
template<typename T,
         typename = std::enable_if_t<
                    has_random_iterator<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
bool operator|(T&& data_set, const struct is_heap_tag&) {
  return std::is_heap(std::begin(data_set), std::end(data_set));
}

///
/// Operator to check if the specified sequence is sorted
///
/// @param data_set
///   The specified sequence to check if sorted
///
/// @return true if sorted, false otherwise
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
bool operator|(T&& data_set, const struct is_sorted_tag&) {
  return std::is_sorted(std::begin(data_set), std::end(data_set));
}

///
/// Operator to transform the specified sequence into a heap
///
/// @param data_set
///   The specified sequence to transform into a heap
///
/// @return A new sequence representing a heap
///
template<typename T,
         typename = std::enable_if_t<
                    has_random_iterator<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct make_heap_tag&) {
  auto result = std::forward<T>(data_set);
  std::make_heap(std::begin(result), std::end(result));
  return result;
}

///
/// Operator to get the max element from the specified sequence
///
/// @param data_set
///   The specified sequence to get the max element from
///
/// @return The max element from the specified sequence
///
/// @note If the specified sequence is empty a default constructed
///   value will be returned
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct max_element_tag&) {
  using sequence = std::remove_reference_t<T>;
  const auto it  = std::max_element(std::begin(data_set), std::end(data_set));
  return (it not_eq std::end(data_set) ? (*it) : typename sequence::value_type{});
}

///
/// Operator to get the min element from the specified sequence
///
/// @param data_set
///   The specified sequence to get the min element from
///
/// @return The min element from the specified sequence
///
/// @note If the specified sequence is empty a default constructed
///   value will be returned
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct min_element_tag&) {
  using sequence = std::remove_reference_t<T>;
  const auto it  = std::min_element(std::begin(data_set), std::end(data_set));
  return (it not_eq std::end(data_set) ? (*it) : typename sequence::value_type{});
}

///
/// Operator to transform the specified sequence into the next permutation
///
/// @param data_set
///   The specified sequence to transform
///
/// @return A new sequence representing the next permutation
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct next_permutation_tag&) {
  auto result = std::forward<T>(data_set);
  std::next_permutation(std::begin(result), std::end(result));
  return result;
}

///
/// Operator to transform the specified sequence into the previous permutation
///
/// @param data_set
///   The specified sequence to transform
///
/// @return A new sequence representing the previous permutation
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct prev_permutation_tag&) {
  auto result = std::forward<T>(data_set);
  std::prev_permutation(std::begin(result), std::end(result));
  return result;
}

///
/// Operator to sort the contents of the specified sequence
///
/// @param data_set
///   The sequence to sort
///
/// @return A new sorted sequence
///
template<typename T,
         typename = std::enable_if_t<
                    has_random_iterator<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct sort_tag&) {
  auto result = std::forward<T>(data_set);
  std::sort(std::begin(result), std::end(result));
  return result;
}

///
/// Operator to sort the contents of the specified sequence
///
/// This version preserves the order of equal elements
///
/// @param data_set
///   The sequence to sort
///
/// @return A new sorted sequence
///
template<typename T,
         typename = std::enable_if_t<
                    has_random_iterator<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct stable_sort_tag&) {
  auto result = std::forward<T>(data_set);
  std::stable_sort(std::begin(result), std::end(result));
  return result;
}

///
/// Operator to remove duplicated elements in the specified sequence
///
/// @param data_set
///   The sequence to remove duplicated elements from
///
/// @return A new sequence with unique elements
///
template<typename T,
         typename = std::enable_if_t<
                    has_random_iterator<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct unique_tag&) {
  auto result  = (std::forward<T>(data_set) | stable_sort);
  const auto _ = std::unique(std::begin(result), std::end(result));
  result.erase(_, result.cend());
  return result;
}

///
/// Operator to sum the contents in the specified sequence
///
/// @param data_set
///   The sequence to sum the contents of
///
/// @return The sum of the contents in the specified sequence
///
/// @note The elements in the specified sequence must support
/// the arithmetic operator `+`
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
auto operator|(T&& data_set, const struct sum_tag&) {
  using sequence = std::remove_reference_t<T>;
  return std::accumulate(std::begin(data_set), std::end(data_set), typename sequence::value_type{});
}

///
/// Extract the keys from a std::map
///
/// @param map
///   The std::map to extract the keys from
///
/// @param reverse_order
///   If true keys are retrieved in reverse order
///
/// @return A std::vector containing all the keys from the
/// specified std::map
///
template<typename K, typename V, typename C, typename A>
auto keys(const std::map<K,V,C,A>& map, const bool reverse_order = false) {
  std::vector<K> keys;
  keys.reserve(map.size());
  for (const auto& _ : map) keys.push_back(_.first);
  if (reverse_order) std::reverse(keys.begin(), keys.end());
  return keys;
}

///
/// Extract the values from a std::map
///
/// @param map
///   The std::map to extract the values from
///
/// @param reverse_order
///   If true values are retrieved in reverse order
///
/// @return A std::vector containing all the values from the
/// specified std::map
///
template<typename K, typename V, typename C, typename A>
auto values(const std::map<K,V,C,A>& map, const bool reverse_order = false) {
  std::vector<V> values;
  values.reserve(map.size());
  for (const auto& _ : map) values.push_back(_.second);
  if (reverse_order) std::reverse(values.begin(), values.end());
  return values;
}

///
/// Make a std::map from two std::vector objects
///
/// @param keys
///   The std::vector object containing the keys
///
/// @param values
///   The std::vector object containing the values
///
/// @return A new std::map populated from the specified std::vector objects
///
template<typename T, typename A1, typename U, typename A2>
auto make_map(const std::vector<T, A1>& keys, const std::vector<U, A2>& values) {
  std::map<T, U> map;
  for (std::size_t e = std::min(keys.size(), values.size()), i = 0U; i < e; ++i) {
    map.emplace(keys[i], values[i]);
  }
  return map;
}

///
/// Swap the mappings contained in the specified std::map where the
/// keys will become the values and the values will become the keys
/// in the resulting std::map
///
/// @param map
///   The std::map to swap it mappings
///
/// @return A new std::map with the swapped mappings
///
template<typename K, typename V, typename C, typename A>
auto swap_mapping(const std::map<K,V,C,A>& map) {
  const auto keys   = collections::keys(map);
  const auto values = collections::values(map);
  return make_map(values, keys);
}

///
/// Add the specified argument to the std::vector object
///
/// @param data_set
///   The std::vector object to add the argument to
///
/// @param value
///   The value to add to the specified std::vector object
///
/// @note The value will only be added if compatible with the
/// the type stored in the std::vector object
///
template<typename T, typename A, typename V>
auto& add_all(std::vector<T,A>& data_set, V&& value) {
  if (std::is_assignable<T&,V>::value) {
    data_set.push_back(std::forward<V>(value));
  }
  return data_set;
}

///
/// Add the specified arguments to the std::vector object
///
/// @param data_set
///   The std::vector object to add the argument to
///
/// @param value
///   The current value to add to the specified std::vector object
///
/// @param args
///    A pack of arguments to add to the specified std::vector object
///
/// @note The values will only be added if compatible with the
/// the type stored in the std::vector object
///
template<typename T, typename A, typename V, typename... Args>
auto& add_all(std::vector<T,A>& data_set, V&& value, Args&&... args) {
  if (std::is_assignable<T&,V>::value) {
    data_set.push_back(std::forward<V>(value));
  }
  return add_all(data_set, std::forward<Args>(args)...);
}

///
/// Map the specified sequence of elements [a] to a sequence
/// of elements [b]
///
/// @param data_set
///   The specified sequence of elements to perform mapping over
///
/// @param map_function
///   A callable object that take an element {a} from the
///   specified sequence and return type {b}
///
/// @return A new sequence of elements of type {b}
///
template<typename T, typename F>
auto map(const T& data_set, F map_function) {
  std::vector<typename std::result_of<F(typename T::value_type)>::type> result;
  result.reserve(std::distance(std::begin(data_set), std::end(data_set)));
  for (const auto& element : data_set) {
    result.push_back(map_function(element));
  }
  return result;
}

///
/// Filter the specified sequence of elements
///
/// @param data_set
///   The specified sequence of elements to filter
///
/// @param filter_function
///   A callable object to filter the elements of the specified
///   sequence
///
/// @return A new sequence of purified elements
///
template<typename T, typename F,
         typename = typename std::enable_if<
                    std::is_same<typename std::result_of<F(typename T::value_type)>::type,
                    bool>::value>::type
        >
auto filter(const T& data_set, F filter_function) {
  std::vector<typename T::value_type> result;
  result.reserve(std::distance(std::begin(data_set), std::end(data_set)));
  for (const auto& element : data_set) {
    if (filter_function(element)) result.push_back(element);
  }
  result.shrink_to_fit();
  return result;
}

///
/// Print the contents of the specified sequence to the specified
/// output stream device
///
/// @param output_device
///   Output device to stream the contents to
///
/// @param data_set
///   A sequence object containing the contents to stream
///
/// @param new_line
///   If true print a new line after the end_symbol
///
/// @param start_symbol
///   The starting symbol to enclose the contents of the specified sequence
///
/// @param end_symbol
///   The ending symbol to enclose the contents of the specified sequence
///
/// @param delimiter
///   The symbol to delimit the contents of the specified sequence
///
template<typename T>
void print_sequence(std::ostream& output_device, T& data_set,
                    const bool new_line = false, const char start_symbol = '[',
                    const char end_symbol = ']', const char delimiter = ',')
{
  auto i = std::begin(data_set);
  const auto e = --std::end(data_set);
  output_device << start_symbol;
  while (i not_eq e) {
    output_device << (*i++) << delimiter;
  }
  output_device << (*i) << end_symbol;
  if (new_line) {
    output_device << '\n';
  }
}

///
/// Get the contents of the specified sequence as a std::string
///
/// @param data_set
///   A sequence object to transform into a std::string
///
/// @param start_symbol
///   The starting symbol to enclose the contents of the specified sequence
///
/// @param end_symbol
///   The ending symbol to enclose the contents of the specified sequence
///
/// @param delimiter
///   The symbol to delimit the contents of the specified sequence
///
template<typename T,
         typename = std::enable_if_t<
                    is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
std::string sequence_to_string(const T& data_set, const char start_symbol = '[',
                               const char end_symbol = ']', const char delimiter = ',')
{
  std::ostringstream sequence_string;
  print_sequence(sequence_string, data_set, false, start_symbol, end_symbol, delimiter);
  return sequence_string.str();
}

///
/// Print the contents of the specified std::map to the specified
/// output stream device
///
/// @param output_device
///   Output device to stream the contents to
///
/// @param data_set
///   A std::map object containing the contents to stream
///
/// @param new_line
///   If true print a new line after the end_symbol
///
/// @param start_symbol
///   The starting symbol to enclose the contents of the specified std::map
///
/// @param end_symbol
///   The ending symbol to enclose the contents of the specified std::map
///
/// @param delimiter
///   The symbol to delimit the contents of the specified std::map
///
template<typename K, typename V, typename C, typename A>
void print_map(std::ostream& output_device, const std::map<K,V,C,A>& data_set,
               const bool new_line = false, const char start_symbol = '{',
               const char end_symbol = '}', const char delimiter = ',')
{
  auto i = data_set.cbegin();
  const auto e = --data_set.cend();
  output_device << start_symbol;
  while (i not_eq e) {
    output_device << (i->first) << ": " << (i->second) << delimiter << ' ';
    ++i;
  }
  output_device << (i->first) << ": " << (i->second) << end_symbol;
  if (new_line) {
    output_device << '\n';
  }
}

} //< namespace collections

///
/// Operator to stream the contents of the specified sequence to the specified
/// output stream device
///
/// @param output_device
///   Output device to stream the contents to
///
/// @param data_set
///   A sequence object containing the contents to stream
///
/// @return The specified output stream device
///
template<typename T,
         typename = std::enable_if_t<
                    collections::is_sequence<
                    std::remove_cv_t<std::remove_reference_t<T>>>::value>>
std::ostream& operator<<(std::ostream& output_device, const T& data_set) {
  collections::print_sequence(output_device, data_set);
  return output_device;
}

///
/// Operator to stream the contents of the specified std::map to the specified
/// output stream device
///
/// @param output_device
///   Output device to stream the contents to
///
/// @param data_set
///   A std::map object containing the contents to stream
///
/// @return The specified output stream device
///
template<typename K, typename V, typename C, typename A>
std::ostream& operator<<(std::ostream& output_device, const std::map<K,V,C,A>& data_set) {
  collections::print_map(output_device, data_set);
  return output_device;
}

#endif //< UTIL_COLLECTIONS_HPP
