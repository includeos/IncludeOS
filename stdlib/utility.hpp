#ifndef STD_UTILITY_HPP
#define STD_UTILITY_HPP

#include "type_traits.hpp"
#include "forward.hpp"

namespace std
{
	// std::move
	template<class T>
	constexpr remove_reference_t<T>&& move(T&& t)
	{
		return static_cast<T&&>(t);
	}
	
	// std::swap
	template <class T>
	inline void swap(T& t1, T& t2)
	{
		T t3 = t1;
		t1 = t2;
		t2 = t3;
	}
	
}

#endif
