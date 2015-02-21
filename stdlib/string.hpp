#ifndef STD_STRING_HPP
#define STD_STRING_HPP

#include "EASTL/string.h"
#include <stdlib.h>

namespace std
{
	using string = eastl::string;
	
	///  std::to_string variants  ///
	inline string to_string(const string& s)
	{
		return s;
	}
	inline string to_string(const char* c)
	{
		return string(c);
	}
	inline string to_string(char* c)
	{
		return string(c);
	}
	
	template <class T>
	inline string to_string(T* t)
	{
		// "0x" + (2 per byte as hex value) + zero
		string result(sizeof(T*) * 2 + 3, 0);
		int len = sprintf( (char*) result.data(), "%p", t);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(char c)
	{
		string result(8, 0);
		int len = sprintf( (char*) result.data(), "%hhd", c);
    result.resize(len);
		
		return result;
	}
	inline string to_string(unsigned char c)
	{
		string result(8, 0);
		int len = sprintf( (char*) result.data(), "%hhu", c);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(short x)
	{
		string result(8, 0);
		int len = sprintf( (char*) result.data(), "%hd", x);
    result.resize(len);
		
		return result;
	}
	inline string to_string(unsigned short x)
	{
		string result(8, 0);
		int len = sprintf( (char*) result.data(), "%hu", x);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(int x)
	{
		string result(12, 0);
		int len = sprintf( (char*) result.data(), "%d", x);
    result.resize(len);
    
		return result;
	}
	inline string to_string(unsigned int x)
	{
		string result(12, 0);
		int len = sprintf( (char*) result.data(), "%u", x);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(long x)
	{
		string result(22, 0);
		int len = sprintf( (char*) result.data(), "%ld", x);
    result.resize(len);
		
		return result;
	}
	inline string to_string(unsigned long x)
	{
		string result(22, 0);
		int len = sprintf( (char*) result.data(), "%lu", x);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(long long x)
	{
		string result(32, 0);
		int len = sprintf( (char*) result.data(), "%lld", x);
    result.resize(len);
		
		return result;
	}
	inline string to_string(unsigned long long x)
	{
		string result(32, 0);
		int len = sprintf( (char*) result.data(), "%llu", x);
    result.resize(len);
		
		return result;
	}
	
	inline string to_string(float x)
	{
		string result(18, 0);
		int len = sprintf( (char*) result.data(), "%f", x);
    result.resize(len);
		
		return result;
	}
	inline string to_string(double x)
	{
		string result(32, 0);
		int len = sprintf( (char*) result.data(), "%f", x);
    result.resize(len);
		
		return result;
	}
	inline string to_string(long double x)
	{
		string result(38, 0);
		int len = sprintf( (char*) result.data(), "%Lf", x);
    result.resize(len);
		
		return result;
	}
	
	inline int stoi(const string& str, size_t* idx = 0, int base = 10)
	{
		int result = 0;
		
		if (str.empty())
		{
			if (idx) *idx = 0;
			// TODO: set errno properly (?)
			return result;
		}
		
		string::const_iterator i = str.begin();
		bool negative = (*i == '-');
		
		if (negative)
		{
			++i;
			if (i == str.end())
			{
				if (idx) *idx = 0;
				// TODO: set errno properly
				return result;
			}
		}
		
		for (; i != str.end(); ++i)
		{
			if (*i < '0' || *i > '9')
			{
				if (idx) *idx = str.begin() - i;
				// TODO: set errno properly
				return result;
			}

			result *= base;
			result += *i - '0';
		}
		
		return (negative) ? -result : result;
    }
}

#endif
