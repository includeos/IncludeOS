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
  auto info {e.type_name()};
  EXPECT(info.length() > 0);
  EXPECT(info == "decltype(nullptr)");
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
}
