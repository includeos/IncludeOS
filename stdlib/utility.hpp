#ifndef STD_UTILITY_HPP
#define STD_UTILITY_HPP

namespace std
{
	// std::move
	template <class X>
	inline X&& move(X& a) noexcept
	{
		return static_cast<X&&>(a);
	}
	
	// std::forward
	template <class T, class U>
	inline T&& forward(U&& u)
	{
		return static_cast<T&&>(u);
	}
	
}

#endif
