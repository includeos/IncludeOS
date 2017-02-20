// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <string>
#include <vector>
#include <fstream>
#include <pwd.h>
#include <info>
#include <gsl/gsl_assert>

const char* pwd_path = "/etc/passwd";

/* Applications are not allowed to modify the structure to which the return
   values point, nor any storage areas pointed to by pointers within the
   structure. Returned pointers might be invalidated or the structure/
   storage areas might be overwritten by subsequent calls. */
static struct passwd ret;

class Pwd {
public:
  static Pwd& instance() {
    static Pwd pwd;
    return pwd;
  }

  void rewind() {pos_ = 0;}

  void close() {open_ = false;}

  struct passwd* find(const char* name) {
    Expects(name != nullptr);
    if (!open_) {
      read();
    }
    const auto it = std::find_if(begin(entries_), end(entries_), [name](const auto& entry){
      return (strcmp(name, entry.c_str()) == 0);
    });
    if (it == end(entries_)) {
      return nullptr;
    }
    extract(*it);
    return &ret;
  }

  struct passwd* find(uid_t uid) {
    char buf[16];
    snprintf(buf, 16, "%d", uid);
    if (!open_) {
      read();
    }
    const auto it = std::find_if(begin(entries_), end(entries_), [buf](const auto& entry){
      char* start = const_cast<char*>(entry.c_str());
      size_t pw_pos = entry.find('\0') + 1;
      size_t uid_pos = entry.find('\0', pw_pos) + 1;
      return (strcmp(buf, start + uid_pos) == 0);
    });
    if (it == end(entries_)) {
      return nullptr;
    }
    extract(*it);
    return &ret;
  }

  struct passwd* next() noexcept {
    if (!open_) {
      read();
    }
    if (pos_ >= entries_.size()) {
      return nullptr;
    }
    try {
      const auto& ent = entries_[pos_];
      int field_seps = std::count(std::begin(ent), std::end(ent), '\0');
      if (field_seps != 6) {
        INFO("pwd", "not a valid passwd file entry");
        return nullptr;
      }
      extract(ent);
      ++pos_;
      return &ret;
    }
    catch (...) {
      INFO("pwd", "error parsing pwd entry");
      return nullptr;
    }
  }

  void extract(const std::string& ent) {
    ret.pw_name = const_cast<char*>(ent.c_str());
    size_t pw_pos = ent.find('\0') + 1;
    size_t uid_pos = ent.find('\0', pw_pos) + 1;
    size_t gid_pos = ent.find('\0', uid_pos) + 1;
    size_t info_pos = ent.find('\0', gid_pos) + 1;
    size_t dir_pos = ent.find('\0', info_pos) + 1;
    size_t shell_pos = ent.find('\0', dir_pos) + 1;
    ret.pw_uid = std::stol(ret.pw_name + uid_pos);
    ret.pw_gid = std::stol(ret.pw_name + gid_pos);
    ret.pw_dir = ret.pw_name + dir_pos;
    ret.pw_shell = ret.pw_name + shell_pos;
  }

  void print() {
    printf("  instance @%p\n", this);
    printf("  open: %d\n", open_);
    printf("  pos: %d\n", pos_);
    printf("  entries: %d\n", entries_.size());
  }

private:
  Pwd() : open_ {false}, pos_ {0} {}

  void read() {
    entries_.clear();
    std::ifstream is(pwd_path);
    std::string line;
    while (std::getline(is, line)) {
      std::replace(std::begin(line), std::end(line), ':', '\0');
      entries_.push_back(line);
    }
    rewind();
    open_ = true;
  }
  bool open_;
  size_t pos_;
  std::vector<std::string> entries_;
};

void endpwent(void) {
  Pwd& pwd = Pwd::instance();
  pwd.close();
}

void setpwent(void) {
  Pwd& pwd = Pwd::instance();
  pwd.rewind();
}

struct passwd *getpwent(void) {
  Pwd& pwd = Pwd::instance();
  return pwd.next();
}

struct passwd *getpwnam(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  Pwd& pwd = Pwd::instance();
  return pwd.find(name);
}

struct passwd *getpwuid(uid_t uid) {
  Pwd& pwd = Pwd::instance();
  return pwd.find(uid);
}
