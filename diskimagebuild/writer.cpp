#include "filetree.hpp"

#include "../api/fs/mbr.hpp"
#include "../api/fs/fat_internal.hpp"
#include <cassert>
#include <cstring>

static const int LONG_CHARS_PER_ENTRY = 13;

long FileSys::to_cluster_hi(long pos) const
{
  return (2 + pos / SECT_SIZE) >> 16;
}
long FileSys::to_cluster_lo(long pos) const
{
  return (2 + pos / SECT_SIZE) & 0xFFFF;
}

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
  root.write(*this, f, SECT_SIZE);
  
  fclose(f);
}

void fill(uint16_t* ucs, int len, const char* ptr)
{
  for (int i = 0; i < len; i++)
    ucs[i * 2] = ptr[i];
}

std::vector<cl_dir> create_longname(std::string name, uint8_t enttype)
{
  assert(!name.empty());
  std::vector<cl_dir> longs;
  // calculate number of entries needed
  if (name.size() % LONG_CHARS_PER_ENTRY) {
    printf("RESIZE %lu to", name.size());
    // resize to multiple of long entry
    int rem = LONG_CHARS_PER_ENTRY - (name.size() % LONG_CHARS_PER_ENTRY);
    name.resize(name.size() + rem);
    // zero-fill the end
    for (int i = name.size() - rem; i < name.size(); i++)
      name[i] = 0x0;
    
    printf(" %lu (resized %d)\n", name.size(), rem);
  }
  // number of entries needed for this longname
  int entmax = name.size() / LONG_CHARS_PER_ENTRY;
  
  // create entries filling as we go
  int current = 0;
  
  for (int i = 0; i < entmax; i++)
  {
    cl_long ent;
    ent.index    = entmax - i;
    // mark last as LAST_LONG_ENTRY
    if (i == entmax-1)
        ent.index |= LAST_LONG_ENTRY;
    ent.attrib   = enttype;
    ent.attrib  |= 0x0F;  // mark as long name
    ent.checksum = 0;
    ent.zero     = 0;
    
    fill(ent.first, 5, &name[current]);
    current += 5;
    fill(ent.second, 6, &name[current]);
    current += 6;
    fill(ent.third, 2, &name[current]);
    current += 2;
    
    longs.push_back(*(cl_dir*) &ent);
    // sanity check
    assert(current <= name.size());
  }
  //
  return longs;
}

cl_dir create_entry(const std::string& name, uint8_t attr, uint32_t size)
{
  cl_dir ent;
  strncpy((char*) ent.shortname, name.data(), SHORTNAME_LEN);
  ent.attrib = attr;
  ent.cluster_hi = 0; /// SET THIS
  ent.cluster_lo = 0; /// SET THIS
  ent.filesize   = 0;
  return ent;
}

long Dir::write(FileSys& fsys, FILE* file, long pos)
{
  // create vector of dirents
  std::vector<cl_dir> ents;
  
  for (auto& sub : subs)
  {
    sub.size_helper = 1;
    // longname if needed
    if (sub.name.size() > SHORTNAME_LEN) {
      // add longname entries
      auto longs = create_longname(sub.name, ATTR_DIRECTORY);
      ents.insert(ents.end(), longs.begin(), longs.end());
      sub.size_helper += longs.size();
    }
    // actual dirent *sigh*
    sub.idx_helper = ents.size();
    ents.push_back(create_entry(sub.name, ATTR_DIRECTORY, 0));
  }
  for (auto& file : files)
  {
    file.size_helper = 1;
    // longname if needed
    if (file.name.size() > SHORTNAME_LEN) {
      // add longname entries
      auto longs = create_longname(file.name, ATTR_READ_ONLY);
      ents.insert(ents.end(), longs.begin(), longs.end());
      file.size_helper += longs.size();
    }
    // actual file entry
    file.idx_helper = ents.size();
    ents.push_back(create_entry(file.name, ATTR_READ_ONLY, file.size));
  }
  // add last entry
  {
    cl_dir last;
    last.shortname[0] = 0x0; // last entry
    last.attrib = 0;
    ents.push_back(last);
  }
  
  // use dirents to measure size in sectors of this directory
  int ssize = ents.size() / 16 + (ents.size() & 15) ? 1 : 0;
  // the next directory and files start at pos + SECT_SIZE * ssize etc
  long newpos = pos + ssize * SECT_SIZE;
  
  // write directories
  for (auto& sub : subs)
  {
    auto& ent = ents[sub.idx_helper];
    ent.cluster_hi = fsys.to_cluster_hi(newpos);
    ent.cluster_lo = fsys.to_cluster_lo(newpos);
    // get new position by writing directory
    newpos = sub.write(fsys, file, newpos);
  }
  // write files
  for (const auto& fil : files)
  {
    auto& ent = ents[fil.idx_helper];
    ent.cluster_hi = fsys.to_cluster_hi(newpos);
    ent.cluster_lo = fsys.to_cluster_lo(newpos);
    // write file to newpos
    newpos = fil.write(fsys, file, newpos);
  }
  
  /// write all the entries for this directory to file
  fseek(file, pos, SEEK_SET);
  int count = fwrite(ents.data(), sizeof(cl_dir), ents.size(), file);
  
  return newpos;
}

long File::write(FileSys&, FILE* file, long pos) const
{
  fseek(file, pos, SEEK_SET);
  int count = fwrite(data.get(), this->size, 1, file);
  assert(count == 1);
  return pos + this->size;
}
