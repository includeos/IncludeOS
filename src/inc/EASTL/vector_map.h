#ifndef EASTL_VECTOR_MAP_H
#define EASTL_VECTOR_MAP_H

#include <EASTL/internal/config.h>

#include <EASTL/algorithm.h>
#include <EASTL/allocator.h>
#include <EASTL/functional.h>
#include <EASTL/iterator.h>
#include <EASTL/map.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>


namespace eastl {

namespace detail {

template<class Pair, class Compare>
struct compare_impl {
  typedef Pair pair_type;
  typedef typename pair_type::first_type first_argument_type;

  compare_impl() {}
  compare_impl(Compare const& src) : m_cmp(src) {}

  bool operator()(first_argument_type const& lhs, first_argument_type const& rhs) const
  { return Compare()(lhs, rhs); }
  bool operator()(pair_type const& lhs, pair_type const& rhs) const
  { return operator()(lhs.first, rhs.first); }
  bool operator()(pair_type const& lhs, first_argument_type const& rhs) const
  { return operator()(lhs.first, rhs); }
  bool operator()(first_argument_type const& lhs, pair_type const& rhs) const
  { return operator()(lhs, rhs.first); }

  operator Compare() const { return m_cmp; }
  void swap(compare_impl& rhs) {
    using ::eastl::swap;
    swap(m_cmp, rhs.m_cmp);
  }
 private:
  Compare m_cmp;
}; // struct compare_impl

} // namespace detail

template<class K, class V, class C = ::eastl::less<K>, class A = EASTLAllocatorType>
class vector_map {
 public:
  typedef K key_type;
  typedef V mapped_type;
  typedef ::eastl::pair<key_type, mapped_type> value_type;
  typedef C key_compare;
  typedef A allocator_type;

  typedef ::eastl::vector<value_type, allocator_type> base_type;
 private:
  typedef detail::compare_impl<key_type, key_compare> compare_impl_type;

  base_type m_base;
  detail::compare_impl<typename base_type::value_type, key_compare> m_cmp;
 public:
  typedef typename base_type::iterator iterator;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;

  typedef typename base_type::size_type size_type;
  typedef typename base_type::difference_type difference_type;

  typedef typename base_type::reference reference;
  typedef typename base_type::const_reference const_reference;
  typedef typename base_type::pointer pointer;
  typedef typename base_type::const_pointer const_pointer;;

  static const size_type kMaxSize = base_type::kMaxSize;

  class value_compare {
    friend class vector_map;
    key_compare const m_cmp;
   protected:
    value_compare(key_compare pred) : m_cmp(pred) {}
   public:
    bool operator()(value_type const& lhs, value_type const& rhs) const
    { return m_cmp(lhs.first, rhs.first); }
  }; // struct value_compare

  explicit vector_map(key_compare const& cmp = key_compare(), allocator_type const& alloc = EASTL_VECTOR_DEFAULT_ALLOCATOR)
      : m_base(alloc), m_cmp(cmp) {}
  template<class InputIterator>
  vector_map(InputIterator first, InputIterator last,
             key_compare const& cmp = key_compare(),
             allocator_type const& alloc = EASTL_VECTOR_DEFAULT_ALLOCATOR)
      : m_base(alloc), m_cmp(cmp)
  {
    ::eastl::map<key_type, mapped_type, key_compare, allocator_type> const tmp(first, last);
    m_base.reserve(tmp.size());
    ::eastl::copy(tmp.begin(), tmp.end(), ::eastl::back_inserter(m_base));
  }

  vector_map& operator =(vector_map const& rhs) {
    vector_map(rhs).swap(*this);
    return *this;
  }

  iterator begin() { return m_base.begin(); }
  iterator end() { return m_base.end(); }
  reverse_iterator rbegin() { return m_base.rbegin(); }
  reverse_iterator rend() { return m_base.rend(); }

  const_iterator begin() const { return m_base.begin(); }
  const_iterator end() const { return m_base.end(); }
  const_reverse_iterator rbegin() const { return m_base.rbegin(); }
  const_reverse_iterator rend() const { return m_base.rend(); }

  void clear() { m_base.clear(); }

  bool empty() const { return m_base.empty(); }
  size_type size() const { return m_base.size(); }
  size_type max_size() { return base_type::kMaxSize; }

  mapped_type& operator[](key_type const& key) {
    return insert(value_type(key, mapped_type())).first->second;
  }

  ::eastl::pair<iterator, bool> insert(value_type const& val) {
    iterator const i(lower_bound(val.first));

    return (i == end() || m_cmp(val.first, i->first))
        ? ::eastl::make_pair(m_base.insert(i, val), true)
        : ::eastl::make_pair(i, false)
        ;
  }
  iterator insert(iterator const pos, value_type const& val) {
    return
        ((pos == begin() || m_cmp(*(pos-1), val)) &&
         (pos ==   end() || m_cmp(val, *pos)))
        ? m_base.insert(pos, val)
        : insert(val).first
        ;
  }
  template<class InputIterator>
  void insert(InputIterator first, InputIterator const last)
  { for(; first != last; ++first) insert(*first); }

