// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#pragma once

#ifndef MENDER_ARTIFACT_HPP
#define MENDER_ARTIFACT_HPP

#include "common.hpp"
#include <tar>

namespace mender {

  class Artifact {

    enum Index {
      VERSION = 0,
      HEADER = 1,
      DATA_START = 2
    };

  public:
    Artifact(byte_seq data);

    /**
     * @brief      Parse according to the mender file structure
     */
    void parse();

    int version() const { return version_; }
    const std::string& format() const { return format_; }
    const std::string& name() const { return name_; }
    const tar::Tar_data& get_update_blob(int index = 0) const
    { return updates_.at(index); }

    void parse_version(const uint8_t* version);

    void parse_header_info(const uint8_t* header_info);

  private:
    byte_seq data_;
    tar::Tar artifact_;               // version, header.tar.gz, data/0000.tar.gz, data/0001.tar.gz, etc.
    tar::Tar_data header_;            // unpacked header.tar.gz
    std::vector<tar::Tar_data> updates_;   // unpacked data/0000.tar.gz, data/0001.tar.gz, etc.

    int version_;                     // version specified in version file
    std::string format_;              // format specified in version file
    std::string name_;                // name of artifact as specified in header_info (first element inside header.tar.gz)

  }; // < class Artifact

} // < namespace mender

#endif
