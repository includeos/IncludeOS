#include <os>
#include <stdio.h>
#include <cassert>

#include <hal/machine.hpp>
#include <hw/dax_device.hpp>
#include <fs/memdisk.hpp>
#include <fs/disk.hpp>

// Includes std::string internal_banana
#include "banana.ascii"

static std::unique_ptr<fs::MemDisk> memdisk;
static std::shared_ptr<fs::Disk> disk;

const uint64_t SIZE = 1048576;
const std::string shallow_banana{"/banana.txt"};
const std::string deep_banana{"/dir1/dir2/dir3/dir4/dir5/dir6/banana.txt"};

void is_done() {
  static int counter = 0;
  if (++counter == 3) INFO("FAT32","SUCCESS\n");
}

void test2()
{
  INFO("FAT32", "Remounting disk.");

  CHECKSERT(not disk->empty(), "Disk not empty");
  CHECKSERT(disk->dev().size() == SIZE / 512, "Disk size is %llu bytes", SIZE);

  disk->init_fs(disk->MBR,
  [] (fs::error_t err, fs::File_system& fs)
  {
    CHECKSERT(not err, "Filesystem mounted on VBR1");

    fs.stat(shallow_banana,
    [] (fs::error_t err, const fs::Dirent& ent) {
      INFO("FAT32", "Shallow banana");

      CHECKSERT(not err, "Stat %s", shallow_banana.c_str());

      CHECKSERT(ent.is_valid(), "Stat file in root dir");
      CHECKSERT(ent.is_file(), "Entity is file");
      CHECKSERT(!ent.is_dir(), "Entity is not directory");
      CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");
      is_done();
    });

    fs.read_file(shallow_banana,
    [] (fs::error_t err, fs::buffer_t buf)
    {
      INFO("FAT32", "Read file");
      CHECKSERT(not err, "read_file: Read %s asynchronously", shallow_banana.c_str());
      printf("%s\n", internal_banana.c_str());
      std::string banana((const char*) buf->data(), buf->size());
      CHECKSERT(banana == internal_banana, "Correct shallow banana");
      is_done();
    });

    fs.stat(deep_banana,
    [] (fs::error_t err, const fs::Dirent& ent) {
      INFO("FAT32", "Deep banana");
      auto& fs = disk->fs();
      CHECKSERT(not err, "Stat %s", deep_banana.c_str());
      CHECKSERT(ent.is_valid(), "Stat file in deep dir");
      CHECKSERT(ent.is_file(), "Entity is file");
      CHECKSERT(!ent.is_dir(), "Entity is not directory");

      CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");

      // asynch file reading test
      fs.read(ent, 0, ent.size(),
      [] (fs::error_t err, fs::buffer_t buf)
      {
        INFO("FAT32", "Read inside stat");
        CHECKSERT(not err, "read: Read %s asynchronously", deep_banana.c_str());

        std::string banana((const char*) buf->data(), buf->size());
        CHECKSERT(banana == internal_banana, "Correct deep fried banana");
        is_done();
      });
    });

  });
}

void Service::start()
{
  auto& dax_device = os::machine().get<hw::DAX_device>(0);
  const char *start = reinterpret_cast<const char*>(dax_device.start_addr());
  const char *end = start + dax_device.size();

  memdisk = std::make_unique<fs::MemDisk>(start, end);
  disk = std::make_shared<fs::Disk>(*memdisk);

  INFO("FAT32", "Running tests for FAT32");
  CHECKSERT(disk, "VirtioBlk disk created");

  // which means that the disk can't be empty
  CHECKSERT(not disk->empty(), "Disk not empty");
  // verify that the size is indeed N sectors
  CHECKSERT(disk->dev().size() == SIZE / 512, "Disk size is %llu sectors", SIZE / 512);

  // auto-mount filesystem
  disk->init_fs(
  [] (fs::error_t err, fs::File_system& fs)
  {
    CHECKSERT(!err, "Filesystem auto-initializedd");

    std::string fat32_str{"FAT32"};
    CHECKSERT(fs.name() == fat32_str, "Filesystem recognized as FAT32");

    fs.ls("/",
    [] (fs::error_t err, auto ents) {
      CHECKSERT(not err, "Listing root directory");
      CHECKSERT(ents->size() == 4, "Exactly four ents in root dir");

      auto& e = ents->at(3);
      CHECKSERT(e.is_file(), "Ent is a file");
      CHECKSERT(e.name() == "banana.txt", "Ents name is 'banana.txt'");

      test2();
    });
  });
}