  void erase(iterator const pos) { m_base.erase(pos); }
  void erase(iterator const first, iterator const last)
  { m_base.erase(first, last); }
  size_type erase(key_type const& k) {
    iterator const i(find(k));
    if(i == end()) return 0;
    else { erase(i); return 1; }
  }

  void swap(vector_map& other) {
    m_base.swap(other.m_base);
    m_cmp.swap(other.m_cmp);
  }

  allocator_type& get_allocator() { return m_base.get_allocator(); }
  void set_allocator(allocator_type const& alloc) { m_base.set_allocator(alloc); }

  key_compare key_comp() const { return static_cast<key_compare>(m_cmp); }
  value_compare value_comp() const { return value_compare(m_cmp); }

  iterator find(key_type const& k) {
    iterator const i(lower_bound(k));
    return (i != end() && m_cmp(k, i->first))? end() : i;
  }
  const_iterator find(key_type const& k) const {
    const_iterator const i(lower_bound(k));
    return (i != end() && m_cmp(k, i->first))? end() : i;
  }
  size_type count(key_type const& k) const { return find(k) != end()? 1 : 0; }
  iterator lower_bound(key_type const& k) {
    return ::eastl::lower_bound(begin(), end(), k, m_cmp);
  }
  const_iterator lower_bound(key_type const& k) const {
    return ::eastl::lower_bound(begin(), end(), k, m_cmp);
  }
  iterator upper_bound(key_type const& k) {
    return ::eastl::upper_bound(begin(), end(), k, m_cmp);
  }
  const_iterator upper_bound(key_type const& k) const {
    return ::eastl::upper_bound(begin(), end(), k, m_cmp);
  }

  ::eastl::pair<iterator, iterator> equal_range(key_type const& k) {
    return ::eastl::equal_range(begin(), end(), k, m_cmp);
  }
  ::eastl::pair<const_iterator, const_iterator> equal_range(key_type const& k) const {
    return ::eastl::equal_range(begin(), end(), k, m_cmp);
  }

  template<class Key, class T, class Compare, class Allocator>
  friend bool operator==(vector_map<Key, T, Compare, Allocator> const& lhs,
                         vector_map<Key, T, Compare, Allocator> const& rhs);
  template<class Key, class T, class Compare, class Allocator>
  friend bool operator!=(vector_map<Key, T, Compare, Allocator> const& lhs,
                         vector_map<Key, T, Compare, Allocator> const& rhs);
  template<class Key, class T, class Compare, class Allocator>
  friend bool operator<(vector_map<Key, T, Compare, Allocator> const& lhs,
                        vector_map<Key, T, Compare, Allocator> const& rhs);
  template<class Key, class T, class Compare, class Allocator>
  friend bool operator>(vector_map<Key, T, Compare, Allocator> const& lhs,
                        vector_map<Key, T, Compare, Allocator> const& rhs);
  template<class Key, class T, class Compare, class Allocator>
  friend bool operator>=(vector_map<Key, T, Compare, Allocator> const& lhs,
                         vector_map<Key, T, Compare, Allocator> const& rhs);
  template<class Key, class T, class Compare, class Allocator>
  friend bool operator<=(vector_map<Key, T, Compare, Allocator> const& lhs,
                         vector_map<Key, T, Compare, Allocator> const& rhs);
}; // class vector_map

template<class Key, class T, class Compare, class Allocator>
inline bool operator==(vector_map<Key, T, Compare, Allocator> const& lhs,
                       vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base == rhs.m_base; }
template<class Key, class T, class Compare, class Allocator>
inline bool operator!=(vector_map<Key, T, Compare, Allocator> const& lhs,
                       vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base != rhs.m_base; }
template<class Key, class T, class Compare, class Allocator>
inline bool operator<(vector_map<Key, T, Compare, Allocator> const& lhs,
                      vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base <  rhs.m_base; }
template<class Key, class T, class Compare, class Allocator>
inline bool operator>(vector_map<Key, T, Compare, Allocator> const& lhs,
                      vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base > rhs.m_base; }
template<class Key, class T, class Compare, class Allocator>
inline bool operator>=(vector_map<Key, T, Compare, Allocator> const& lhs,
                       vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base >= rhs.m_base; }
template<class Key, class T, class Compare, class Allocator>
inline bool operator<=(vector_map<Key, T, Compare, Allocator> const& lhs,
                       vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.m_base <= rhs.m_base; }

template<class Key, class T, class Compare, class Allocator>
void swap(vector_map<Key, T, Compare, Allocator> const& lhs,
          vector_map<Key, T, Compare, Allocator> const& rhs)
{ return lhs.swap(rhs); }

} // namespace eastl

#endif // EASTL_VECTOR_MAP_H
