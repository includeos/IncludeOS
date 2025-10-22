// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <fs/path.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("Path can be constructed with specified path")
{
  fs::Path path {"/Users/Bjarne/Documents"s};
  EXPECT(path.empty() == false);
}

CASE("Path can be assigned to with string")
{
  const std::string str {"/usr/local/bin"};
  fs::Path path = str;
  EXPECT(path.empty() == false);
  EXPECT(path.size() == 3u);
}

CASE("Path can be constructed with initializer list")
{
  fs::Path path {"/", "Users", "Bjarne", "Documents"};
  EXPECT(path.to_string() == "/Users/Bjarne/Documents/");
}

CASE("Path by default is constructed as \"/\"")
{
  fs::Path path;
  EXPECT(path.to_string() == "/");
  EXPECT(path.empty() == true);
  EXPECT(path.size() == 0u);
}

CASE("Paths can be checked for equality")
{
  fs::Path src_path {"/etc"};
  fs::Path dest_path {"/etc"};
  EXPECT(src_path == dest_path);
}

CASE("Paths can be checked for inequality")
{
  fs::Path src_path {"/etc"};
  fs::Path dest_path {"/bin"};
  EXPECT(src_path != dest_path);
}

CASE("Path can return number of components")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  EXPECT(path.size() == 3u);
}

CASE("Path can return specific component")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  EXPECT(path[2] == "Documents");
}

CASE("Path does not segfault when requested component is out of range")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  EXPECT_THROWS(auto path_component = path[242]);
}

CASE("Path can tell if it is empty")
{
  fs::Path path {"/Users/Bjarne"};
  EXPECT(path.empty() == false);
}

CASE("Path can return itself as string")
{
  auto str = "/Users/Bjarne/Documents/"s;
  fs::Path path {"/Users/Bjarne/Documents"s};
  EXPECT(path.to_string() == str);
}

CASE("Path::to_string returns /-ending string")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  auto str = path.to_string();
  EXPECT(str.back() == '/');
}

CASE("Path components can be added")
{
  fs::Path path {"/Users/Bjarne/Documents"s};
  path += "Books"s;
  path += "C++"s;
  EXPECT(path.size() == 5u);
}

CASE("Paths can be added")
{
  fs::Path root {"/etc"};
  auto subdirectory_name = "private/logs"s;
  fs::Path path = root + subdirectory_name;
  EXPECT(path.size() == 3u);
}

CASE("Paths can return their parent")
{
  fs::Path path {"/etc/private/logs"s};
  fs::Path path_parent {path.up()};
  EXPECT(path_parent.size() == 2u);
  EXPECT(path_parent == "/etc/private"s);
  fs::Path root {"/"s};
  fs::Path root_parent {root.up()};
  EXPECT(root_parent == "/");
}

CASE("pop_front returns path with first path component removed")
{
  fs::Path path {"/etc/private/logs"};
  auto front = path.pop_front();
  EXPECT(front == "/private/logs"s);
}

CASE("pop_back returns path with last path component removed")
{
  fs::Path path {"/etc/private/logs"};
  auto back = path.pop_back();
  EXPECT(back == "/etc/private");
}

CASE("pop_back expects that path is not empty")
{
  fs::Path path {"/etc/private/logs"};
  EXPECT_NO_THROW(path.pop_back().pop_back().pop_back());
  EXPECT_THROWS(path.pop_back());
}

CASE("pop_front expects that path is not empty")
{
  fs::Path path {"/usr/local/bin"};
  EXPECT_NO_THROW(path.pop_front().pop_front().pop_front());
  EXPECT_THROWS(path.pop_front());
}

CASE("front expects that path is not empty")
{
  fs::Path path {"/dev"};
  EXPECT_NO_THROW(auto front = path.front());
  path.pop_front();
  EXPECT_THROWS(auto front = path.front());
}

CASE("back expects that path is not empty")
{
  fs::Path path {"/Users"};
  EXPECT_NO_THROW(auto back = path.back());
  path.pop_back();
  EXPECT_THROWS(auto back = path.back());
}
