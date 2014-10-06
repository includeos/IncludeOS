#ifndef STD_IOSTREAM_HPP
#define STD_IOSTREAM_HPP

#include <stdio.h>
#include <utility>

namespace std
{
	enum StreamToken
	{
		endl
	};
	
	class OutputStream
	{
	public:
		inline OutputStream& 
		operator << (StreamToken token)
		{
			switch (token)
			{
			case endl:
				printf("\n");
				break;
			}
			return *this;
		}
		
		template <class T>
		inline OutputStream& 
		operator << (const T& type)
		{
			printf("%s", to_string(type).data());
			return *this;
		}
	};
	class InputStream
	{
		
	};
	extern OutputStream cout;
	extern InputStream  cin;
	
}

#endif
