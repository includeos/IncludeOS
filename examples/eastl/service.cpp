#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

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
	
	std::cout << "Service out!" << std::endl;
}
