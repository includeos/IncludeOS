#include "filetree.hpp"

#include "../api/fs/mbr.hpp"
#include "fat_internal.hpp"
#include <cassert>
#include <cstring>

static const int LONG_CHARS_PER_ENTRY = 13;
static const int ENTS_PER_SECT = 16;

long FileSys::to_cluster_hi(long pos) const
{
  return (((pos - SECT_SIZE) / SECT_SIZE) >> 16);
}
long FileSys::to_cluster_lo(long pos) const
{
  return (((pos - SECT_SIZE) / SECT_SIZE) & 0xFFFF);
}

long FileSys::write(FILE* file)
{
  assert(file);

  char mbr_code[SECT_SIZE];
  auto* mbr = (fs::MBR::mbr*) mbr_code;

  // create "valid" MBR
  memcpy(mbr->oem_name, "INCLUDOS", 8);
  mbr->magic = 0xAA55;

  // create valid BPB for old FAT
  auto* BPB = mbr->bpb();
  BPB->bytes_per_sector = SECT_SIZE;
  BPB->sectors_per_cluster = 1;
  BPB->reserved_sectors = 1; // reduce cost
  BPB->fa_tables        = 2; // always 2 FATs
  BPB->media_type       = 0xF8; // hard disk
  BPB->sectors_per_fat  = 1; // 1 sector per FAT to minify cost
  BPB->root_entries     = 0; // not using old ways
  BPB->small_sectors    = 0;
  BPB->disk_number      = 0;
  BPB->signature        = 0x29;
  strcpy(BPB->volume_label, "IncludeOS");
  strcpy(BPB->system_id,    "FAT32");

  for (decltype(sizeof(fs::MBR::partition)) i = 0, e = 4*sizeof(fs::MBR::partition); i < e; ++i) {
    ((char*) mbr->part)[i] = 0;
  }

  // write root and other entries recursively
  long root_pos = SECT_SIZE * 3; // Note: roots parent is itself :)
  long total_size = root.write(*this, file, root_pos, root_pos);

  // update values
  BPB->large_sectors = root.sectors_used();

  // write MBR
  fseek(file, 0, SEEK_SET);
  int count = fwrite(mbr_code, SECT_SIZE, 1, file);
  assert(count == 1);

  return total_size;
}

void fill(uint16_t* ucs, int len, const char* ptr)
{
  for (int i = 0; i < len; i++)
    ucs[i] = ptr[i];
}

std::vector<cl_dir> create_longname(std::string name, uint8_t enttype)
{
  assert(name.size() > SHORTNAME_LEN);

  // create checksum of "shortname"
  unsigned char csum = 0;
  for (decltype(SHORTNAME_LEN) i = 0; i < SHORTNAME_LEN; ++i)
  {
    csum = (csum >> 1) + ((csum & 1) << 7); // rotate
    csum += name[i];                        // next byte
  }

  std::vector<cl_dir> longs;
  // calculate number of entries needed
  if (name.size() % LONG_CHARS_PER_ENTRY) {
    // resize to multiple of long entry
    int rem = LONG_CHARS_PER_ENTRY - (name.size() % LONG_CHARS_PER_ENTRY);
    name.resize(name.size() + rem);
    // fill rest with spaces
    for (decltype(name.size()) i = (name.size() - rem), e = name.size(); i < e; ++i) {
      name[i] = 0x0;
    }
  }
  // number of entries needed for this longname
  int entmax = name.size() / LONG_CHARS_PER_ENTRY;

  // create entries filling as we go
  decltype(name.size()) current = 0;

  for (int i = 1; i <= entmax; i++)
  {
    cl_long ent;
    ent.index    = i;
    // mark last as LAST_LONG_ENTRY
    if (i == entmax)
      ent.index = entmax | LAST_LONG_ENTRY;
    ent.attrib   = enttype | 0x0F;  // mark as long name
    ent.checksum = csum;
    ent.zero     = 0;

    fill(ent.first, 5, &name[current]);
    current += 5;
    fill(ent.second, 6, &name[current]);
    current += 6;
    fill(ent.third, 2, &name[current]);
    current += 2;
    // sanity check
    assert(current <= name.size());

    longs.insert(longs.begin(), *(cl_dir*) &ent);
  }
  //
  return longs;
}

void create_preamble(
    FileSys& fsys, std::vector<cl_dir>& ents, long self, long parent)
{
  cl_dir ent;
  ent.attrib = ATTR_DIRECTORY;
  ent.filesize = 0;
  ent.modified = 0;
  // . current directory
  memcpy((char*) ent.shortname, ".            ", SHORTNAME_LEN);
  ent.cluster_hi = fsys.to_cluster_hi(self);
  ent.cluster_lo = fsys.to_cluster_lo(self);
  ents.push_back(ent);
  // .. parent directory
  memcpy((char*) ent.shortname, "..           ", SHORTNAME_LEN);
  ent.cluster_hi = fsys.to_cluster_hi(parent);
  ent.cluster_lo = fsys.to_cluster_lo(parent);
  ents.push_back(ent);
}

