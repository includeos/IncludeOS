#ifndef FILESYSTEM_IN_USERSPACE_HPP
#define FILESYSTEM_IN_USERSPACE_HPP

#include <algorithm>

/* FUSE opcodes copied directly from libfuse */
enum fuse_opcode {
	FUSE_LOOKUP          = 1,
	FUSE_FORGET          = 2,  /* no reply */
	FUSE_GETATTR         = 3,
	FUSE_SETATTR         = 4,
	FUSE_READLINK        = 5,
	FUSE_SYMLINK         = 6,
	FUSE_MKNOD           = 8,
	FUSE_MKDIR           = 9,
	FUSE_UNLINK          = 10,
	FUSE_RMDIR           = 11,
	FUSE_RENAME          = 12,
	FUSE_LINK            = 13,
	FUSE_OPEN            = 14,
	FUSE_READ            = 15,
	FUSE_WRITE           = 16,
	FUSE_STATFS          = 17,
	FUSE_RELEASE         = 18,
	FUSE_FSYNC           = 20,
	FUSE_SETXATTR        = 21,
	FUSE_GETXATTR        = 22,
	FUSE_LISTXATTR       = 23,
	FUSE_REMOVEXATTR     = 24,
	FUSE_FLUSH           = 25,
	FUSE_INIT            = 26,
	FUSE_OPENDIR         = 27,
	FUSE_READDIR         = 28,
	FUSE_RELEASEDIR      = 29,
	FUSE_FSYNCDIR        = 30,
	FUSE_GETLK           = 31,
	FUSE_SETLK           = 32,
	FUSE_SETLKW	         = 33,
	FUSE_ACCESS	         = 34,
	FUSE_CREATE	         = 35,
	FUSE_INTERRUPT       = 36,
	FUSE_BMAP            = 37,
	FUSE_DESTROY         = 38,
	FUSE_IOCTL           = 39,
	FUSE_POLL            = 40,
	FUSE_NOTIFY_REPLY    = 41,
	FUSE_BATCH_FORGET    = 42,
	FUSE_FALLOCATE       = 43,
	FUSE_READDIRPLUS     = 44,
	FUSE_RENAME2         = 45,
	FUSE_LSEEK           = 46,
	FUSE_COPY_FILE_RANGE = 47,
	FUSE_SETUPMAPPING    = 48,
	FUSE_REMOVEMAPPING   = 49,
	FUSE_SYNCFS          = 50,
	FUSE_TMPFILE         = 51,
	FUSE_STATX           = 52,

	/* CUSE specific operations */
	CUSE_INIT		= 4096,

	/* Reserved opcodes: helpful to detect structure endian-ness */
	CUSE_INIT_BSWAP_RESERVED	= 1048576,	/* CUSE_INIT << 8 */
	FUSE_INIT_BSWAP_RESERVED	= 436207616,	/* FUSE_INIT << 24 */
};

typedef uint64_t fuse_ino_t;

/* Flags copied directly from  */
/* Main flags */
#define FUSE_ASYNC_READ		(1 << 0)
#define FUSE_POSIX_LOCKS	(1 << 1)
#define FUSE_FILE_OPS		(1 << 2)
#define FUSE_ATOMIC_O_TRUNC	(1 << 3)
#define FUSE_EXPORT_SUPPORT	(1 << 4)
#define FUSE_BIG_WRITES		(1 << 5)
#define FUSE_DONT_MASK		(1 << 6)
#define FUSE_SPLICE_WRITE	(1 << 7)
#define FUSE_SPLICE_MOVE	(1 << 8)
#define FUSE_SPLICE_READ	(1 << 9)
#define FUSE_FLOCK_LOCKS	(1 << 10)
#define FUSE_HAS_IOCTL_DIR	(1 << 11)
#define FUSE_AUTO_INVAL_DATA	(1 << 12)
#define FUSE_DO_READDIRPLUS	(1 << 13)
#define FUSE_READDIRPLUS_AUTO	(1 << 14)
#define FUSE_ASYNC_DIO		(1 << 15)
#define FUSE_WRITEBACK_CACHE	(1 << 16)
#define FUSE_NO_OPEN_SUPPORT	(1 << 17)
#define FUSE_PARALLEL_DIROPS    (1 << 18)
#define FUSE_HANDLE_KILLPRIV	(1 << 19)
#define FUSE_POSIX_ACL		(1 << 20)
#define FUSE_ABORT_ERROR	(1 << 21)
#define FUSE_MAX_PAGES		(1 << 22)
#define FUSE_CACHE_SYMLINKS	(1 << 23)
#define FUSE_NO_OPENDIR_SUPPORT (1 << 24)
#define FUSE_EXPLICIT_INVAL_DATA (1 << 25)
#define FUSE_MAP_ALIGNMENT	(1 << 26)
#define FUSE_SUBMOUNTS		(1 << 27)
#define FUSE_HANDLE_KILLPRIV_V2	(1 << 28)
#define FUSE_SETXATTR_EXT	(1 << 29)
#define FUSE_INIT_EXT		(1 << 30)
#define FUSE_INIT_RESERVED	(1 << 31)

