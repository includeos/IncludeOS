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

#include <iostream>
#include <fs/vfs.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

class Person {
public:
  explicit Person(std::string name, int age) : name_ {name}, age_ {age} {};
  bool isBjarne() const { return (name_ == "Bjarne") ? true : false; }
private:
  std::string name_;
  int age_;
};

CASE("VFS entries can have names")
{
  fs::VFS_entry e("entry", "description");
  EXPECT(e.name() == "entry");
}

CASE("VFS entries can have descriptions")
{
  fs::VFS_entry e("entry", "description");
  EXPECT(e.desc() == "description");
}

CASE("VFS entries can return type info as string")
{
  fs::VFS_entry e("entry", "description");
  auto info = e.type_name();
  EXPECT(info.length() > 0u);
  auto pos = info.find("nullptr");
  EXPECT(pos != std::string::npos);
}

CASE("maximum type_name() length can be specified")
{
  fs::VFS_entry e("some_entry", "with_description");
  auto info = e.type_name(3);
  EXPECT(info.length() == 3u);
}

CASE("VFS entries can return their number of children")
{
  fs::VFS_entry e("entry", "description");
  EXPECT(e.child_count() == 0);
}

CASE("VFS entries can contain arbitrary objects")
{
  Person p {"Bjarne", 65};
  fs::VFS_entry e(p, "creator", "duh");
  auto info = e.type_name();
  EXPECT(info != "decltype(nullptr)");
  EXPECT(info == "Person");
  Person bjarne = e.obj<Person>();
  EXPECT(bjarne.isBjarne() == true);
  EXPECT_THROWS(e.obj<std::string>());
  // constness is checked
  const Person q {"Dennis", 70};
  fs::VFS_entry f(q, "inspiration", "duh^2");
  EXPECT_THROWS_AS(Person z = f.obj<Person>(), fs::Err_bad_cast);
  EXPECT_NO_THROW(const Person z = f.obj<const Person>());
}

CASE("VFS can mount entries in a tree")
{
  // tree is initially empty
  EXPECT(fs::VFS::root().child_count() == 0);

  // mount a char
  char a {'a'};
  fs::mount("/mnt/chars/a", a, "the letter a");
  EXPECT(fs::VFS::root().child_count() == 1);

  // cannot mount if already occupied
  char b {'b'};
  EXPECT_THROWS(fs::mount("/mnt/chars/a", b, "the letter b"));
  EXPECT_NO_THROW(fs::mount("/mnt/chars/b", b, "the letter c"));

  // mount another
  char c {'c'};
  EXPECT_NO_THROW(fs::mount("/mnt/chars/c", c, "the letter c"));

  // mount direct descendants of root
  char d {'d'};
  char e {'e'};
  EXPECT_NO_THROW(fs::mount("/d", d, "the letter d"));
  EXPECT_NO_THROW(fs::mount("/e", e, "the letter e"));
  EXPECT(fs::VFS::root().child_count() == 3);

  // get mounted objects of correct type
  char our_char;
  EXPECT_THROWS(auto dir = fs::get<fs::Dirent>("/mnt/chars/c"));
  EXPECT_NO_THROW(our_char = fs::get<char>("/mnt/chars/c"));
}