cl_dir create_entry(const std::string& name, uint8_t attr, uint32_t size)
{
  cl_dir ent;
  ent.shortname[0] = name[0];
  decltype(name.size()) nlen = std::min(name.size(), SHORTNAME_LEN);
  memcpy((char*) ent.shortname, name.data(), nlen);
  // fill rest with spaces
  for (decltype(SHORTNAME_LEN) i = nlen; i < SHORTNAME_LEN; ++i) {
    ent.shortname[i] = 32;
  }
  ent.attrib = attr;
  ent.cluster_hi = 0; /// SET THIS
  ent.cluster_lo = 0; /// SET THIS
  ent.filesize   = size;
  ent.modified   = 0;
  return ent;
}

void fill_unused(std::vector<cl_dir>& ents, int num)
{
  cl_dir ent;
  ent.shortname[0] = 0xE5; // unused entry
  ent.attrib     = 0;
  ent.cluster_hi = 0;
  ent.cluster_lo = 0;
  ent.filesize   = 0;
  ent.modified   = 0;
  while (num-- > 0) ents.push_back(ent);
}
void mod16_test(std::vector<cl_dir>& ents, int& mod16, int long_entries)
{
  // if longname is overshooting sector
  int x = mod16 % ENTS_PER_SECT;
  if (x + long_entries + 1 > ENTS_PER_SECT) {
    // fill remainder of sector with unused entries
    x = ENTS_PER_SECT - x;
    fill_unused(ents, x);
    mod16 += x;
  }
  mod16 += long_entries;
}

long Dir::write(FileSys& fsys, FILE* file, long pos, long parent)
{
  // create vector of dirents
  std::vector<cl_dir> ents;
  // create . and .. entries
  create_preamble(fsys, ents, pos, parent);
  //
  int mod16 = ents.size();

  for (auto& sub : subs)
  {
    sub.size_helper = 1;
    // longname if needed
    if (sub.name.size() > SHORTNAME_LEN) {
      // add longname entries
      auto longs = create_longname(sub.name, ATTR_DIRECTORY);
      if (longs.size() > ENTS_PER_SECT) continue;

      // test and fill remainder of sector if overshooting
      mod16_test(ents, mod16, longs.size());
      // insert longnames to back of directory
      ents.insert(ents.end(), longs.begin(), longs.end());
      sub.size_helper += longs.size();
    }
    mod16 += 1;
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
      // test and fill remainder of sector if overshooting
      mod16_test(ents, mod16, longs.size());
      // insert longnames to back of directory
      ents.insert(ents.end(), longs.begin(), longs.end());
      file.size_helper += longs.size();
    }
    mod16 += 1;
    // actual file entry
    file.idx_helper = ents.size();
    ents.push_back(create_entry(file.name, ATTR_READ_ONLY, file.size));
  }
  // add last entry
  {
    cl_dir last;
    last.shortname[0] = 0x0; // last entry
    last.cluster_hi = 0;
    last.cluster_lo = 0;
    last.attrib   = 0;
    last.modified = 0;
    last.filesize = 0;
    ents.push_back(last);
  }

  // use dirents to measure size in sectors of this directory
  long ssize = (ents.size() / 16) + ((ents.size() & 15) ? 1 : 0);

  // the next directory and files start at pos + SECT_SIZE * ssize etc
  long newpos = pos + ssize * SECT_SIZE;

  // write directories
  for (auto& sub : subs)
  {
    auto& ent = ents[sub.idx_helper];
    ent.cluster_hi = fsys.to_cluster_hi(newpos);
    ent.cluster_lo = fsys.to_cluster_lo(newpos);
    // get new position by writing directory
    newpos = sub.write(fsys, file, newpos, pos);
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
  assert(count == static_cast<int>(ents.size()));

  return newpos;
}

long File::write(FileSys&, FILE* file, long pos) const
{
  //printf("writing file to %ld with size %u\n", pos, this->size);
  fseek(file, pos, SEEK_SET);
  if (this->size > 0)
  {
    int count = fwrite(data.get(), this->size, 1, file);
    assert(count == 1);
  }
  // write zeroes to remainder
  int rem = SECT_SIZE - (this->size & (SECT_SIZE-1));
  fwrite("\0", 1, rem, file);
  // return position after file
  return pos + this->sectors_used() * SECT_SIZE;
}
