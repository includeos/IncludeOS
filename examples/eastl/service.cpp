#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

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
	std::cout << str << " test number: " << 52 << std::endl;
	
	std::cout << "Service out!" << std::endl;
}
