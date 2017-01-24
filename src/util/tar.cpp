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

#include <tar>

#include <cstdlib>  // strtol

using namespace tar;

// -------------------- Element --------------------

long int Element::size() const noexcept {
  // Size of element's size in bytes
  char* pEnd;
  long int filesize;
  filesize = strtol(header_.size, &pEnd, 8); // header_.size is octal
  return filesize;
}

bool Element::is_tar_gz() const noexcept {
  const std::string n = name();
  return n.size() > 7 and n.substr(n.size() - 7) == ".tar.gz";
}

// ----------------------- Tar -----------------------

const Element& Tar::element(const std::string& path) const {
  for (auto& element : elements_) {
    if (element.name() == path) {
      return element;
    }
  }

  throw Tar_exception(std::string{"Path " + path + " doesn't exist. Folder names should have a trailing /"});
}

std::vector<std::string> Tar::element_names() const {
  std::vector<std::string> element_names;

  for (auto& element : elements_)
    element_names.push_back(element.name());

  return element_names;
}

// -------------------- Reader --------------------

unsigned int Reader::decompressed_length(const uint8_t* data, size_t size) const {
  unsigned int dlen = data[size - 1];

  for (int i = 2; i <= 4; i++)
    dlen = DECOMPRESSION_SIZE*dlen + data[size - i];

  return dlen;
}

Tar Reader::read_uncompressed(const uint8_t* data, size_t size) {
  if (size % SECTOR_SIZE not_eq 0)
    throw Tar_exception("Invalid size of tar file");

  Tar tar;

  // Go through the whole tar file block by block
  for (size_t i = 0; i < size; i += SECTOR_SIZE) {
    Tar_header* header = (Tar_header*) ((const char*) data + i);
    Element element{*header};

    if (element.name().empty()) {
      // Empty header name - continue
      continue;
    }

    // Check if this is a directory or not (typeflag) (directories have no content)
    // If typeflag not set -> is not a directory and has content
    if (not element.typeflag_is_set() or not element.is_dir()) {
      i += SECTOR_SIZE; // move past header
      element.set_content_start(data + i);
      i += (SECTOR_SIZE * (element.num_content_blocks() - 1));  // move to the end of the element
    }

    tar.add_element(element);
  }

  return tar;
}
