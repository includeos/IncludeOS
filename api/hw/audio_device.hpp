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

#pragma once
#ifndef HW_AUDIO_DEVICE_HPP
#define HW_AUDIO_DEVICE_HPP

#include <cstdint>
#include <delegate>
#include <memory>
#include <vector>

namespace hw
{
  class Audio_device {
  public:
    enum mixer_value_t {
      MASTER_VOLUME,
      PCM_OUT
    };
    /**
     * Method to get the type of device
     *
     * @return The type of device as a C-String
     */
    static const char* device_type() noexcept
    { return "Audio device"; }

    /**
     * Method to get the name of the device
     *
     * @return The name of the device as a std::string
     */
    virtual std::string device_name() const = 0;

    /**
     * Get the human-readable name of this device's controller
     *
     * @return The human-readable name of this device's controller
     */
    virtual const char* driver_name() const noexcept = 0;

    /* Get & set hardware mixer values */
    virtual void     set_mixer(mixer_value_t, uint32_t) = 0;
    virtual uint32_t get_mixer(mixer_value_t) = 0;

    /**
     * Method to deactivate the block device
     */
    virtual void deactivate() = 0;

    virtual ~Audio_device() noexcept = default;
  }; //< Audio_device
} //< hw

#endif
