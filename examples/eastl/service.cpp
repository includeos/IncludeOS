#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <tuple.hpp>
#include <signal>
#include <delegate>

class TestStream
{
public:
	friend std::ostream& operator << (std::ostream& out, const TestStream& test);
};
std::ostream& operator << (std::ostream& out, const TestStream&)
{
	return out << std::string("Test Stream");
}
TestStream testStream;

struct PrintNum
{
    void operator()(int i) const
    {
        std::cout << i << std::endl;
    }
};
void testFunction()
{
	std::cout << "called void testFunction()" << std::endl;
}


namespace std
{
	template <typename F, typename... Args>
	class bind
	{
	private:
		std::function<F> func;
		std::tuple<Args...> elem;
		
	public:
		bind(F function, Args... args)
			: func(function), elem(args...)
		{}
		
		void operator() ()
		{
			//func(elem...);
		}
	};
}

class TestSignal
{
public:
	signal<void()> test;
};


void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
	std::function<int* ()> 
	testLambda = []
	{
		std::cout << "std::function<int* ()> called" << std::endl;
		return nullptr;
	};
	
	struct Song
	{
		Song(std::string art, std::string tit)
			: artist(art), title(tit) {}
		
		std::string artist;
		std::string title;
	};
	
	/////////////////////////////////////////////////////////////////////////////
	//// std::ostream, std::istream
	/////////////////////////////////////////////////////////////////////////////
	{
		eastl::string str("test string");
		std::cout << str << " int: " << 52 << " long: " << 52L << std::endl;
		std::cout << "short: " << (short)52 << " char: " << 'C' << std::endl;
		std::cout << "pointer: " << &str << std::endl;
		
		std::cout << "class: " << testStream << std::endl;
		
		std::cin >> str;
		std::cout << "You wrote: " << str << std::endl;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::vector
	/////////////////////////////////////////////////////////////////////////////
	{
		std::vector<int> vec;
		vec.resize(500);
		vec[0] = 500;
		vec[499] = 500;
		
		assert(vec[499] == 500);
		vec.resize(1);
		assert(vec[0] == 500);
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::vector::emplace_back
	/////////////////////////////////////////////////////////////////////////////
	{
		std::vector<Song> empl_test;
		empl_test.emplace_back("Bob Dylan", "The Times They Are A Changing");
		
		for (const auto& s : empl_test)
		{
			std::cout << s.artist << ":" << s.title << std::endl;
		}
		
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::map
	/////////////////////////////////////////////////////////////////////////////
	{
		std::map<int, int> testMap;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::function
	/////////////////////////////////////////////////////////////////////////////
	{
		std::function<void()> test = testFunction;
		
		std::cout << "calling std::function:" << std::endl;
		test();
	}
	/////////////////////////////////////////////////////////////////////////////
	//// vector<std::function>
	/////////////////////////////////////////////////////////////////////////////
	{
		std::vector<std::function<void()>> fvec;
		fvec.push_back(testFunction);
		fvec.push_back(testFunction);
		
		std::cout << "vector[0] == " << &fvec[0] << std::endl;
		std::cout << "vector[1] == " << &fvec[1] << std::endl;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::function + lambda
	/////////////////////////////////////////////////////////////////////////////
	{
		std::cout << "calling std::function lambda:" << std::endl;
		std::cout << "result: " << testLambda() << std::endl;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// signal
	/////////////////////////////////////////////////////////////////////////////
	{
		TestSignal testSignal;
		testSignal.test.connect(
		[] {
			std::cout << "std::signal lambda test" << std::endl;
		});
		testSignal.test.connect(testFunction);
		testSignal.test.connect(testFunction);
		
		std::cout << "emitting signal:" << std::endl;
		testSignal.test.emit();
	}
	/////////////////////////////////////////////////////////////////////////////
	//// delegate
	/////////////////////////////////////////////////////////////////////////////
	{
		std::function<void()> test = testFunction;
		
		delegate<void()> delgStatic = testFunction;
		delegate<void()> delgDynamic = test;
		delegate<int*()> delgFunctor = testLambda;
		
		std::cout << "calling delegates:" << std::endl;
		delgStatic();
		delgDynamic();
		std::cout << "result: " << delgFunctor() << std::endl;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::shared_ptr, std::make_shared
	/////////////////////////////////////////////////////////////////////////////
	{
		std::cout << "creating shared pointers:" << std::endl;
		
		std::vector<std::shared_ptr<Song>> sv;
		
		sv.push_back(
			std::make_shared<Song>("Bob Dylan", "The Times They Are A Changing"));
		sv.push_back(
			std::make_shared<Song>("Aretha Franklin", "Bridge Over Troubled Water"));
		sv.push_back(
			std::make_shared<Song>("Thalia", "Entre El Mar y Una Estrella"));
		sv.push_back(
                             std::make_shared<Song>("Alf Prøysen", "Blåbærturen")); //æøå
		
		/*std::vector<std::shared_ptr<Song>> v2;
		std::remove_copy_if(v.begin(), v.end(), back_inserter(v2), 
		[] (std::shared_ptr<Song> s) 
		{
			return s->artist.compare(L"Bob Dylan") == 0;		
		});*/
		
		for (const auto& s : sv)
		{
			std::cout << s->artist << ":" << s->title << std::endl;
		}
		std::cout << "result: " << sv[0]->artist << 
			" (usage: " << sv[0].use_count() << ")" << std::endl;
		
		std::shared_ptr<Song> shared2(sv[0]);
		std::cout << "result: " << shared2->artist << 
			" (usage: " << sv[0].use_count() << " == " << shared2.use_count() << ")" << std::endl;
	}
        
        /////////////////////////////////////////////////////////////////////////////
	//// std::string
	/////////////////////////////////////////////////////////////////////////////

        std::string s("A string!");
        
        std::cout << s << std::endl;
        for(auto it = s.begin(); it != s.end(); ++it)
          std::cout << *it;
        std::cout << std::endl;
        
        
	std::cout << "Service out!" << std::endl;
}
