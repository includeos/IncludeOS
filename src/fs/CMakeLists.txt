
SET(SRCS
    disk.cpp
    filesystem.cpp
    dirent.cpp
    mbr.cpp
    path.cpp
    fat.cpp
    fat_async.cpp
    fat_sync.cpp
    memdisk.cpp
    )

add_library(fs OBJECT ${SRCS})
