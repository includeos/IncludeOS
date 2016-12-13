#include "filetree.hpp"

#include "../api/fs/mbr.hpp"
#include <cassert>
#include <cstring>

void FileSys::write(const char* path)
{
  FILE* f = fopen(path, "w");
  assert(f);
  
  char mbr_code[SECT_SIZE];
  auto* mbr = (fs::MBR::mbr*) mbr_code;
  
  // create "valid" MBR
  strncpy(mbr->oem_name, "INCLUDOS", sizeof(mbr->oem_name));
  mbr->magic = 0xAA55;
  
  // create valid BPB for old FAT
  auto* BPB = mbr->bpb();
  BPB->bytes_per_sector = SECT_SIZE;
  BPB->disk_number      = 0;
  BPB->fa_tables        = 2; // always 2 FATs
  BPB->sectors_per_fat  = 1; // 1 sector per FAT to minify cost
  BPB->reserved_sectors = 0; // reduce cost
  BPB->root_entries     = 0; // not using old ways
  
  BPB->small_sectors    = 0;
  // calculate this value from entries
  BPB->large_sectors    = root.sectors_used();
  
  // write MBR
  int count = fwrite(mbr_code, SECT_SIZE, 1, f);
  assert(count == 1);
  
  // write root and other entries recursively
  root.write(f, SECT_SIZE);
  
  fclose(f);
}

void Dir::write(FILE* file, long pos)
{
  
}
