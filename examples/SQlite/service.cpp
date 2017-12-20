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

#include <service>
#include <cassert>
#include <iostream>
#include <sl3/database.hpp>

void Service::start()
{
  using namespace sl3;
  // define a db
  Database db(":memory:");
  // run commands against the db
  db.execute("CREATE TABLE tbl(f1 INTEGER, f2 TEXT, f3 REAL);");
  // create a command with parameters
  auto cmd = db.prepare("INSERT INTO tbl (f1, f2, f3) VALUES (?,?,?);");

  //add some data
  cmd.execute(parameters(1, "one", 1.1));
  cmd.execute(parameters(2, "two", 2.2));

  // access the data
  Dataset ds = db.select("SELECT * FROM tbl;");

  // Dataset is a container
  assert(ds.size()==2);

  // Dataset row is a container
  auto row = ds[0] ;
  assert(row.size()==3);
  assert ( row[0].type() == Type::Int ) ;
  assert ( row[1].type() == Type::Text ) ;
  assert ( row[2].type() == Type::Real ) ;

  // of course there is also iterator access
  for(auto& row  :ds) {

      for (auto& field : row) {
          std::cout << field << " " ;
      }
      std::cout << std::endl;
  }
}
