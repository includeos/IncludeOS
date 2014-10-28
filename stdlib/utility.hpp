#ifndef STD_UTILITY_HPP
#define STD_UTILITY_HPP

#include "declval.hpp"
#include "forward.hpp"
#include "type_traits.hpp"

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
	
	// std::pair
	template <class T1, class T2>
	struct pair
	{
		typedef T1 first_type;
		typedef T2 second_type;
		
		pair() : first(), second() {};
		
		template<class U, class V>
		pair (const pair<U, V>& pr)
			: first(pr.first), second(pr.second) {}
		
		pair (const first_type& a, const second_type& b)
			: first(a), second(b) {}
		
		template <class U, class V>
		pair& operator= (const pair<U, V>& pr)
		{
			first = pr.first; second = pr.second;
		}
		
		template <class U, class V>
		pair& operator= (pair<U, V>&& pr)
		{
			first  = move(pr.first);
			second = move(pr.second);
		}
		
		// members
		first_type  first;
		second_type second;
	};
	
	// std::make_pair
	template <class T1, class T2>
	pair<T1, T2> make_pair (T1&& x, T2&& y)
	{
		return pair<T1, T2>(forward<T1>(x), forward<T2>(y));
	}
	
}

#endif
