#include "mbr.hpp"

namespace fs
{
  template <int P, typename FS>
  void Disk<P, FS>::partitions(on_parts_func func)
  {
    // read Master Boot Record (sector 0)
    device->read_sector(0,
    [this, func] (const void* data)
    {
      std::vector<Partition> parts;
      
      if (!data)
      {
        func(false, parts);
        return;
      }
      
      // first sector is the Master Boot Record
      auto* mbr = (MBR::mbr*) data;
      
      for (int i = 0; i < 4; i++)
      {
        // all the partitions are offsets to potential Volume Boot Records
        /*
        printf("<P%u> ", i+1);
        printf("Flags: %u\t", mbr->part[i].flags);
        printf("Type: %s\t", MBR::id_to_name( mbr->part[i].type ).c_str() );
        printf("LBA begin: %x\n", mbr->part[i].lba_begin);
        */
        parts.emplace_back(
            mbr->part[i].flags,    // flags
            mbr->part[i].type,     // id
            mbr->part[i].lba_begin, // LBA
            mbr->part[i].sectors);
      }
      
      func(true, parts);
    });
  }
  
  template <int P, typename FS>
  std::string Disk<P, FS>::Partition::name() const
  {
    return MBR::id_to_name(id);
  }
  
  
}
