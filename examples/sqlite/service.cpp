// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>

#include <sqlite>

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
	/////////////////////////////////////////////////////////////////////////////
	//// SQLite in-memory database
	/////////////////////////////////////////////////////////////////////////////
	
	using namespace database;
	SQLite sqlite;
	
	if (!sqlite.good()) return;
	
	// create in-memory table
	sqlite.exec(
		"CREATE TABLE COMPANY("
		"ID INT PRIMARY KEY     NOT NULL,"
		"NAME           TEXT    NOT NULL)");
	
	// insert stuff into table
	sqlite.exec(
		"INSERT INTO company VALUES (1, 'hei');"
		"INSERT INTO company VALUES (2, 'test')");
	
	// get results as vector of std::pair
	std::vector< std::pair<int, std::string> > results;
	
	sqlite.select("SELECT id, name FROM company",
	[&results] (SQResult& result)
	{
		results.emplace_back( result.getInt(0), result.getString(1) );
	});
	
	std::cout << "Total: " << results.size() << std::endl;
	for (auto p : results)
	{
		std::cout << "Result: " << p.first << " => " << p.second << std::endl;
	}
	
	std::cout << "Service out!" << std::endl;
}
