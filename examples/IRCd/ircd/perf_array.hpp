#pragma once
#include "common.hpp"
#include "ciless.hpp"
#include <deque>

class IrcServer;

template <typename T, typename IDX>
struct perf_array
{
  using typevec = std::deque<T>;
  using iterator       = typename typevec::iterator;
  using const_iterator = typename typevec::const_iterator;

  // generate new id
  T& create(IrcServer&);
  // create and hash
  T& create(IrcServer&, const std::string& name);
  // force-create empty element
  T& create_empty(IrcServer&);

  T& get(IDX idx) {
    return tvec.at(idx);
  }

  IDX find(const std::string&) const;

  bool   empty() const noexcept {
    return tvec.empty();
  }
  size_t size() const noexcept {
    return tvec.size();
  }

  void free(T& object);

  // T iterators
  auto begin() {
    return tvec.begin();
  }
  auto begin() const {
    return tvec.cbegin();
  }
  auto end() {
    return tvec.end();
  }
  auto end() const {
    return tvec.cend();
  }

  // hash map
  auto& hash_map() noexcept {
    return h_map;
  }
  void hash(const std::string& value, IDX idx)
  {
    h_map[value] = idx;
  }
  void emplace_hash(std::string value, IDX idx)
  {
    h_map.emplace(std::move(value), idx);
  }
  void erase_hash(const std::string& value)
  {
    h_map.erase(value);
  }

private:
  typevec          tvec;
  std::vector<IDX> free_idx;
  std::map<std::string, IDX, ci_less> h_map;
};

template <typename T, typename IDX>
T& perf_array<T, IDX>::create(IrcServer& srv)
{
  // use prev idx
  if (free_idx.empty() == false) {
    IDX idx = free_idx.back();
    free_idx.pop_back();
    return tvec[idx];
  }
  // create new value
  tvec.emplace_back(tvec.size(), srv);
  return tvec.back();
}
template <typename T, typename IDX>
T& perf_array<T, IDX>::create(IrcServer& srv, const std::string& name)
{
  auto& type = create(srv);
  this->hash(name, type.get_id());
  return type;
}
template <typename T, typename IDX>
T& perf_array<T, IDX>::create_empty(IrcServer& srv)
{
  tvec.emplace_back(tvec.size(), srv);
  return tvec.back();
}

template <typename T, typename IDX>
IDX perf_array<T, IDX>::find(const std::string& name) const
{
  auto it = h_map.find(name);
  if (it != h_map.end()) return it->second;
  return (IDX) -1;
}

template <typename T, typename IDX>
void perf_array<T, IDX>::free(T& type)
{
  // give back id
  free_idx.push_back(type.get_id());
  // give back name, if it has been given one
  if (!type.name_hash().empty())
      erase_hash(type.name_hash());
}
