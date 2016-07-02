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

#ifndef BUCKET_BUCKET_HPP
#define BUCKET_BUCKET_HPP

#include <vector>
#include <functional>
#include <string>
#include <unordered_map>

#include "bucket_errors.hpp"

namespace bucket {

inline std::string to_string(const std::string& str)
{ return str; }

template <typename T>
struct Type { typedef T type; };

/**
 * @brief A (magical) bucket to store stuff inside
 * @details Stores an object together with a key.
 *
 * @tparam T What to put in the bucket
 *
 * @note Type {T} must contain a public member <key>
 *       of type {size_t} and must implement the
 *       json::Serializable interface. Also, type {T}
 *       cannot be declared as {const} since the member
 *       <key> is assigned to from within the Bucket.
 */
template <typename T>
class Bucket {
public:
  enum Constraint {
    NONE,
    UNIQUE,
    NOT_NULL
  };

private:
  using Key        = size_t;
  using Collection = std::unordered_map<Key, T>;
  using Content    = typename Collection::value_type;
  using IsEqual    = std::function<bool(const T&, const T&)>;

  using Column = std::string;

  template <typename Value>
  using Resolver = std::function<const Value&(const T&)>;

  template <typename Value>
  using Index = std::unordered_map<Value, Key>;

  template <typename Value>
  struct IndexedColumn {
    Resolver<Value> resolver;
    Index<Value> index;
    Constraint constraint; // currently only supports one
  };

  template <typename Value>
  using Indexes = std::unordered_map<Column, IndexedColumn<Value>>;

public:
  explicit Bucket(size_t limit = 1000);

  /**
   * @brief Capture something inside the bucket.
   * @details Add to the map and assign a unqiue key
   *
   * @param  Something to capture
   * @return The given ID to the captured something
   */
  Key capture(T&);

  /**
   * @brief Uses black magic to spawn a new something inside the bucket.
   * @details Call emplace on the underlying container, while also assigning a unique key.
   *
   * @param  Constructor args to something
   * @return The created something
   */
  template <typename... Args>
  T& spawn(Args&&...);

  /**
   * @brief Pick up the something with the given key inside the bucket.
   * @details Using key to find the object inside the container.
   * @throws Throws type {ObjectNotFound} if the object doesn't exist
   *
   * @param  The key to the something
   * @return (Hopefully) the something with the given key.
   */
  T& pick_up(const Key);

  /**
   * @brief Abandon/release the something with the given key
   * @details Calls earse on the underlying container
   *
   * @param  The key to the something.
   * @return Wheter something was abandoned or not.
   */
  bool abandon(const Key);

  template <typename Value>
  T& look_for(const Column&, const Value&);

  /**
   * @brief Lineup everything inside the bucket.
   * @details Builds a vector with only the values from the underlying container.
   * @return A vector with all the content inside the bucket.
   */
  std::vector<T> lineup() const;

  template <typename Value>
  void add_index(Column&& col, Resolver<Value> res, Constraint con = NONE);
  //{ add_index(Type<Value>(), std::forward<Column>(col), res, con); };

  template <typename Writer>
  void serialize(Writer& writer) const;

private:
  Key        idx_;
  Collection bucket_;
  const size_t LIMIT;

  // Indexes
  Indexes<std::string> string_indexes_;



  template <typename Value>
  inline Indexes<Value>& get_indexes()
  { return get_indexes(Type<Value>()); }

  inline Indexes<std::string>& get_indexes(Type<std::string>)
  { return string_indexes_; }

  // Constraints

  inline bool constraints_fails(const T&) const;

  template <typename Value>
  inline bool constraints_fails(const Indexes<Value>&, const T&) const;

  template <typename Value>
  inline bool is_unique(const IndexedColumn<Value>&, const T&) const;

  template <typename Value>
  inline bool is_null(const IndexedColumn<Value>&, const T&) const;
  // string impl
  inline bool is_null(const IndexedColumn<std::string>&, const T&) const;

  inline void update_indexes(const T&);