/* Extra flags */
#define FUSE_SECURITY_CTX	(1ULL << 32)
#define FUSE_HAS_INODE_DAX	(1ULL << 33)
#define FUSE_CREATE_SUPP_GROUP	(1ULL << 34)
#define FUSE_HAS_EXPIRE_ONLY	(1ULL << 35)
#define FUSE_DIRECT_IO_ALLOW_MMAP (1ULL << 36)
#define FUSE_PASSTHROUGH	(1ULL << 37)
#define FUSE_NO_EXPORT_SUPPORT	(1ULL << 38)
#define FUSE_HAS_RESEND		(1ULL << 39)

#define FUSE_ROOT_ID 1ULL

typedef struct __attribute__((packed)) fuse_in_header {
	uint32_t len;       /* Total size of the data, including this header */
	uint32_t opcode;    /* The kind of operation (see below) */
	uint64_t unique;    /* A unique identifier for this request */
	uint64_t nodeid;    /* ID of the filesystem object being operated on */
	uint32_t uid;       /* UID of the requesting process */
	uint32_t gid;       /* GID of the requesting process */
	uint32_t pid;       /* PID of the requesting process */
	uint32_t padding;

	fuse_in_header(uint32_t plen, uint32_t opcod, uint64_t uniqu, uint64_t nodei)
	: len(sizeof(fuse_in_header) + plen), opcode(opcod), unique(uniqu), nodeid(nodei),
	  uid(0), gid(0), pid(0), padding(0)
	{}
} fuse_in_header;

typedef struct __attribute__((packed)) {
	uint32_t len;       /* Total size of data written to the file descriptor */
	int32_t  error;     /* Any error that occurred (0 if none) */
	uint64_t unique;    /* The value from the corresponding request */
} fuse_out_header;

typedef struct __attribute__((packed)) fuse_init_in {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
	uint32_t flags2;
	uint32_t unused[11];

	fuse_init_in(uint32_t majo, uint32_t mino) :
    major(majo), minor(mino), max_readahead(0),
    flags(FUSE_INIT_EXT | FUSE_SUBMOUNTS), flags2(0)
	{
		std::fill_n(unused, 11, 0);
	}
} fuse_init_in;

typedef struct __attribute__((packed)) {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
	uint16_t max_background;
	uint16_t congestion_threshold;
	uint32_t max_write;
	uint32_t time_gran;
	uint32_t unused[9];
} fuse_init_out;

/* Ported from VirtioFSD daemon start */
typedef struct {
    uint64_t ino;
    uint64_t size;
    uint64_t blocks;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint32_t atimensec;
    uint32_t mtimensec;
    uint32_t ctimensec;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint32_t rdev;
    uint32_t blksize;
    uint32_t flags;
} stat;

typedef struct {
	fuse_ino_t ino;
	uint64_t generation;
	uint64_t entry_valid;
	uint64_t attr_valid;
	uint32_t entry_valid_nsec;
	uint32_t attr_valid_nsec;
	stat attr;
} fuse_entry_param;
/* Ported from Rust VirtioFSD daemon end */

typedef struct __attribute__((packed)) fuse_open_in {
	uint32_t flags;
	uint32_t open_flags;

	fuse_open_in(uint32_t flag, uint32_t open_flag)
	: flags(flag), open_flags(open_flag) {}
} fuse_open_in;

typedef struct __attribute__((packed)) {
	uint64_t fh;
	uint32_t open_flags;
	int32_t	backing_id;
} fuse_open_out;

typedef struct __attribute__((packed)) fuse_read_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t size;
	uint32_t read_flags;
	uint64_t lock_owner;
	uint32_t flags;
	uint32_t padding;

	fuse_read_in(uint64_t f, uint64_t offse, uint32_t siz, 
		uint32_t read_flag, uint32_t flag)
	: fh(f), offset(offse), size(siz), read_flags(read_flag),
	  lock_owner(0), flags(flag), padding(0) {}
} fuse_read_in;

typedef struct __attribute__((packed)) fuse_write_in {
  uint64_t fh;
  uint64_t offset;
  uint32_t size;
  uint32_t write_flags;
  uint64_t lock_owner;
  uint32_t flags;
  uint32_t padding;

	fuse_write_in(uint64_t f, uint64_t offse, uint32_t siz, 
		uint32_t write_flag, uint32_t flag)
	: fh(f), offset(offse), size(siz), write_flags(write_flag),
	  lock_owner(0), flags(flag), padding(0) {}
};

typedef struct __attribute__((packed)) fuse_write_out {
	uint32_t size;
	uint32_t padding;
};

typedef struct __attribute__((packed)) fuse_release_in {
	uint64_t fh;
	uint32_t flags;
	uint32_t release_flags;
	uint64_t lock_owner;
	fuse_release_in(uint64_t f, uint32_t flag, uint32_t release_flag) 
	: fh(f), flags(flag), release_flags(release_flag) {}
} fuse_release_in;

#endif // FILESYSTEM_IN_USERPSACE_HPP
