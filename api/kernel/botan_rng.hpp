
#pragma once
#ifndef KERNEL_BOTAN_RNG_HPP
#define KERNEL_BOTAN_RNG_HPP

#include <botan/secmem.h>
#include <kernel/rng.hpp>
#include <chrono>
#include <string>

/**
* An interface to a cryptographic random number generator
*/
class IncludeOS_RNG : public Botan::RandomNumberGenerator
{
public:
    IncludeOS_RNG() = default;
    virtual ~IncludeOS_RNG() = default;

    void randomize(uint8_t output[], size_t length) override
    {
      rng_extract(&output[0], length);
    }

    bool accepts_input() const override
    {
      return false;
    }

    void add_entropy(const uint8_t input[], size_t length) override
    {
      rng_absorb(&input[0], length);
      this->seed_bytes += length;
    }

    std::string name() const override {
      return "IncludeOS RNG";
    }

    void clear() override {
      this->seed_bytes = 0;
    }

    bool is_seeded() const override {
      return true; //this->seed_bytes >= 4096;
    }

    static Botan::RandomNumberGenerator& get() {
      static IncludeOS_RNG rng;
      return rng;
    }
private:
    size_t seed_bytes = 0;
    IncludeOS_RNG(const IncludeOS_RNG& rng) = delete;
    IncludeOS_RNG& operator=(const IncludeOS_RNG& rng) = delete;
};

#endif
