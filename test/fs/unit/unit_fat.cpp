#include <common.cxx>
#include <fs/disk.hpp>
#include <fs/memdisk.hpp>
#include <util/sha1.hpp>
#include <unistd.h>
using namespace fs;

static MemDisk* mdisk = nullptr;
static Disk_ptr disk = nullptr;

CASE("Prepare custom memdisk")
{
  const char* rootp(getenv("INCLUDEOS_SRC"));
  std::string path="memdisk.fat";
  if (access(path.c_str(),F_OK) == -1)
  {
    if (rootp == nullptr) path = "..";
    else path = std::string(rootp) + "/test";
    path += "/memdisk.fat";
  }
  auto* fp = fopen(path.c_str(), "rb");
  EXPECT(fp != nullptr);
  fseek(fp, 0L, SEEK_END);
  long int size = ftell(fp);
  rewind(fp);
  // read file into buffer
  char* buffer = new char[size];
  EXPECT(buffer);
  size_t res = fread(buffer, size, 1, fp);
  EXPECT(res == 1);
  // create memdisk
  mdisk = new MemDisk(buffer, buffer + size);
  EXPECT(mdisk);
}

CASE("Initialize FAT fs")
{
  disk = std::make_shared<Disk> (*mdisk);
  disk->init_fs(
    [&lest_env] (auto err, File_system& fs)
    {
      EXPECT(!err);
    });
}

CASE("List root directory")
{
  auto& fs = disk->fs();
  auto res = fs.ls("/");
  // validate root directory
  EXPECT(!res.error);
  EXPECT(res.entries->size() == 8);

  // validate each entry
  auto& ents = *res.entries;
  EXPECT(ents.at(0).name() == ".");
  EXPECT(ents.at(1).name() == "..");
  EXPECT(ents.at(2).name() == "folder");
  EXPECT(ents.at(3).name() == "test.pem");
  EXPECT(ents.at(4).name() == "test.der");
  EXPECT(ents.at(5).name() == "build.sh");
  EXPECT(ents.at(6).name() == "server.key");
  EXPECT(ents.at(7).name() == "test.key");
}

CASE("Validate /folder")
{
  auto& fs = disk->fs();
  // validate folder sync
  auto res = fs.ls("/folder/");
  EXPECT(!res.error);
  EXPECT(res.entries->size() == 3);

  // validate each entry
  auto& ents = *res.entries;
  EXPECT(ents.at(0).name() == ".");
  EXPECT(ents.at(1).name() == "..");
  EXPECT(ents.at(2).name() == "file.txt");

  // validate folder async
  fs.ls("/folder",
  [&lest_env] (auto error, Dirvec_ptr dirvec)
  {
    EXPECT(!error);
    auto& ents = *dirvec;
    EXPECT(ents.at(0).name() == ".");
    EXPECT(ents.at(1).name() == "..");
    EXPECT(ents.at(2).name() == "file.txt");
  });
}

CASE("Validate /folder using dirent")
{
  auto& fs = disk->fs();
  // get dirent by using stat()
  auto ent = fs.stat("/folder");
  EXPECT(ent.is_valid());
  EXPECT(ent.is_dir());

  // validate folder sync using dirent
  auto res = fs.ls(ent);
  EXPECT(!res.error);
  EXPECT(res.entries->size() == 3);

  // validate each entry
  auto& ents = *res.entries;
  EXPECT(ents.at(0).name() == ".");
  EXPECT(ents.at(1).name() == "..");
  EXPECT(ents.at(2).name() == "file.txt");

  // validate folder async using dirent
  fs.ls(ent,
  [&lest_env] (auto error, Dirvec_ptr dirvec)
  {
    EXPECT(!error);
    auto& ents = *dirvec;
    EXPECT(ents.at(0).name() == ".");
    EXPECT(ents.at(1).name() == "..");
    EXPECT(ents.at(2).name() == "file.txt");
  });
}

CASE("Validate /folder/file.txt")
{
  auto& fs = disk->fs();
  // read and validate file
  auto buffer = fs.read_file("/folder/file.txt");
  EXPECT(buffer.size() == 24);
  SHA1 sha1;
  sha1.update(buffer.data(), buffer.size());
  const std::string hash = sha1.as_hex();

  EXPECT(hash == "de1d4db81aa9344377ccfd49f5a239533d4f4ee3");

  const std::string text((const char*) buffer.data(), buffer.size());
  EXPECT(text == "This file contains text\n");
}

CASE("Stat /folder/file.txt")
{
  auto& fs = disk->fs();
  // get dirent by stat()
  auto ent = fs.stat("/folder/file.txt");
  EXPECT(ent.is_valid());
  EXPECT(ent.is_file());
  EXPECT(ent.size() == 24);

  // read and validate file using dirent
  auto buffer = fs.read(ent, 0, ent.size());
  EXPECT(buffer.size() == 24);

  const std::string text((const char*) buffer.data(), buffer.size());
  EXPECT(text == "This file contains text\n");
}
