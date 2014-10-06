#ifndef STD_UTILITY_HPP
#define STD_UTILITY_HPP

#include "string"
#include <stdlib.h>

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
	
	// std::to_string
	inline string to_string(string s)
	{
		return s;
	}
	inline string to_string(const char* c)
	{
		return string(c);
	}
	
	inline string to_string(int x)
	{
		string result(20, 0);
		sprintf( (char*) result.data(), "%d", x);
		
		return result;
	}
	inline string to_string(unsigned int x)
	{
		string result(20, 0);
		sprintf( (char*) result.data(), "%u", x);
		
		return result;
	}
	
}

#endif
