#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include <functional>

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

#include <tuple.hpp>
#include <utility>

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

#include <signal>

class TestSignal
{
public:
	signal<void()> test;
};

#include <delegate>

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
	std::vector<int> vec;
	vec.resize(500);
	vec[0] = 500;
	vec[499] = 500;
	
	assert(vec[499] == 500);
	vec.resize(1);
	assert(vec[0] == 500);
	
	std::map<int, int> testMap;
	
	eastl::string str("test string");
	std::cout << str << " int: " << 52 << " long: " << 52L << std::endl;
	std::cout << "short: " << (short)52 << " char: " << 'C' << std::endl;
	std::cout << "pointer: " << &str << std::endl;
	
	std::cout << "class: " << testStream << std::endl;
	
	std::cin >> str;
	std::cout << "You wrote: " << str << std::endl;
	
	std::function<void()> 
	testLambda = []
	{
		std::cout << "std::function<void()> testLambda" << std::endl;
	};
	testLambda();
	
	std::function<void()> test = testFunction;
	
	std::cout << "calling std::function:" << std::endl;
	test();
	
	TestSignal testSignal;
	testSignal.test.connect(
	[] {
		std::cout << "std::signal lambda test" << std::endl;
	});
	testSignal.test.connect(testFunction);
	testSignal.test.connect(testFunction);
	
	std::cout << "emitting signal:" << std::endl;
	testSignal.test.emit();
	
	std::vector<std::function<void()>> fvec;
	fvec.push_back(testFunction);
	fvec.push_back(testFunction);
	
	std::cout << "vector[0] == " << &fvec[0] << std::endl;
	std::cout << "vector[1] == " << &fvec[1] << std::endl;
	
	delegate<void()> delgStatic = testFunction;
	delegate<void()> delgDynamic = test;
	
	std::cout << "calling delegates:" << std::endl;
	delgStatic();
	delgDynamic();
	
	std::cout << "Service out!" << std::endl;
}
