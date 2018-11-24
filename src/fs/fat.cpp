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

#include <fs/fat.hpp>
#include <fs/fat_internal.hpp>

#include <fs/mbr.hpp>
#include <cassert>
#include <cstring>
#include <locale>
#include <os.hpp> // for panic()

#include <info>
#include <common>

//#define FS_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define FS_PRINT(fmt, ...)  /** **/

inline std::string trim_right_copy(
   const std::string& s,
   const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

namespace fs
{
  FAT::FAT(hw::Block_device& dev)
    : device(dev) {
    //
  }

  void FAT::init(const void* base_sector) {

    // assume its the master boot record for now
    auto* mbr = (MBR::mbr*) base_sector;

    MBR::BPB* bpb = mbr->bpb();
    this->sector_size = bpb->bytes_per_sector;

    if (UNLIKELY(this->sector_size < 512)) {
      fprintf(stderr,
          "Invalid sector size (%u) for FAT32 partition\n", sector_size);
      fprintf(stderr,
          "Are you initializing the correct partition?\n");
      os::panic("FAT32: Invalid sector size");
    }

    // Let's begin our incantation
    // To drive out the demons of old DOS we have to read some PBP values
    FS_PRINT("Bytes per sector: \t%u\n", bpb->bytes_per_sector);
    FS_PRINT("Sectors per cluster: \t%u\n", bpb->sectors_per_cluster);
    FS_PRINT("Reserved sectors: \t%u\n", bpb->reserved_sectors);
    FS_PRINT("Number of FATs: \t%u\n", bpb->fa_tables);

    FS_PRINT("Small sectors (FAT16): \t%u\n", bpb->small_sectors);

    FS_PRINT("Sectors per FAT: \t%u\n", bpb->sectors_per_fat);
    FS_PRINT("Sectors per Track: \t%u\n", bpb->sectors_per_track);
    FS_PRINT("Number of Heads: \t%u\n", bpb->num_heads);
    FS_PRINT("Hidden sectors: \t%u\n", bpb->hidden_sectors);
    FS_PRINT("Large sectors: \t%u\n", bpb->large_sectors);
    FS_PRINT("Disk number: \t0x%x\n", bpb->disk_number);
    FS_PRINT("Signature: \t0x%x\n", bpb->signature);

    FS_PRINT("System ID: \t%.8s\n", bpb->system_id);

    // sector count
    if (bpb->small_sectors)
      this->sectors  = bpb->small_sectors;
    else
      this->sectors  = bpb->large_sectors;

    // sectors per FAT (not sure about the rule here)
    this->sectors_per_fat = bpb->sectors_per_fat;
    if (this->sectors_per_fat == 0)
      this->sectors_per_fat = *(uint32_t*) &mbr->boot[25];

    // root dir sectors from root entries
    this->root_dir_sectors = ((bpb->root_entries * 32) + (sector_size - 1)) / sector_size;

    // calculate index of first data sector
    this->data_index = bpb->reserved_sectors + (bpb->fa_tables * this->sectors_per_fat) + this->root_dir_sectors;
    FS_PRINT("First data sector: %u\n", this->data_index);

    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;
    FS_PRINT("Reserved sectors: %u\n", this->reserved);

    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;
    FS_PRINT("Sectors per cluster: %u\n", this->sectors_per_cluster);

    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;
    FS_PRINT("Data sectors: %u\n", this->data_sectors);

    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;
    FS_PRINT("Total clusters: %u\n", this->clusters);

    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085) {
      this->fat_type = FAT::T_FAT12;
      this->root_cluster = 2;
      FS_PRINT("The image is type FAT12, with %u clusters\n", this->clusters);
    }
    else if (this->clusters < 65525) {
      this->fat_type = FAT::T_FAT16;
      this->root_cluster = 2;
      FS_PRINT("The image is type FAT16, with %u clusters\n", this->clusters);
    }
    else {
      this->fat_type = FAT::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      this->root_cluster = 2;
      FS_PRINT("The image is type FAT32, with %u clusters\n", this->clusters);
      FS_PRINT("Root dir entries: %u clusters\n", bpb->root_entries);
      //assert(bpb->root_entries == 0);
      //this->root_dir_sectors = 0;
      //this->data_index = bpb->reserved_sectors + bpb->fa_tables * this->sectors_per_fat;
    }
    FS_PRINT("Root cluster index: %u (sector %u)\n", this->root_cluster, cl_to_sector(root_cluster));
    FS_PRINT("System ID: %.8s\n", bpb->system_id);
  }

  void FAT::init(uint64_t base, uint64_t size, on_init_func on_init)
  {
    this->lba_base = base;
    this->lba_size = size;

    // read Partition block
    device.read(
      base,
      hw::Block_device::on_read_func::make_packed(
      [this, on_init] (buffer_t data)
      {
        auto* mbr = (MBR::mbr*) data->data();
        if (mbr == nullptr) {
          on_init({ error_t::E_IO, "Could not read MBR" }, *this);
          return;
        }

        // verify image signature
        FS_PRINT("OEM name: \t%s\n", mbr->oem_name);
        FS_PRINT("MBR signature: \t0x%x\n", mbr->magic);
        if (UNLIKELY(mbr->magic != 0xAA55)) {
          on_init({ error_t::E_MNT, "Missing or invalid MBR signature" }, *this);
          return;
        }

        // initialize FAT16 or FAT32 filesystem
        init(mbr);

        // determine which FAT version is initialized
        switch (this->fat_type) {
        case FAT::T_FAT12:
          INFO("FAT", "Initializing FAT12 filesystem");
          break;
        case FAT::T_FAT16:
          INFO("FAT", "Initializing FAT16 filesystem");
          break;
        case FAT::T_FAT32:
          INFO("FAT", "Initializing FAT32 filesystem");
          break;
        }
        INFO2("[ofs=%u  size=%u (%u bytes)]\n",
              this->lba_base, this->lba_size, this->lba_size * 512);

        // on_init callback
        on_init(no_error, *this);
      })
    );
  }

  bool FAT::int_dirent(uint32_t sector, const void* data, dirvector& dirents) const
  {
    auto* root = (cl_dir*) data;
    bool  found_last = false;

    for (int i = 0; i < 16; i++) {

      if (UNLIKELY(root[i].shortname[0] == 0x0)) {
        found_last = true;
        // end of directory
        break;
      }
      else if (UNLIKELY(root[i].shortname[0] == 0xE5)) {
        // unused index
      }
      else {
        // traverse long names, then final cluster
        // to read all the relevant info

        if (LIKELY(root[i].is_longname())) {
          auto* L = (cl_long*) &root[i];
          // the last long index is part of a chain of entries
          if (L->is_last()) {
            // buffer for long filename
            char final_name[256];
            int  final_count = 0;

            int  total = L->long_index();
            // ignore names we can't complete inside of one sector
            if (i + total >= 16) return false;

            // go to the last entry and work backwards
            i += total-1;
            L += total-1;

            for (int idx = total; idx > 0; idx--) {
              uint16_t longname[13];
              memcpy(longname+ 0, L->first, 10);
              memcpy(longname+ 5, L->second, 12);
              memcpy(longname+11, L->third, 4);

              for (int j = 0; j < 13; j++) {
                // 0xFFFF indicates end of name
                if (UNLIKELY(longname[j] == 0xFFFF)) break;
                // sometimes, invalid stuff are snuck into filenames
                if (UNLIKELY(longname[j] == 0x0)) break;

                final_name[final_count] = longname[j] & 0xFF;
                final_count++;
              }
              L--;

              if (UNLIKELY(final_count > 240)) {
                FS_PRINT("Suspicious long name length, breaking...\n");
                break;
              }
            }

            final_name[final_count] = 0;
            FS_PRINT("Long name: %s\n", final_name);

            i++; // skip over the long version
            // to the short version for the stats and cluster
            auto* D = &root[i];
            std::string dirname(final_name, final_count);

            dirents.emplace_back(
                this,
                D->type(),
                std::move(dirname),
                D->dir_cluster(root_cluster),
                sector, // parent block
                D->size(),
                D->attrib,
                D->get_modified());
          }
        }
        else {
          auto* D = &root[i];
          FS_PRINT("Short name: %.11s\n", D->shortname);

          std::string dirname((char*) D->shortname, 11);
          dirname = trim_right_copy(dirname);

          dirents.emplace_back(
              this,
              D->type(),
              std::move(dirname),
              D->dir_cluster(root_cluster),
              sector, // parent block
              D->size(),
              D->attrib,
              D->get_modified());
        }
      } // entry is long name

    } // directory list
    return found_last;
  }

}
