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
		
		inline ostream& 
		operator << (char type)
		{
			printf("%c", type);
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
	public:
		istream& operator >> (string& str)
		{
			// reserve enough for sizeable stream input
      str.reserve(100);
			// read from stream
      fgets ((char*) str.c_str(), 100, stdin);
			// downsize (without realloc)
      str.resize(strlen(str.c_str()));
      
			return *this;
		}
	};
	extern ostream cout;
	extern istream cin;
}

#endif
