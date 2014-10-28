#ifndef STD_INITIALIZER_LIST_HPP
#define STD_INITIALIZER_LIST_HPP

#include "type_traits.hpp"

namespace std
{
	template <class T>
	class initializer_list
	{
	public:
		typedef T         value_type;
		typedef const T&  reference;
		typedef const T&  const_reference;
		typedef const T*  iterator;
		typedef const T*  const_iterator;
		typedef size_t    size_type;
		
		constexpr initializer_list()
			: m_array(0), m_size(0) {}
		
		// Number of elements.
		constexpr size_type
		size() const noexcept { return m_size; }
		
		// First element.
		constexpr const_iterator
		begin() const noexcept { return m_array; }
		
		// One past the last element.
		constexpr const_iterator
		end() const noexcept { return begin() + size(); }
		
	private:
		constexpr initializer_list(const_iterator a, size_type len)
			: m_array(a), m_size(len) {}
		
		iterator  m_array;
		size_type m_size;
	};
	
}

#endif
