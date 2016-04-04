#include <fs/mbr.hpp>

namespace fs
{
  std::string MBR::id_to_name(uint8_t id)
  {
    switch (id)
      {
      case 0x00:
        return "Empty";
      case 0x01:
        return "DOS 12-bit FAT";
      case 0x02:
        return "XENIX root";
      case 0x03:
        return "XENIX /usr";
      case 0x04:
        return "DOS 3.0+ 16-bit FAT";
      case 0x05:
        return "DOS 3.3+ Extended Partition";
      case 0x06:
        return "DOS 3.31+ 16-bit FAT (32M+)";
      case 0x07:
        return "NTFS or exFAT";
      case 0x08:
        return "Commodore DOS logical FAT";
      case 0x0b:
        return "WIN95 OSR2 FAT32";
      case 0x0c:
        return "WIN95 OSR2 FAT32, LBA-mapped";
      case 0x0d:
        return "SILICON SAFE";
      case 0x0e:
        return "WIN95: DOS 16-bit FAT, LBA-mapped";
      case 0x0f:
        return "WIN95: Extended partition, LBA-mapped";
      case 0x11:
        return "Hidden DOS 12-bit FAT";
      case 0x12:
        return "Configuration utility partition (Compaq)";
      case 0x14:
        return "Hidden DOS 16-bit FAT <32M";
      case 0x16:
        return "Hidden DOS 16-bit FAT >= 32M";
      case 0x27:
        return "Windows RE hidden partition";
      case 0x3c:
        return "PartitionMagic recovery partition";
      case 0x82:
        return "Linux swap";
      case 0x83:
        return "Linux native partition";
      case 0x84:
        return "Hibernation partition";
      case 0x85:
        return "Linux extended partition";
      case 0x86:
        return "FAT16 fault tolerant volume set";
      case 0x87:
        return "NTFS fault tolerant volume set";
      case 0x8e:
        return "Linux Logical Volume Manager partition";
      case 0x9f:
        return "BSDI (BSD/OS)";
      case 0xa6:
        return "OpenBSD";
      case 0xa8:
        return "Apple MacOS X (BSD-like filesystem)";
      case 0xa9:
        return "NetBSD";
      case 0xaf:
        return "MacOS X HFS";
      default:
        return "Invalid identifier: " + std::to_string(id);
      }
  }
  
  
}
