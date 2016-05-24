#ifndef BUCKET_BUCKET_HPP
#define BUCKET_BUCKET_HPP

#include <map>
#include <vector>

namespace bucket {

/**
 * @brief A (magical) bucket to store stuff inside
 * @details Stores an object together with a key.
 *
 * @tparam T What to put in the bucket
 */
template <typename T>
class Bucket {
private:
  using Key = size_t;
  using Collection = std::map<Key, T>;
  using Content = std::pair<Key, T>;

public:
  Bucket();

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
   * @details Call emplace on the underlying map, while also assigning a unique key.
   *
   * @param  Constructor args to something
   * @return The created something
   */
  template <typename... Args>
  T& spawn(Args&&...);

  /**
   * @brief Pick up the something with the given key inside the bucket.
   * @details Using key to find the object inside the map. Throws if not found
   *
   * @param  The key to the something
   * @return (Hopefully) the something with the given key.
   */
  T& pick_up(const Key);

  /**
   * @brief Abandon/release the something with the given key
   * @details Calls earse on the underlying map
   *
   * @param  The key to the something.
   * @return Wheter something was abandoned or not.
   */
  bool abandon(const Key);

  /**
   * @brief Lineup everything inside the bucket.
   * @details Builds a vector with only the values from the underlying map.
   * @return A vector with all the content inside the bucket.
   */
  std::vector<T> lineup() const;

  template <typename Writer>
  void serialize(Writer& writer) const;

private:
  Key idx;
  Collection bucket_;

};

template <typename T>
Bucket<T>::Bucket() : idx(1), bucket_()
{

}

template <typename T>
size_t Bucket<T>::capture(T& obj) {
  obj.key = idx++;
  bucket_.insert({obj.key, obj});
  return obj.key;
}

template <typename T>
template <typename... Args>
T& Bucket<T>::spawn(Args&&... args) {
  Key id = idx++;
  bucket_.emplace(id, T{args...});
  auto& obj = bucket_[id];
  obj.key = id;
  return obj;
}

template <typename T>
T& Bucket<T>::pick_up(const Key key) {
  auto it = bucket_.find(key);
  if(it != bucket_.end())
    return it->second;
  throw "not found";
}

template <typename T>
bool Bucket<T>::abandon(const Key key) {
  return bucket_.erase(key);
}

template <typename T>
std::vector<T> Bucket<T>::lineup() const {
  std::vector<T> vec;
  vec.reserve(bucket_.size());
  for(auto content : bucket_)
    vec.push_back(content.second);
  assert(vec.size() == bucket_.size());
  return vec;
}

template <typename T>
template <typename Writer>
void Bucket<T>::serialize(Writer& writer) const {
  writer.StartArray();
  for(auto content : bucket_)
    content.second.serialize(writer);
  writer.EndArray();
}



}; // < namespace bucket

#endif
