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
//1 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define OS_TERMINATE_ON_CONTRACT_VIOLATION

#include <os>
#include <hal/machine.hpp>
#include <fs/vfs.hpp>
#include <memdisk>

struct Base {
  virtual std::string name() {
    return "Base";
  }
};

struct Thing1 : public Base {

  std::string name() override  {
    return "Thing 1";
  }

  double secret() { return secret_; }

private:
  double secret_ = 0.97;

};


struct Thing2 {

  std::string name() {
    return "Thing 2";
  }

  int secret(){
    return secret_;
  }

private:
  int secret_ = 42;
};


fs::File_system& memdisk() {
  static auto disk = fs::shared_memdisk();

  if (not disk->fs_ready())
  {
    disk->init_fs([](fs::error_t err, auto&) {
        if (err) os::panic("ERROR MOUNTING DISK\n");
      });
  }
  return disk->fs();
}


void Service::start(const std::string&)
{
  INFO("VFS_test", "Starting virtual filesystem test");

  Thing1 thing1;
  Thing2 thing2;

  fs::VFS_entry obj1 { thing1, "Thing 1", "The first thing" };
  fs::VFS_entry obj2 { thing2, "Thing 2", "" };

  std::cout << "Entry 1: " << obj1.name() << "\n";
  std::cout << "Entry 2: " << obj2.name() << "\n";


  try {

    // THROWS as expected : type_id doesn't match
    auto&& one_as_two = obj1.obj<Thing2>();
    assert(false && "Error: Illegal type conversion detected");

    // Supress unused warning
    std::cout << one_as_two.secret() << "\n";

  }catch(const std::runtime_error& e){

    CHECKSERT(true, "Exception thrown when fetching Thing 1 as Thing 2: %s", e.what());
  }


  try {

    // THROWS even though thing1 inherits base. Typeid does not account for inheritance
    auto&& one_as_base = obj1.obj<Base>();

    // Supress unused warning
    printf("Impossible: %s \n", one_as_base.name().c_str());

  }catch(const std::runtime_error& e){
    CHECKSERT(true, "Exception thrown when fetching Thing 1 as Base: %s", e.what());
  }

  auto&& one_as_one = Thing1(obj1);

  CHECKSERT(true, "obj1 successfully converted to correct type Thing1 using cast operator");

  auto&& two_as_two = obj2.obj<Thing2>();
  CHECKSERT(true, "obj2 successfully converted to correct type Thing2 using getter");


  CHECKSERT(one_as_one.secret() == 0.97, "Thing1's secret is correct");
  CHECKSERT(two_as_two.secret() == 42, "Thing2's secret is correct");


  // Store and retreive a lambda as a VFS entry
  bool lambda_called = false;
  delegate<void()> func1 = [&lambda_called]{
    lambda_called = true;
  };

  fs::VFS_entry func_entry{func1, "Function 1", "Sets a bool to true"};
  func_entry.obj<delegate<void()>>()();

  CHECKSERT(lambda_called, "The lambda entry was called");

  fs::Path p{"usr","local","bin"};
  std::cout << "Path from init-list: " << p.to_string() << "\n";

  p.pop_back();
  std::cout << "Path from init-list: " << p.to_string() << "\n";

  p.pop_front();
  std::cout << "Path from init-list: " << p.to_string() << "\n";

  fs::Path p2{"/usr/local/bin"};
  std::cout << "Path tokens (" << p2.to_string() << ")\n";
  for (auto token : p2) {
    std::cout << token << "\n";
  }


  /** Mouting anything **/
  fs::mount("/proc/functions/func1", func1, "User function");

  auto func2 = []{ printf("I'print at random: %i \n", rand()); };
  fs::mount("/proc/functions/func2", func2, "User function");

  CHECKSERT(fs::VFS::root().child_count() == 1, "Mount added one element to root");

  auto func3 = []{ printf("I'm user function 3"); };


  int i = 42;
  fs::mount("/proc/integers/i", i, "Holds the meaning of life etc.");

  int j = 43;
  fs::mount("/proc/integers/j", j, "A prime");

  auto f = 3.14f;
  fs::mount("/proc/floats/pi", f, "Pi-ish");

  const auto t = 6.28f;
  fs::mount("/proc/floats/tau", t, "Twice the pie");

  /** PRINT **/
  fs::print_tree();

  auto&& int_i = fs::get<int>("/proc/integers/i");

  CHECKSERT(int_i == 42, "Typed object retreival works");

  int_i = 48;
  auto&& int_i2 = fs::get<int>("/proc/integers/i");
  CHECKSERT(int_i2 == 48, "Non-const mounted objects can be modified.");

  auto&& tau = fs::get<const float>("/proc/floats/tau");
  CHECKSERT(tau == 6.28f, "Retreiving const-mounted object as const works");


  /** Mounting exceptions */

  // Duplicate mount
  try {
    fs::mount("/proc/functions/", func3,"Prints something");
    FAIL("Mounting to a mount point in use should throw");
  } catch(const std::runtime_error& e){
    CHECKSERT(true, "Trying to mount to the same mount point again throws");
  }

  // Non-creating mount
  try {
    std::string plan("Make America Great Again");
    fs::mount<false>("/users/donald/master_plan.txt", plan, "Donalds plan");
    FAIL("Mounting to a non-existing mount point with no create flag should throw");
  } catch (const fs::Err_mountpoint_invalid& e) {
    CHECKSERT(e.what(), "Trying to mount to non-existing path with no create flag throws");
  }

  // Mounting / retreiving is const-safe
  try {
    auto&& tau = fs::get<float>("/proc/floats/tau");
    FAIL("Retreiving a const mounted object as non-const should throw");
    assert(tau);

  } catch (const fs::Err_bad_cast& e){
    CHECKSERT(true, "Trying to retreive a const-mounted object as non-const throws");
  }

  // Retreiving non-mounted objects
  try {
    auto&& string = fs::get<std::string>("/users/donald/master_plan.txt");
    FAIL("Retreiving a non mounted object should throw");
    assert(string.empty());

  } catch (const fs::Err_not_found& e){
    CHECKSERT(true, "Trying to retreive a non existing object throws");
  }


  /** Mounting a memdisk / file system **/
  auto file1 = memdisk().stat("/users/alfred/etc/ip4/config.txt");
  auto file2 = memdisk().stat("/users/ingve/etc/ip4/config.txt");


  Expects(file1.read() == "10.0.0.42\n");
  Expects(file2.read() == "10.0.0.43\n");

  // Mount the whole memdisk
  fs::mount("/filesys/memdisk", memdisk(), "In-memory file system");

  const char* memdisk_dir = "/users/alfred/etc";
  const char* vfs_dir = "/suspicious_users/alfred/etc";
  auto etc = memdisk().stat(memdisk_dir);

  INFO("VFS_test","Mountning memdisk dir '%s' on vfs '%s' \n",
         memdisk_dir, vfs_dir);

  // Mount a part of the memdisk filesystem into vfs.
  fs::mount(vfs_dir, etc , "Alfreds config");

  auto etc_ingve = memdisk().stat("/usres/ingve/etc");
  fs::mount("/trusted_users/ingve/etc", etc_ingve , "Ingves config");


  /** PRINT **/
  fs::print_tree();

  auto etc_folder =  fs::get<fs::Dirent>("suspicious_users/alfred/etc");

  const char* forward_dir = "ip4/config.txt";
  auto ip4_config = etc_folder.stat_sync({"ip4", "config.txt"});
  auto ip4_config2 = etc_folder.stat_sync(forward_dir);

  INFO("VFS_test","stat and read relative path '%s' from mounted dirent ",
       forward_dir);

  CHECKSERT(ip4_config.read() == "10.0.0.42\n",
            "You can stat a file relative to a dirent");

  CHECKSERT(ip4_config.read() == ip4_config2.read(),
            "stat using const char* and initializer list both works");

  INFO("VFS_test", "File content: %s", ip4_config.read().c_str());

  auto list = ip4_config.ls();

  for (auto entry : list)
    std::cout << entry.name() << "\n";

  const char* vfs_path = "/suspicious_users/alfred/etc/ip4/config.txt";
  auto alfreds_config = fs::VFS::stat_sync(vfs_path);

  INFO("VFS_test", "Trying to stat vfs path '%s' directly", vfs_path);
  auto alfreds_config2 = fs::stat_sync(vfs_path);

  INFO("VFS_test", "File content: %s", alfreds_config2.read().c_str());

  CHECKSERT(alfreds_config2.read() == "10.0.0.42\n",
            "File content verified");

  CHECKSERT(alfreds_config2.read() == alfreds_config.read(),
            "You can directly stat and read objects via mounted dirents");


  /** Locate all disk drives **/
  INFO("VFS_test", "Mounting all disk drives: ");

  for (auto& drv : os::machine().get<hw::Block_device>()) {
    INFO("VFS_test", "Drive name: %s \n", drv.get().device_name().c_str());
    fs::mount({"dev", drv.get().device_name()}, drv.get(), drv.get().driver_name());
  }

  auto& disk0 = fs::get<fs::Disk_ptr>("/dev/vblk0");
  auto& disk1 = fs::get<fs::Disk_ptr>("/dev/vblk1");

  // TODO: LS over children in VFS_entries...

  CHECKSERT(std::string(disk0->name()) == "vblk0", "First drive mounted successfully");
  CHECKSERT(std::string(disk1->name()) == "vblk1", "Second drive mounted successfully");


  // Mount a directory on a given disk, on a VFS path
  try {
    fs::VFS::mount({"/overlord/pictures/"}, disk0->device_id(), {"/pictures/"}, "Image of our lord commander", [](auto){
        FAIL("Mounting a directory from an uninitialized disk should not call user callback");
      });
    FAIL("Mounting a directory from an uninitialized disk should throw");
  } catch(const fs::Disk::Err_not_mounted& e){
    CHECKSERT(true, "Mounting a directory on an uninitialized disk throws");
  }

  auto my_disk = disk1;

  // initializing a file system
  my_disk->init_fs(
  [my_disk](auto err, auto& fs)
  {
      if (err) {
        INFO("VFS_test", "Error mounting disk: %s \n", err.to_string().c_str());
        return;
      }

      fs.ls("/", [my_disk](auto err, auto dirvec){

          if (err) {
            INFO("VFS_test", "ls on disk %s failed", my_disk->name().c_str());
            return;
          }

          INFO("VFS_test", "ls on disk %s:", my_disk->name().c_str());
          for (auto ent : *dirvec) {
            std::cout << "\t|-" << ent.name() << "\n";
          }

        });

      INFO("VFS_test", "Filesystem mounted on %s", my_disk->name().c_str());
      fs::VFS::mount({"/overlord/pictures/"}, my_disk->device_id(), {"/pictures/"}, "Images of our lord commander", [my_disk](auto err){

          if (err)
            os::panic("Error mounting dirent from disk on VFS path");

          INFO("VFS_test", "Reading content of newly mounted folder");


          // Actual user interface for VFS
          fs::stat("/overlord/pictures/profile.txt", [](auto err, auto dir){

              if (err)
                os::panic("Error stating file \n");

              INFO("VFS_test", "File found. Reading \n");

              dir.read([](auto err, auto buf){

                  if (err)
                    os::panic("Errror reading file contents \n");

                  std::string res((char*)buf->data(), buf->size());

                  std::cout << "Our overlords likeness: \n\n " << res;
                  INFO("VFS_test", "SUCCESS");
                });
            });

          /** PRINT **/
          fs::print_tree();

        });
    });
}
