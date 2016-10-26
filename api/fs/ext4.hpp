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

#pragma once
#ifndef FS_EXT4_HPP
#define FS_EXT4_HPP

#include "filesystem.hpp"
#include <hw/drive.hpp>
#include <functional>
#include <cstdint>
#include <memory>

namespace fs
{
  class Path;

  struct EXT4 : public File_system
  {
    /**
       Blocks                      2^32           2^32    2^32     2^32
       Inodes                      2^32           2^32    2^32     2^32
       File System Size                  4TiB     8TiB   16TiB   256PiB
       Blocks Per Block Group     8,192         16,384  32,768  524,288
       Inodes Per Block Group     8,192         16,384  32,768  524,288
       Block Group Size                  8MiB      32MiB        128MiB          32GiB
       Blocks Per File, Extents          2^32     2^32    2^32     2^32
       Blocks Per File, Block Maps      16,843,020      134,480,396     1,074,791,436   4,398,314,962,956
       File Size, Extents         4TiB     8TiB   16TiB  256TiB
       File Size, Block Maps    16GiB   256GiB  4TiB    256PiB
    **/

    // 0   = Mount MBR
    // 1-4 = Mount VBR 1-4
    virtual void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) override;

    // path is a path in the mounted filesystem
    virtual void  ls(const std::string& path, on_ls_func) override;
    virtual List  ls(const std::string& path) override;

