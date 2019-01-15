#include <common.cxx>
#include <fs/fd_compatible.hpp>
#include <posix/fd_map.hpp>
#include <posix/fd.hpp>
#include <unistd.h>

class TestableFD : public FD
{
public:
  TestableFD(int fd)
    : FD{fd}
  {
    
  }
  virtual ~TestableFD() {}

  int close() override
  {
    return 0;
  }
};

class Shit : public FD_compatible
{
public:
  Shit()
  {
    this->open_fd =
      [] () -> FD& {
        return FD_map::_open<TestableFD>();
      };
  }
  virtual ~Shit() {}
};

CASE("Calls into testable FD")
{
  Shit shit{};
  auto& fd = shit.open_fd();
  int tfd = fd.get_id();
  EXPECT(tfd >= 0);
  EXPECT(fd.close() == 0);
  
  EXPECT(fd.is_file() == false);
  EXPECT(fd.is_socket() == false);
  
  EXPECT(fd.read(nullptr, 0) < 0);
  EXPECT(fd.readv(nullptr, 0) < 0);
  EXPECT(fd.write(nullptr, 0) < 0);
  
  //EXPECT_THROWS(FD_map::close())
}
