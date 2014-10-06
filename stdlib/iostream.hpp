#ifndef STD_IOSTREAM_HPP
#define STD_IOSTREAM_HPP

#include <stdio.h>
#include <string>

namespace std
{
	enum StreamToken
	{
		endl
	};
	
	class ostream
	{
	public:
		inline ostream& 
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
		inline ostream& 
		operator << (const T& type)
		{
			printf("%s", to_string(type).data());
			return *this;
		}
	};
	class istream
	{
		
	};
	extern ostream cout;
	extern istream cin;
}

#endif
