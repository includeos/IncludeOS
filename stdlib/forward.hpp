#ifndef STD_FORWARD_HPP
#define STD_FORWARD_HPP

#include "type_traits.hpp"

namespace std
{
	// std::forward
	template <class T>
	constexpr T&& forward(remove_reference_t<T>& t) noexcept
	{
		return static_cast<T&&>(t);
	}
	template <class T>
	constexpr T&& forward(remove_reference_t<T>&& t) noexcept
	{
		return static_cast<T&&>(t);
	}
	
}

#endif
