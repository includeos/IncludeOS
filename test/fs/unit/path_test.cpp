
#include <fs/path.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("Path can be constructed with specified path")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  EXPECT(path.empty() == false);
}

CASE("Path can be constructed with initializer list")
{
  fs::Path path {"/", "Users", "Bjarne", "Documents"};
  EXPECT(path.to_string() == "/Users/Bjarne/Documents/");
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
  EXPECT(path.size() == 3);
}

CASE("Path can return specific component")
{
  fs::Path path {"/Users/Bjarne/Documents"};
  EXPECT(path[2] == "Documents");
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
  EXPECT(path.size() == 5);
}

CASE("Paths can be added")
{
  fs::Path root {"/etc"};
  auto subdirectory_name = "private/logs"s;
  fs::Path path = root + subdirectory_name;
  EXPECT(path.size() == 3);
}

CASE("Paths can return their parent")
{
  fs::Path path {"/etc/private/logs"s};
  fs::Path parent {path.up()};
  EXPECT(parent.size() == 2);
}