  template <typename Value>
  inline void update_indexes(Indexes<Value>&, const T&);

};

template <typename T>
Bucket<T>::Bucket(size_t limit)
  : idx_{1}, bucket_{}, LIMIT{limit}
{

}

template <typename T>
size_t Bucket<T>::capture(T& obj) {
  if(bucket_.size() >= LIMIT)
    throw CannotCreateObject{"The Bucket is full. Please come again!"};

  if(constraints_fails(obj))
    return 0;

  obj.key = idx_++;
  bucket_.insert({obj.key, obj});

  update_indexes(obj);

  return obj.key;
}

template <typename T>
template <typename... Args>
T& Bucket<T>::spawn(Args&&... args) {
  T obj{args...};
  Key id = capture(obj);
  if(!id)
    throw CannotCreateObject{"The object got away.."};
  return pick_up(id);
}

template <typename T>
T& Bucket<T>::pick_up(const Key key) {
  auto it = bucket_.find(key);
  if(it not_eq bucket_.end()) {
    return it->second;
  }
  throw ObjectNotFound{"No object exist with key: " + std::to_string(key)};
}

template <typename T>
bool Bucket<T>::abandon(const Key key) {
  return bucket_.erase(key);
}

template <typename T>
template <typename Value>
T& Bucket<T>::look_for(const Column& col, const Value& val) {
  auto& indexes = get_indexes<Value>();
  // check for column
  const auto indexes_it = indexes.find(col);
  if(indexes_it == indexes.end())
    throw ObjectNotFound{"Column not indexed: " + col};

  // check for value
  auto& idx_col = indexes_it->second;
  const auto it = idx_col.index.find(val);
  if(it == idx_col.index.end()) {
    using namespace std;
    throw ObjectNotFound{"Value [" + to_string(val) + "] does not exist in Column: " + col};
  }

  return pick_up(it->second);
}

template <typename T>
std::vector<T> Bucket<T>::lineup() const {
  std::vector<T> line;
  line.reserve(bucket_.size());
  for(auto& content : bucket_) {
    line.push_back(content.second);
  }
  assert(line.size() == bucket_.size());
  return line;
}

template <typename T>
template <typename Writer>
void Bucket<T>::serialize(Writer& writer) const {
  writer.StartArray();
  for(auto& content : bucket_) {
    content.second.serialize(writer);
  }
  writer.EndArray();
}

template <typename T>
template <typename Value>
void Bucket<T>::add_index(Column&& col, Resolver<Value> res, Constraint con) {
  IndexedColumn<Value> ic;
  ic.resolver = res;
  ic.constraint = con;
  get_indexes<Value>().emplace(col, ic);
}

template <typename T>
inline bool Bucket<T>::constraints_fails(const T& obj) const {
  // check all string values
  if(constraints_fails(string_indexes_, obj))
    return true;

  return false;
}

template <typename T>
template <typename Value>
inline bool Bucket<T>::constraints_fails(const Indexes<Value>& indexes, const T& obj) const {
  for(auto& idx : indexes) {
    auto& idx_col = idx.second;
    switch(idx_col.constraint) {

      case UNIQUE: {
        if(!is_unique(idx_col, obj))
          throw ConstraintUnique{"Not Unique."};
        break;
      }

      case NOT_NULL: {
        if(is_null(idx_col, obj))
          throw ConstraintNull{"Null value."};
        break;
      }
      default:
        break;
    }
  } // < switch Constraint
  return false;
}

template <typename T>
template <typename Value>
inline bool Bucket<T>::is_unique(const IndexedColumn<Value>& col, const T& obj) const
{ return col.index.find(col.resolver(obj)) == col.index.end(); }

template <typename T>
template <typename Value>
inline bool Bucket<T>::is_null(const IndexedColumn<Value>& col, const T& obj) const
{ return col.resolver(obj) == 0; }

template <typename T>
inline bool Bucket<T>::is_null(const IndexedColumn<std::string>& col, const T& obj) const
{ return col.resolver(obj).empty(); }


template <typename T>
inline void Bucket<T>::update_indexes(const T& obj) {
  update_indexes(string_indexes_, obj);
}

template <typename T>
template <typename Value>
inline void Bucket<T>::update_indexes(Indexes<Value>& indexes, const T& obj) {
  for(auto& idx : indexes) {
    auto& idx_col = idx.second;
    idx_col.index[idx_col.resolver(obj)] = obj.key;
  }
}

} // < namespace bucket

#endif //< BUCKET_BUCKET_HPP
