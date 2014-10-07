#ifndef STD_STRING_HPP
#define STD_STRING_HPP

#include "EASTL/string.h"
#include <stdlib.h>

namespace std
{
	using string = eastl::string;
	
	// std::to_string
	inline string to_string(string s)
	{
		return s;
	}
	inline string to_string(const char* c)
	{
		return string(c);
	}
	
	template <class T>
	inline string to_string(T* t)
	{
		string result(32, 0);
		sprintf( (char*) result.data(), "%p", t);
		
		return result;
	}
	
	inline string to_string(char c)
	{
		string result(8, 0);
		sprintf( (char*) result.data(), "%hhd", c);
		
		return result;
	}
	inline string to_string(unsigned char c)
	{
		string result(8, 0);
		sprintf( (char*) result.data(), "%hhu", c);
		
		return result;
	}
	
	inline string to_string(short x)
	{
		string result(8, 0);
		sprintf( (char*) result.data(), "%hd", x);
		
		return result;
	}
	inline string to_string(unsigned short x)
	{
		string result(8, 0);
		sprintf( (char*) result.data(), "%hu", x);
		
		return result;
	}
	
	inline string to_string(int x)
	{
		string result(12, 0);
		sprintf( (char*) result.data(), "%d", x);
		
		return result;
	}
	inline string to_string(unsigned int x)
	{
		string result(12, 0);
		sprintf( (char*) result.data(), "%u", x);
		
		return result;
	}
	
	inline string to_string(long x)
	{
		string result(22, 0);
		sprintf( (char*) result.data(), "%ld", x);
		
		return result;
	}
	inline string to_string(unsigned long x)
	{
		string result(22, 0);
		sprintf( (char*) result.data(), "%lu", x);
		
		return result;
	}
	
	inline string to_string(long long x)
	{
		string result(32, 0);
		sprintf( (char*) result.data(), "%lld", x);
		
		return result;
	}
	inline string to_string(unsigned long long x)
	{
		string result(32, 0);
		sprintf( (char*) result.data(), "%llu", x);
		
		return result;
	}
	
	inline string to_string(float x)
	{
		string result(18, 0);
		sprintf( (char*) result.data(), "%f", x);
		
		return result;
	}
	inline string to_string(double x)
	{
		string result(32, 0);
		sprintf( (char*) result.data(), "%f", x);
		
		return result;
	}
	inline string to_string(long double x)
	{
		string result(38, 0);
		sprintf( (char*) result.data(), "%Lf", x);
		
		return result;
	}
}

#endif
