//#define DEBUG
#include <fs/fat.hpp>

#include <fs/mbr.hpp>
#include <cassert>
#include <debug>

#include <cstring>
#include <locale>
#include <kernel/syscalls.hpp> // for panic()

#include <info>
//#undef debug
//#define debug(...) printf(__VA_ARGS__)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

inline std::string trim_right_copy(
   const std::string& s,
   const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

namespace fs
{
  FAT::FAT(hw::Drive& dev)
    : device(dev) {
    //
  }

  void FAT::init(const void* base_sector) {

    // assume its the master boot record for now
    auto* mbr = (MBR::mbr*) base_sector;

    MBR::BPB* bpb = mbr->bpb();
    this->sector_size = bpb->bytes_per_sector;

    if (unlikely(this->sector_size < 512)) {
      fprintf(stderr,
          "Invalid sector size (%u) for FAT32 partition\n", sector_size);
      fprintf(stderr,
          "Are you mounting the correct partition?\n");
      panic("FAT32: Invalid sector size");
    }

    // Let's begin our incantation
    // To drive out the demons of old DOS we have to read some PBP values
    debug("Bytes per sector: \t%u\n", bpb->bytes_per_sector);
    debug("Sectors per cluster: \t%u\n", bpb->sectors_per_cluster);
    debug("Reserved sectors: \t%u\n", bpb->reserved_sectors);
    debug("Number of FATs: \t%u\n", bpb->fa_tables);

    debug("Small sectors (FAT16): \t%u\n", bpb->small_sectors);

    debug("Sectors per FAT: \t%u\n", bpb->sectors_per_fat);
    debug("Sectors per Track: \t%u\n", bpb->sectors_per_track);
    debug("Number of Heads: \t%u\n", bpb->num_heads);
    debug("Hidden sectors: \t%u\n", bpb->hidden_sectors);
    debug("Large sectors: \t%u\n", bpb->large_sectors);
    debug("Disk number: \t0x%x\n", bpb->disk_number);
    debug("Signature: \t0x%x\n", bpb->signature);

    debug("System ID: \t%.8s\n", bpb->system_id);

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
    debug("First data sector: %u\n", this->data_index);

    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;
    debug("Reserved sectors: %u\n", this->reserved);

    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;
    debug("Sectors per cluster: %u\n", this->sectors_per_cluster);

    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;
    debug("Data sectors: %u\n", this->data_sectors);

    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;
    debug("Total clusters: %u\n", this->clusters);

    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085) {
      this->fat_type = FAT::T_FAT12;
      this->root_cluster = 2;
      debug("The image is type FAT12, with %u clusters\n", this->clusters);
    }
    else if (this->clusters < 65525) {
      this->fat_type = FAT::T_FAT16;
      this->root_cluster = 2;
      debug("The image is type FAT16, with %u clusters\n", this->clusters);
    }
    else {
      this->fat_type = FAT::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      this->root_cluster = 2;
      debug("The image is type FAT32, with %u clusters\n", this->clusters);
      //printf("Root dir entries: %u clusters\n", bpb->root_entries);
      //assert(bpb->root_entries == 0);
      //this->root_dir_sectors = 0;
      //this->data_index = bpb->reserved_sectors + bpb->fa_tables * this->sectors_per_fat;
    }
    debug("Root cluster index: %u (sector %u)\n", this->root_cluster, cl_to_sector(root_cluster));
    debug("System ID: %.8s\n", bpb->system_id);
  }

  void FAT::mount(uint64_t base, uint64_t size, on_mount_func on_mount) {

    this->lba_base = base;
    this->lba_size = size;

    // read Partition block
    device.read(this->lba_base,
    [this, on_mount] (buffer_t data) {

      auto* mbr = (MBR::mbr*) data.get();
      assert(mbr != nullptr);

      // verify image signature
      debug("OEM name: \t%s\n", mbr->oem_name);
      debug("MBR signature: \t0x%x\n", mbr->magic);
      if (unlikely(mbr->magic != 0xAA55)) {
        on_mount({ error_t::E_MNT, "Missing or invalid MBR signature" });
        return;
      }

      // initialize FAT16 or FAT32 filesystem
      init(mbr);

      // determine which FAT version is mounted
      switch (this->fat_type) {
      case FAT::T_FAT12:
        INFO("FS", "Mounting FAT12 filesystem");
        break;
      case FAT::T_FAT16:
        INFO("FS", "Mounting FAT16 filesystem");
        break;
      case FAT::T_FAT32:
        INFO("FS", "Mounting FAT32 filesystem");
        break;
      }
      INFO2("[ofs=%u  size=%u (%u bytes)]\n",
            this->lba_base, this->lba_size, this->lba_size * 512);

      // on_mount callback
      on_mount(no_error);
    });
  }

  bool FAT::int_dirent(uint32_t  sector, const void* data, dirvector& dirents) {

    auto* root = (cl_dir*) data;
    bool  found_last = false;

    for (int i = 0; i < 16; i++) {

      if (unlikely(root[i].shortname[0] == 0x0)) {
        //printf("end of dir\n");
        found_last = true;
        // end of directory
        break;
      }
      else if (unlikely(root[i].shortname[0] == 0xE5)) {
        // unused index
      }
      else {
        // traverse long names, then final cluster
        // to read all the relevant info

        if (likely(root[i].is_longname())) {
          auto* L = (cl_long*) &root[i];
          // the last long index is part of a chain of entries
          if (L->is_last()) {
            // buffer for long filename
            char final_name[256];
            int  final_count = 0;

            int  total = L->long_index();
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
                if (unlikely(longname[j] == 0xFFFF)) break;
                // sometimes, invalid stuff are snuck into filenames
                if (unlikely(longname[j] == 0x0)) break;

                final_name[final_count] = longname[j] & 0xFF;
                final_count++;
              }
              L--;

              if (unlikely(final_count > 240)) {
                debug("Suspicious long name length, breaking...\n");
                break;
              }
            }

            final_name[final_count] = 0;
            debug("Long name: %s\n", final_name);

            i++; // skip over the long version
            // to the short version for the stats and cluster
            auto* D = &root[i];
            std::string dirname(final_name, final_count);
            dirname = trim_right_copy(dirname);

            dirents.emplace_back(
                D->type(),
                dirname,
                D->dir_cluster(root_cluster),
                sector, // parent block
                D->size(),
                D->attrib,
                D->modified);
          }
        }
        else {
          auto* D = &root[i];
          debug("Short name: %.11s\n", D->shortname);

          std::string dirname((char*) D->shortname, 11);
          dirname = trim_right_copy(dirname);

          dirents.emplace_back(
              D->type(),
              dirname,
              D->dir_cluster(root_cluster),
              sector, // parent block
              D->size(),
              D->attrib,
              D->modified);
        }
      } // entry is long name

    } // directory list
    return found_last;
  }

}
