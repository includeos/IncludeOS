#ifndef BUCKET_BUCKET_HPP
#define BUCKET_BUCKET_HPP

#include <vector>
#include <functional>
#include <string>
#include <unordered_map>

#include "bucket_errors.hpp"

namespace bucket {

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
  };

public:
  enum Constraint {
    NONE,
    UNIQUE
  };

  explicit Bucket();

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

  /**
   * @brief Lineup everything inside the bucket.
   * @details Builds a vector with only the values from the underlying container.
   * @return A vector with all the content inside the bucket.
   */
  std::vector<T> lineup() const;

  void set_unique_constraint(IsEqual func) {
    check_if_equal = func;
  }

  template <typename Value>
  void add_index(Column&& col, Resolver<Value> res, Constraint con = NONE)
  { add_index(Type<Value>(), std::forward<Column>(col), res, con); };

  template <typename Writer>
  void serialize(Writer& writer) const;

private:
  Key        idx_;
  Collection bucket_;

  IsEqual check_if_equal;

  std::unordered_map<Column, IndexedColumn<std::string>> string_indexes_;

  template <typename Value>
  void add_index(Type<Value>, Column&&, Resolver<Value>, Constraint);

  // string implementation
  void add_index(Type<std::string>, Column&&, Resolver<std::string>, Constraint);
};

template <typename T>
Bucket<T>::Bucket() : idx_{1}, bucket_{}
{

}

template <typename T>
size_t Bucket<T>::capture(T& obj) {
  for(auto& str_idx : string_indexes_) {
    auto& idx_col = str_idx.second;
    if(idx_col.index.find(idx_col.resolver(obj)) != idx_col.index.end())
      return 0;
  }

  obj.key = idx_++;
  bucket_.insert({obj.key, obj});

  for(auto& str_idx : string_indexes_) {
    auto& idx_col = str_idx.second;
    idx_col.index[idx_col.resolver(obj)] = obj.key;
  }

  return obj.key;
}

template <typename T>
template <typename... Args>
T& Bucket<T>::spawn(Args&&... args) {
  T obj{args...};
  Key id = capture(obj);
  if(!id)
    throw BucketException{"Can not spawn."};
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
void Bucket<T>::add_index(Type<std::string>, Column&& col, Resolver<std::string> res, Constraint con) {
  IndexedColumn<std::string> ic;
  ic.resolver = res;
  string_indexes_.emplace(col, ic);
}

/*template <typename T> template <>
void Bucket<T>::add_constraint<std::string>(Column&& col, Resolver<std::string> resolver) {

}*/

} // < namespace bucket

#endif //< BUCKET_BUCKET_HPP