    /** Read @n bytes from file pointed by @entry starting at position @pos */
    virtual void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) override;
    virtual Buffer read(const Dirent&, uint64_t pos, uint64_t n) override;

    // return information about a filesystem entity
    virtual void   stat(const std::string&, on_stat_func) override;
    virtual Dirent stat(const std::string& ent) override;

    // returns the name of the filesystem
    virtual std::string name() const override
    {
      return "Linux EXT4";
    }

    // constructor
    EXT4(hw::Drive& dev);
    ~EXT4() {}

  private:
    static const int EXT4_N_BLOCKS = 15;
    static const int EXT2_GOOD_OLD_INODE_SIZE = 128;

    struct superblock
    {
      uint32_t  inodes_count;    // Total inode count
      uint32_t  blocks_count_lo; // Total block count
      uint32_t  r_blocks_count_lo; // Super-user alloc. blocks
      uint32_t  free_blocks_count_lo; // Free block count
      uint32_t  free_inodes_count; // Free inode count
      uint32_t  first_data_block; // First data block
      // Block size is 2 ^ (10 + s_log_block_size)
      uint32_t  log_block_size;
      // Cluster size is (2 ^ s_log_cluster_size) blocks if bigalloc
      // is enabled, zero otherwise
      uint32_t  log_cluster_size;
      uint32_t  blocks_per_group; // Blocks per group
      uint32_t  clusters_per_group; // Clusters per group, if bigalloc is enabled
      uint32_t  inodes_per_group; // Inodes per group
      uint32_t  mtime; // Mount time, in seconds since the epoch
      uint32_t  wtime; // Write time, in seconds since the epoch

      uint16_t  mnt_count; // Number of mounts since the last fsck
      uint16_t  max_mnt_count; // Number of mounts beyond which a fsck is needed

      uint16_t  magic; // Magic signature, 0xEF53

      // File system state. Valid values are:
      //  0x0001        Cleanly umounted
      //  0x0002        Errors detected
      //  0x0004        Orphans being recovered
      uint16_t  state;

      // Behaviour when detecting errors. One of:
      //  1     Continue
      //  2     Remount read-only
      //  3     Panic
      uint16_t  errors;
      uint16_t  minor_rev_level; // Minor revision level

      uint32_t  lastcheck; // Time of last check, in seconds since the epoch
      uint32_t  checkinterval; // Maximum time between checks, in seconds

      // OS. One of:
      //  0     Linux
      //  1     Hurd
      //  2     Masix
      //  3     FreeBSD
      //  4     Lites
      uint32_t  creator_os;

      // Revision level. One of:
      //  0     Original format
      //  1     v2 format w/ dynamic inode sizes
      uint32_t  rev_level;

      uint16_t  def_resuid; // Default uid for reserved blocks
      uint16_t  def_resgid; // Default gid for reserved blocks

      //////////////////////////////////////////////////////////////
      /// These fields are for EXT4_DYNAMIC_REV superblocks only ///
      //////////////////////////////////////////////////////////////
      //
      // Note: the difference between the compatible feature set
      // and the incompatible feature set is that if there is a bit
      // set in the incompatible feature set that the kernel doesn't
      // know about, it should refuse to mount the filesystem.
      //
      // e2fsck's requirements are more strict; if it doesn't know
      // about a feature in either the compatible or incompatible
      // feature set, it must abort and not try to meddle with things
      // it doesn't understand...

      uint32_t  first_ino;  // First non-reserved inode
      uint16_t  inode_size; // Size of inode structure, in bytes
      uint16_t  block_group_nr; // Block group # of this superblock

      // Compatible feature set flags.
      // Kernel can still read/write this fs even if it doesn't
      // understand a flag; fsck should not do that. Any of:
      //  0x1   Directory preallocation (COMPAT_DIR_PREALLOC).
      //  0x2   "imagic inodes". Not clear from the code what this does (COMPAT_IMAGIC_INODES).
      //  0x4   Has a journal (COMPAT_HAS_JOURNAL).
      //  0x8   Supports extended attributes (COMPAT_EXT_ATTR).
      //  0x10  Has reserved GDT blocks for filesystem expansion (COMPAT_RESIZE_INODE).
      //  0x20  Has directory indices (COMPAT_DIR_INDEX).
      //  0x40  "Lazy BG". Not in Linux kernel, seems to have been for uninitialized block groups? (COMPAT_LAZY_BG)
      //  0x80  "Exclude inode". Not used. (COMPAT_EXCLUDE_INODE).
      //  0x100         "Exclude bitmap". Seems to be used to indicate the presence of snapshot-related exclude bitmaps? Not defined in kernel or used in e2fsprogs (COMPAT_EXCLUDE_BITMAP).
      //  0x200         Sparse Super Block, v2. If this flag is set, the SB field s_backup_bgs points to the two block groups that contain backup superblocks (COMPAT_SPARSE_SUPER2).
      uint32_t  feature_compat;

      // Incompatible feature set. If the kernel or fsck doesn't
      // understand one of these bits, it should stop. Any of:
      //  0x1   Compression (INCOMPAT_COMPRESSION).
      //  0x2   Directory entries record the file type. See ext4_dir_entry_2 below (INCOMPAT_FILETYPE).
      //  0x4   Filesystem needs recovery (INCOMPAT_RECOVER).
      //  0x8   Filesystem has a separate journal device (INCOMPAT_JOURNAL_DEV).
      //  0x10  Meta block groups. See the earlier discussion of this feature (INCOMPAT_META_BG).
      //  0x40  Files in this filesystem use extents (INCOMPAT_EXTENTS).
      //  0x80  Enable a filesystem size of 2^64 blocks (INCOMPAT_64BIT).
      //  0x100         Multiple mount protection. Not implemented (INCOMPAT_MMP).
      //  0x200         Flexible block groups. See the earlier discussion of this feature (INCOMPAT_FLEX_BG).
      //  0x400         Inodes can be used for large extended attributes (INCOMPAT_EA_INODE). (Not implemented?)
      //  0x1000        Data in directory entry (INCOMPAT_DIRDATA). (Not implemented?)
      //  0x2000        Metadata checksum seed is stored in the superblock. This feature enables the administrator to change the UUID of a metadata_csum filesystem while the filesystem is mounted; without it, the checksum definition requires all metadata blocks to be rewritten (INCOMPAT_CSUM_SEED).
      //  0x4000        Large directory >2GB or 3-level htree (INCOMPAT_LARGEDIR).
      //  0x8000        Data in inode (INCOMPAT_INLINE_DATA).
      //  0x10000       Encrypted inodes are present on the filesystem. (INCOMPAT_ENCRYPT).
      uint32_t feature_incompat;

      // Readonly-compatible feature set. If the kernel doesn't
      // understand one of these bits, it can still mount read-only.
      // Any of:
      //0x1     Sparse superblocks. See the earlier discussion of this feature (RO_COMPAT_SPARSE_SUPER).
      //0x2     This filesystem has been used to store a file greater than 2GiB (RO_COMPAT_LARGE_FILE).
      //0x4     Not used in kernel or e2fsprogs (RO_COMPAT_BTREE_DIR).
      //0x8     This filesystem has files whose sizes are represented in units of logical blocks, not 512-byte sectors. This implies a very large file indeed! (RO_COMPAT_HUGE_FILE)
      //0x10    Group descriptors have checksums. In addition to detecting corruption, this is useful for lazy formatting with uninitialized groups (RO_COMPAT_GDT_CSUM).
      //0x20    Indicates that the old ext3 32,000 subdirectory limit no longer applies (RO_COMPAT_DIR_NLINK).
      //0x40    Indicates that large inodes exist on this filesystem (RO_COMPAT_EXTRA_ISIZE).
      //0x80    This filesystem has a snapshot (RO_COMPAT_HAS_SNAPSHOT).
      //0x100   Quota (RO_COMPAT_QUOTA).
      //0x200   This filesystem supports "bigalloc", which means that file extents are tracked in units of clusters (of blocks) instead of blocks (RO_COMPAT_BIGALLOC).
      //0x400   This filesystem supports metadata checksumming. (RO_COMPAT_METADATA_CSUM; implies RO_COMPAT_GDT_CSUM, though GDT_CSUM must not be set)
      //0x800   Filesystem supports replicas. This feature is neither in the kernel nor e2fsprogs. (RO_COMPAT_REPLICA)
      //0x1000  Read-only filesystem image; the kernel will not mount this image read-write and most tools will refuse to write to the image. (RO_COMPAT_READONLY)
      //0x2000  Filesystem tracks project quotas. (RO_COMPAT_PROJECT)
      uint32_t  feature_ro_compat;

      uint8_t uuid[16]; // 128-bit UUID for volume
      char  volume_name[16]; // Volume label

      // Directory where filesystem was last mounted
      char  last_mounted[64];

      // For compression (Not used in e2fsprogs/Linux):
      uint32_t  algorithm_usage_bitmap;

      // Performance hints. Directory preallocation should only
      // happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
      // # of blocks to try to preallocate for ... files? (Not used in e2fsprogs/Linux)
      uint8_t  prealloc_blocks;
      // # of blocks to preallocate for directories. (Not used in e2fsprogs/Linux)
      uint8_t  prealloc_dir_blocks;
      // Number of reserved GDT entries for future filesystem expansion
      uint16_t reserved_gdt_blocks;

      // Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set
      uint8_t   journal_uuid[16]; // UUID of journal superblock
      uint32_t  journal_inum;   // inode number of journal file.
      uint32_t  journal_dev;    // Device number of journal file, if the external journal feature flag is set

      uint32_t  last_orphan; // Start of list of orphaned inodes to delete
      uint32_t  hash_seed[4]; // HTREE hash seed

      // Default hash algorithm to use for directory hashes. One of:
      //  0x0   Legacy.
      //  0x1   Half MD4.
      //  0x2   Tea.
      //  0x3   Legacy, unsigned.
      //  0x4   Half MD4, unsigned.
      //  0x5   Tea, unsigned.
      uint8_t   def_hash_version;

      // If this value is 0 or EXT3_JNL_BACKUP_BLOCKS (1), then the s_jnl_blocks field contains a duplicate copy of the inode's i_block[] array and i_size
      uint8_t   jnl_backup_type;
      // Size of group descriptors, in bytes, if the 64bit incompat feature flag is set
      uint16_t  desc_size;

      // Default mount options. Any of:
      //  0x001         Print debugging info upon (re)mount. (EXT4_DEFM_DEBUG)
      //  0x002         New files take the gid of the containing directory (instead of the fsgid of the current process). (EXT4_DEFM_BSDGROUPS)
      //  0x004         Support userspace-provided extended attributes. (EXT4_DEFM_XATTR_USER)
      //  0x008         Support POSIX access control lists (ACLs). (EXT4_DEFM_ACL)
      //  0x010         Do not support 32-bit UIDs. (EXT4_DEFM_UID16)
      //  0x020         All data and metadata are commited to the journal. (EXT4_DEFM_JMODE_DATA)
      //  0x040         All data are flushed to the disk before metadata are committed to the journal. (EXT4_DEFM_JMODE_ORDERED)
      //  0x060         Data ordering is not preserved; data may be written after the metadata has been written. (EXT4_DEFM_JMODE_WBACK)
      //  0x100         Disable write flushes. (EXT4_DEFM_NOBARRIER)
      //  0x200         Track which blocks in a filesystem are metadata and therefore should not be used as data blocks. This option will be enabled by default on 3.18, hopefully. (EXT4_DEFM_BLOCK_VALIDITY)
      //  0x400         Enable DISCARD support, where the storage device is told about blocks becoming unused. (EXT4_DEFM_DISCARD)
      //  0x800         Disable delayed allocation. (EXT4_DEFM_NODELALLOC)
      uint32_t  default_mount_opts;

      // First metablock block group, if the meta_bg feature is enabled
      uint32_t  first_meta_bg;
      // When the filesystem was created, in seconds since the epoch
      uint32_t  mkfs_time;
      // Backup copy of the journal inode's i_block[] array in the first 15 elements and i_size_high and i_size in the 16th and 17th elements, respectively.
      uint32_t  jnl_blocks[17];

      // 64bit support valid if EXT4_FEATURE_COMPAT_64BIT
      uint32_t  blocks_count_hi;      // High 32-bits of the block count
      uint32_t  r_blocks_count_hi;    // High 32-bits of the reserved block count
      uint32_t  free_blocks_count_hi; // High 32-bits of the free block count
      uint16_t  min_extra_isize;  // All inodes have at least # bytes
      uint16_t  want_extra_isize; // New inodes should reserve # bytes

      // Miscellaneous flags. Any of:
      //  0x01  Signed directory hash in use.
      //  0x02  Unsigned directory hash in use.
      //  0x04  To test development code.
      uint32_t  flags;

      // RAID stride. This is the number of logical blocks read from or written to the disk before moving to the next disk. This affects the placement of filesystem metadata, which will hopefully make RAID storage faster
      uint16_t  raid_stride;
      uint16_t  mmp_interval; // # seconds to wait in multi-mount prevention (MMP) checking. In theory, MMP is a mechanism to record in the superblock which host and device have mounted the filesystem, in order to prevent multiple mounts. This feature does not seem to be implemented...
      uint64_t  mmp_block;    // Block # for multi-mount protection data.

      // RAID stripe width. This is the number of logical blocks read from or written to the disk before coming back to the current disk. This is used by the block allocator to try to reduce the number of read-modify-write operations in a RAID5/6.
      uint32_t  raid_stripe_width;
      // Size of a flexible block group is 2 ^ s_log_groups_per_flex
      uint8_t   log_groups_per_flex;
      // Metadata checksum algorithm type. The only valid value is 1 (crc32c)
      uint8_t   checksum_type;
      uint16_t  reserved_pad;

      // Number of KiB written to this filesystem over its lifetime
      uint64_t  kbytes_written;
      uint32_t  snapshot_inum; // inode number of active snapshot. (Not used in e2fsprogs/Linux.)
      uint32_t  snapshot_id; // Sequential ID of active snapshot. (Not used in e2fsprogs/Linux.)
      uint64_t  snapshot_r_blocks_count; // Number of blocks reserved for active snapshot's future use. (Not used in e2fsprogs/Linux.)
      uint32_t  snapshot_list; // inode number of the head of the on-disk snapshot list. (Not used in e2fsprogs/Linux.)

      uint32_t  error_count; // Number of errors seen
      uint32_t  first_error_time; // First time an error happened, in seconds since the epoch
      uint32_t  first_error_ino; // inode involved in first error
      uint64_t  first_error_block; // Number of block involved of first error
      uint8_t   first_error_func[32]; // Name of function where the error happened

      uint32_t  first_error_line; // Line number where error happened
      uint32_t  last_error_time;  // Time of most recent error, in seconds since the epoch
      uint32_t  last_error_ino;   // inode involved in most recent error
      uint32_t  last_error_line;  // Line number where most recent error happened
      uint64_t  last_error_block; // Number of block involved in most recent error
      uint8_t   last_error_func[32]; // Name of function where the most recent error happened

      uint8_t   mount_opts[64]; // ASCIIZ string of mount options

      uint32_t  usr_quota_inum; // Inode number of user quota file
      uint32_t  grp_quota_inum; // Inode number of group quota file
      uint32_t  overhead_blocks; // Overhead blocks/clusters in fs. (Huh? This field is always zero, which means that the kernel calculates it dynamically.)
      uint32_t  backup_bgs[2]; // Block groups containing superblock backups (if sparse_super2)

      // Encryption algorithms in use. There can be up to four algorithms in use at any time; valid algorithm codes are given below:
      //  0     Invalid algorithm (ENCRYPTION_MODE_INVALID).
      //  1     256-bit AES in XTS mode (ENCRYPTION_MODE_AES_256_XTS).
      //  2     256-bit AES in GCM mode (ENCRYPTION_MODE_AES_256_GCM).
      //  3     256-bit AES in CBC mode (ENCRYPTION_MODE_AES_256_CBC).
      uint8_t   encrypt_algos[4];
      // Salt for the string2key algorithm for encryption.
      uint8_t   encrypt_pw_salt[16];

      uint32_t  lpf_ino; // Inode number of lost+found
      uint32_t  prj_quota_inum; // Inode that tracks project quotas

      // Checksum seed used for metadata_csum calculations. This value is crc32c(~0, $orig_fs_uuid).
      uint32_t  checksum_seed;

      uint32_t  reserved[98]; // Padding to the end of the block
      uint32_t  checksum; // Superblock checksum

    } __attribute__((packed));
    // <-- should be 1024 bytes

    struct group_desc
    {
      uint32_t  block_bitmap_lo; // Lower 32-bits of location of block bitmap
      uint32_t  inode_bitmap_lo; // Lower 32-bits of location of inode bitmap
      uint32_t  inode_table_lo;  // Lower 32-bits of location of inode table

      uint16_t  free_blocks_count_lo; // Lower 16-bits of free block count
      uint16_t  free_inodes_count_lo; // Lower 16-bits of free inode count
      uint16_t  used_dirs_count_lo;   // Lower 16-bits of directory count
      // Block group flags. Any of:
      //  0x1   inode table and bitmap are not initialized (EXT4_BG_INODE_UNINIT).
      //  0x2   block bitmap is not initialized (EXT4_BG_BLOCK_UNINIT).
      //  0x4   inode table is zeroed (EXT4_BG_INODE_ZEROED).
      uint16_t  flags;

      uint32_t  exclude_bitmap_lo; // Lower 32-bits of location of snapshot exclusion bitmap
      uint16_t  block_bitmap_csum_lo; // Lower 16-bits of the block bitmap checksum
      uint16_t  inode_bitmap_csum_lo; // Lower 16-bits of the inode bitmap checksum
      uint16_t  itable_unused_lo; // Lower 16-bits of unused inode count. If set, we needn't scan past the (sb.s_inodes_per_group - gdt.bg_itable_unused)th entry in the inode table for this group

      // Group descriptor checksum; crc16(sb_uuid+group+desc) if the RO_COMPAT_GDT_CSUM feature is set, or crc32c(sb_uuid+group_desc) & 0xFFFF if the RO_COMPAT_METADATA_CSUM feature is set
      uint16_t  checksum;

      /////////////////////////////////////////////////////////////////
      // These fields only exist if the 64bit feature is enabled and
      // desc_size > 32.
      uint32_t  block_bitmap_hi;
      uint32_t  inode_bitmap_hi;
      uint32_t  inode_table_hi;
      uint16_t  free_blocks_count_hi;
      uint16_t  free_inodes_count_hi;
      uint16_t  used_dirs_count_hi; // Upper 16-bits of directory count
      uint16_t  itable_unused_hi; // Upper 16-bits of unused inode count

      uint32_t  exclude_bitmap_hi; // Upper 32-bits of location of snapshot exclusion bitmap
      uint16_t  block_bitmap_csum_hi;
      uint16_t  inode_bitmap_csum_hi;

      uint32_t  reserved; // Padding to 64 bytes

    } __attribute__((packed));

    struct inode_table
    {
      // File mode. Any of:
      //  0x1   S_IXOTH (Others may execute)
      //  0x2   S_IWOTH (Others may write)
      //  0x4   S_IROTH (Others may read)
      //  0x8   S_IXGRP (Group members may execute)
      //  0x10  S_IWGRP (Group members may write)
      //  0x20  S_IRGRP (Group members may read)
      //  0x40  S_IXUSR (Owner may execute)
      //  0x80  S_IWUSR (Owner may write)
      //  0x100         S_IRUSR (Owner may read)
      //  0x200         S_ISVTX (Sticky bit)
      //  0x400         S_ISGID (Set GID)
      //  0x800         S_ISUID (Set UID)
      //  These are mutually-exclusive file types:
      //  0x1000        S_IFIFO (FIFO)
      //  0x2000        S_IFCHR (Character device)
      //  0x4000        S_IFDIR (Directory)
      //  0x6000        S_IFBLK (Block device)
      //  0x8000        S_IFREG (Regular file)
      //  0xA000        S_IFLNK (Symbolic link)
      //  0xC000        S_IFSOCK (Socket)
      uint16_t  mode;

      uint16_t  uid; // Owner-ID (lower 16)
      uint32_t  size_lo; // Size (lower 32)
      uint32_t  atime; // last access time
      uint32_t  ctime; // last inode change time
      uint32_t  mtime; // last data modification time
      uint32_t  dtime; // deletion time
      uint16_t  gid;
      uint16_t  links_count; // Hard links
      uint32_t  blocks_lo; // "Block" count (lower 32)

      // Inode flags. Any of:
      //  0x1   This file requires secure deletion (EXT4_SECRM_FL). (not implemented)
      //  0x2   This file should be preserved, should undeletion be desired (EXT4_UNRM_FL). (not implemented)
      //  0x4   File is compressed (EXT4_COMPR_FL). (not really implemented)
      //  0x8   All writes to the file must be synchronous (EXT4_SYNC_FL).
      //  0x10  File is immutable (EXT4_IMMUTABLE_FL).
      //  0x20  File can only be appended (EXT4_APPEND_FL).
      //  0x40  The dump(1) utility should not dump this file (EXT4_NODUMP_FL).
      //  0x80  Do not update access time (EXT4_NOATIME_FL).
      //  0x100         Dirty compressed file (EXT4_DIRTY_FL). (not used)
      //  0x200         File has one or more compressed clusters (EXT4_COMPRBLK_FL). (not used)
      //  0x400         Do not compress file (EXT4_NOCOMPR_FL). (not used)
      //  0x800         Encrypted inode (EXT4_ENCRYPT_FL). This bit value previously was EXT4_ECOMPR_FL (compression error), which was never used.
      //  0x1000        Directory has hashed indexes (EXT4_INDEX_FL).
      //  0x2000        AFS magic directory (EXT4_IMAGIC_FL).
      //  0x4000        File data must always be written through the journal (EXT4_JOURNAL_DATA_FL).
      //  0x8000        File tail should not be merged (EXT4_NOTAIL_FL). (not used by ext4)
      //  0x10000       All directory entry data should be written synchronously (see dirsync) (EXT4_DIRSYNC_FL).
      //  0x20000       Top of directory hierarchy (EXT4_TOPDIR_FL).
      //  0x40000       This is a huge file (EXT4_HUGE_FILE_FL).
      //  0x80000       Inode uses extents (EXT4_EXTENTS_FL).
      //  0x200000      Inode used for a large extended attribute (EXT4_EA_INODE_FL).
      //  0x400000      This file has blocks allocated past EOF (EXT4_EOFBLOCKS_FL). (deprecated)
      //  0x01000000    Inode is a snapshot (EXT4_SNAPFILE_FL). (not in mainline)
      //  0x04000000    Snapshot is being deleted (EXT4_SNAPFILE_DELETED_FL). (not in mainline)
      //  0x08000000    Snapshot shrink has completed (EXT4_SNAPFILE_SHRUNK_FL). (not in mainline)
      //  0x10000000    Inode has inline data (EXT4_INLINE_DATA_FL).
      //  0x20000000    Create children with the same project ID (EXT4_PROJINHERIT_FL).
      //  0x80000000    Reserved for ext4 library (EXT4_RESERVED_FL).
      // Aggregate flags:
      //  0x4BDFFF      User-visible flags.
      //  0x4B80FF      User-modifiable flags. Note that while EXT4_JOURNAL_DATA_FL and EXT4_EXTENTS_FL can be set with setattr, they are not in the kernel's EXT4_FL_USER_MODIFIABLE mask, since it needs to handle the setting of these flags in a special manner and they are masked out of the set of flags that are saved directly to i_flags.
      uint32_t  flags;

      union // linux1, hurd1, masix1
      {
        uint32_t  l_version;
        uint32_t  h_translator;
        uint32_t  m_reserved;
      } osd1;

      // Block map or extent tree
      uint32_t  block[EXT4_N_BLOCKS];

      uint32_t  generation; // File version for NFS
      uint32_t  file_acl_lo; // Extended attribs (lower 32)
      uint32_t  size_high;  // File size (upper 32) OR dir_acl
      uint32_t  obso_faddr; // (Obsolete) fragment address

      uint8_t   osd2[12];

      uint16_t  extra_isize;
      uint16_t  checksum_hi; // Checksum (upper 16)
      uint32_t  ctime_extra;
      uint32_t  mtime_extra;
      uint32_t  atime_extra;

      uint32_t  crtime;     // File creation time
      uint32_t  crtime_extra; // (upper 32)

      uint32_t  version_hi; // Version number (upper 32)
      uint32_t  projid;     // Project ID for quota API

    } __attribute__((packed));

    struct extent_header
    {
      uint16_t  magic;   // 0xF30A
      uint16_t  entries; // current # entries following header
      uint16_t  max_entries;
      uint16_t  depth;   // depth of this node in extent tree
      uint32_t  generation;
    };

    struct extent_idx
    {
      uint32_t  block;  // inode covers file blocks from @block onward
      uint32_t  leaf_lo; // block number of the extent node
      uint16_t  leaf_hi; // that is the next lower level in the tree
      uint16_t  unused;
    };

    struct extent
    {
      uint32_t  block;
      uint16_t  len;
      uint16_t  start_hi; // upper!
      uint32_t  start_lo; // lower!
    };

    struct extent_tail
    {
      // Checksum of the extent block,
      // crc32c(uuid+inum+igeneration+extentblock)
      uint32_t  checksum;
    };

    // initialize filesystem by providing base sector
    void init(const void* base_sector);

    // tree traversal
    typedef std::function<void(bool, uint64_t, dirvec_t)> cluster_func;
    void traverse(std::shared_ptr<Path> path, cluster_func callback);

    // device we can read and write sectors to
    hw::Drive& device;

    // system fields

  };

} // fs

#endif
