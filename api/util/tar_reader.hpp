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
#ifndef TAR_READER_HPP
#define TAR_READER_HPP

#include "tar_header.hpp"

// Temp:
// Edit:
#include <string>
#include <memory>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>  // memcpy
#include <time.h>   // time_t
#include <cstdlib>  // exit
// #include <gsl/gsl>
#include <sys/stat.h>
#include <stdexcept>

#include <sstream>

/*
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
*/

/*struct Header_old_tar {
	char name[100];      // File name                    // Offset 0
	char mode[8];        // File mode                    // Offset 100
	char uid[8];         // Owner's numeric user ID      // Offset 108
	char gid[8];         // Group's numeric user ID      // Offset 116
	char size[12];       // File size in bytes           // Offset 124
	char mtime[12];      // Last modification time in numeric Unix time format   // Offset 136
	char checksum[8];    // Checksum for header record   // Offset 148
	char linkflag[1];    // Link indicator (file type)   // Offset 156
	char linkname[100];  // Name of linked file          // Offset 157
	char pad[255];
};*/
// All unused bytes in the header record are filled with nulls.
// More info about each field: https://github.com/libarchive/libarchive/blob/master/libarchive/tar.5

// ustar (unix standard tar)
// Extended the struct with new fields:
// = 512 characters/bytes
/*struct Header_posix_ustar {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];		// new
	// char linkflag[1];	// removed
	char linkname[100];
	char magic[6];			// new
	char version[2];		// new
	char uname[32];			// new
	char gname[32];			// new
	char devmajor[8];		// new
	char devminor[8];		// new
	char prefix[155];		// new
	char pad[12];			  // new
	// char pad[255];		// removed
};*/
// Currently most tar implementations comply with the ustar format, occasionally extending it by
// adding new fields to the blank area at the end of the header record.

// GNU tar
// ...

// ---------------------------------- archive/tar (Go) ---------------------------------------------------

// Used by Mender (Go programming language: import archive/tar):
// Package tar implements access to tar archives. It aims to cover most of the variations, including
// those produced by GNU and BSD tars.

// Constants:
/* The type flag field options
const char TypeReg = '0';           // Regular file
const char TypeRegA = '\x00';       // Regular file
const char TypeLink = '1';		      // Hard link
const char TypeSymlink = '2';	      // Symbolic link
const char TypeChar = '3';		      // Character device node
const char TypeBlock = '4';         // Block device node
const char TypeDir = '5';           // Directory
const char TypeFifo = '6';          // Fifo node
const char TypeCont = '7';          // Reserved
const char TypeXHeader = 'x';       // Extended header
const char TypeXGlobalHeader = 'g'; // Global extended header
const char TypeGNULongName = 'L';   // Next file has a long name
const char TypeGNULongLink = 'K';   // Next file symlinks to a file with a long name
const char TypeGNUSparse = 'S';     // Sparse file
*/

/*
	Error variables:

	ErrWriteTooLong - create error or exception with text "archive/tar: Write too long"
	ErrFieldTooLong - create error or exception with text "archive/tar: Header field too long"
	ErrWriteAfterClose - create error or exception with text "archive/tar: Write after close"

	ErrHeader - create error or exception with text "archive/tar: Invalid tar header"
*/

/*
  Every file starts with a 512 byte header
*/

const int SECTOR_SIZE = 512;

class Tar_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

struct __attribute__((packed)) Content_block {
  char block[SECTOR_SIZE];
};

class Tar {

  // using Span = gsl::span<Content_block>;
  // using Span_iterator = gsl::span<Content_block>::iterator;

public:
  // TODO Temp: content to span (gsl) later: (by value ok)
  // Not necessarily Content_block* here
  Tar(const std::vector<Tar_header*> headers, const std::vector<Content_block*> content)
    : headers_{headers}, content_{content} {

    // First header says size of first file and so on

  }

  ~Tar() {
    printf("Tar destructor\n");
    /*for (auto header : headers_) {
      delete header;
    }*/
  }

  std::vector<Content_block*> get_content(const std::string& path) {
    for (auto header : headers_) {
      if (std::string{header->name} == path) {
        printf("Found path\n");

        return std::vector<Content_block*>{content_.begin() + header->first_block_index, content_.begin() + (header->first_block_index + header->num_content_blocks)};
      }
    }

    throw Tar_exception(std::string{"Path " + path + " doesn't exist"});
  }

  Tar_header* get_header(const std::string& path) {
    for (auto header : headers_) {
      if (std::string{header->name} == path) {
        printf("Found header\n");

        return header;
      }
    }

    throw Tar_exception(std::string{"Path " + path + " doesn't exist"});
  }

  // add

private:
  std::vector<Tar_header*> headers_;    // Doesn't have to be in contiguous memory

  // TODO Temp: To span (gsl) later (not necessarily pointer here):
  std::vector<Content_block*> content_;

};  // class Tar

class Tar_reader {

public:
  void read(const std::string& tar_filename) {

    // Get tar over network f.ex. - just load to memory/ram and expect
    // Buffer
    // vector of Headers
    // span (gsl) of blocks
    // Ok to use new if close/delete

    if (tar_filename == "")
      throw Tar_exception("Invalid filename: Filename is empty");

    if (tar_filename.substr(tar_filename.size() - 4) not_eq ".tar")
      throw Tar_exception("Invalid file suffix: This is not a .tar file");

    // Add support for mender and tar.gz too

    // Test if file exists

    struct stat stat_tar;

    if (stat(tar_filename.c_str(), &stat_tar) == -1)
      throw Tar_exception(std::string{"Unable to open tar file " + tar_filename});

    printf("Filename: %s\n", tar_filename.c_str());
    printf("Size of tar file (stat): %ld\n", stat_tar.st_size);

    if (stat_tar.st_size % SECTOR_SIZE not_eq 0)
      throw Tar_exception("Invalid size of tar file");

    // std::vector<char> buf(stat_tar.st_size);
    // auto* buf_head = buf.data();
    char buf[stat_tar.st_size];
    auto* buf_head = buf;

    std::ifstream tar_stream{tar_filename}; // Load into memory
    auto read_bytes = tar_stream.read(buf_head, stat_tar.st_size).gcount();

    printf("-------------------------------------\n");
    printf("Read %ld bytes from tar file\n", read_bytes);
    printf("So this tar file consists of %ld sectors\n", (read_bytes / SECTOR_SIZE));

/*
    printf("Sector 13:\n");
    for (int i = SECTOR_SIZE*12; i < SECTOR_SIZE*13; i++)
      printf("%c", buf[i]);
    printf("\n");
*/

    /*
    std::string name{buf.begin(), buf.begin() + 100};
    h.set_name(name);
    printf("Name: %s\n", h.name().c_str());

    int64_t mode;
    memcpy(&mode, &buf[100], sizeof(int64_t));
    h.set_mode(mode);
    printf("Mode: %ld\n", h.mode());

    int uid;
    memcpy(&uid, &buf[108], 8);
    h.set_uid(uid);
    printf("UID: %d\n", h.uid());

    int gid;
    memcpy(&gid, &buf[116], 8);
    h.set_gid(gid);
    printf("GID: %d\n", gid);

    int64_t s;
    memcpy(&s, &buf[124], 12);
    h.set_size(s);
    printf("Size: %ld\n", h.size());

    time_t rawtime;
    struct tm* timeinfo;
    char buffer[12];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime (buffer, 12, "%c", ); //timeinfo);
    std::string mod_time{buffer};

    h.set_mod_time(mod_time);
    printf("%s\n", h.mod_time().c_str());
    */

    // TODO Temp: To span (gsl) later (contiguous memory)
    std::vector<Content_block*> blocks;

    std::vector<Tar_header*> headers;

    // Go through the whole buffer/tar file block by block
    for ( long i = 0; i < stat_tar.st_size; i += SECTOR_SIZE) {

      // One header or content at a time (one block)

      printf("----------------- New header -------------------\n");

      Tar_header* h = (Tar_header*) (buf + i);

      if (strcmp(h->name, "") == 0) {
        printf("Empty header name: Continue\n");
        continue;
      }

      //strcpy(h->name, std::string{buf.begin() + i + OFFSET_NAME, buf.begin() + i + OFFSET_NAME + LENGTH_NAME}.c_str());

      printf("Name: %s\n", h->name);
      printf("Mode: %s\n", h->mode);
      printf("UID: %s\n", h->uid);
      printf("GID: %s\n", h->gid);
      printf("Filesize: %s\n", h->size);
      printf("Mod_time: %s\n", h->mod_time);
      printf("Checksum: %s\n", h->checksum);
      printf("Linkname: %s\n", h->linkname);
      printf("Magic: %s\n", h->magic);
      printf("Version: %s\n", h->version);
      printf("Uname: %s\n", h->uname);
      printf("Gname: %s\n", h->gname);
      printf("Devmajor: %s\n", h->devmajor);
      printf("Devminor: %s\n", h->devminor);
      printf("Prefix: %s\n", h->prefix);
      printf("Pad: %s\n", h->pad);
      printf("Typeflag: %c\n", h->typeflag);

      // Check if this is a directory or not (typeflag) (Directories have no content)

      if (h->typeflag not_eq DIRTYPE) {  // Is not a directory so has content

        // Next block is first content block and we need to subtract number of headers (are incl. in i)
        h->first_block_index = (i / SECTOR_SIZE) - headers.size();
        printf("First block index: %d\n", h->first_block_index);

        // Get the size of this file in the tarball
        char* pEnd;
        long int filesize;
        filesize = strtol(h->size, &pEnd, 8); // h->size is octal
        printf("Filesize decimal: %ld\n", filesize);

        int num_content_blocks = 0;
        if (filesize % SECTOR_SIZE not_eq 0)
          num_content_blocks = (filesize / SECTOR_SIZE) + 1;
        else
          num_content_blocks = (filesize / SECTOR_SIZE);
        printf("Num content blocks: %d\n", num_content_blocks);

        h->num_content_blocks = num_content_blocks;

        for (int j = 0; j < num_content_blocks; j++) {

          i += SECTOR_SIZE; // Go to next block with content

          Content_block* content_blk = (Content_block*) (buf + i);
          /*Content_block content_blk;
          snprintf(content_blk.block, SECTOR_SIZE, "%s", (buf + i));
          printf("For-loop Content block: %s\n", content_blk.block);*/
          printf("Content block:\n%s\n", content_blk->block);

          /* The data/content is there:
          printf("And for-loop char by char:\n");
          for (int k = i; k < (i+SECTOR_SIZE); k++) {
            printf("%c", buf[k]);
          }
          printf("\n");*/

          // NB: Doesn't stop at 512 - writes the rest as well (until the file's end)

          blocks.push_back(content_blk);  // TODO: GSL

          printf("For loop content blocks. i = %ld\n", i);
        }
      }

      printf("After for-loop with content: %ld content blocks\n", blocks.size());
      headers.push_back(h);
    }

    printf("Num headers: %ld\n", headers.size());
    printf("Num content blocks: %ld\n", blocks.size());

    tar_stream.close();

    // Testing:

    Tar tar{headers, blocks};

    Tar_header* h = tar.get_header("level_1_folder_1/Makefile");
    printf("Tar header for Makefile:\n");
    printf("Name: %s\n", h->name);
    printf("Mode: %s\n", h->mode);
    printf("Typeflag: %c\n", h->typeflag);
    printf("Size: %s\n", h->size);
    printf("Prefix: %s\n", h->prefix);
    printf("Pad: %s\n", h->pad);
    printf("First block index: %d\n", h->first_block_index);
    printf("Num content blocks: %d\n", h->num_content_blocks);

    std::vector<Content_block*> result = tar.get_content("level_1_folder_1/Makefile");
    printf("Content in Makefile:\n");

    for (Content_block* res : result)
      printf("Block: %s\n", res->block);

/* Mender

    // START CONTENT of info-file:
    printf("Content: ");
    for (int i = 512; i < 1024; i++)
      printf("%s", std::string{buf[i]}.c_str());
    printf("\n");

    // Next file (header.tar.gz):

    printf("--------------------------------------\n");

    printf("Header.tar.gz (compressed):\n");

    Header h2;

    std::string h2_name{buf.begin() + 1024, buf.begin() + (1024 + 100)};

    printf("Name: ");
    for (int i = 1024; i < 1124; i++) {
      printf("%s", std::string{buf[i]}.c_str());
    }
    printf("\n");

    h2.set_name(h2_name);
    printf("H2's name: %s\n", h2.name().c_str());

    // osv.

    // header.tar.gz's content is from 1536 to 2048

    // Next file (data):

    printf("--------------------------------------\n");

    // Data is a directory containing image files (one compressed tar file for each image)
    printf("Data:\n");

    Header h3;

    std::string h3_name{buf.begin() + 2048, buf.begin() + (2048 + 100)};

    h3.set_name(h3_name);

    printf("Name: %s\n", h3.name().c_str());

    // osv.
*/
    /*printf("Content: ");
    for (int i = 2560; i < 3072; i++) {
      printf("%s");
    }*/

    // Terminate

    // fclose(f);
  }

/* A Reader provides sequential access to the contents of a tar archive. A tar archive consists of
// a sequence of files. The next method advances to the next file in the archive (including the first),
// and then it can be treated as an io.Reader to access the file's data.
class Reader {
	// Contains filtered or unexported fields

public:
	Reader((io.Reader) r);
	Reader next(const Reader& tr);				// Advances to the next entry in the tar archive (io.EOF is returned at the end of the input)
	int read(const char* bytes);		// Reads from the current entry in the tar archive. It returns 0, io.EOF when it reaches the end of that entry,
										// until next() is called to advance to the next entry
										// Calling read() on special types like TypeLink, TypeSymLink, TypeChar, TypeBlock, TypeDir, and TypeFifo returns
										// 0, io.EOF regardless of what the Header.size claims

private:
  int64_t pad;    // Amount of padding (ignored) after current file entry
  char blk[];     // Buffer to use as temporary local storage
};

// A Writer provides sequential writing of a tar archive in POSIX.1 format. A tar archive consists of a sequence of files.
// Call write_header() to begin a new file, and then call write() to supply that file's data, writing at most hdr.size bytes in total.
struct Writer {
	// Contains filtered or unexported fields

	Writer((io.Writer) w);
	void close();							// Closes the tar archive, flushing any unwritten data to the underlying writer
	void flush();							// Finishes writing the current file (optional)
	int write(const char* bytes);			// Writes to the current entry in the tar archive. Returns the error ErrWriteTooLong if more than
											// hdr.size bytes are written after write_header()
	void write_header(const Header& hdr);	// Writes hdr and prepares to accept the file's contents. Calls flush() if it is not the first header
											// Calling after a close() will return ErrWriteAfterClose
};
*/


/* -------------------- libarchive -------------------- */
/*
struct tar {
	struct archive_string acl_text;
	struct archive_string entry_pathname;
	struct
};*/

/*static int archive_read_format_tar_read_data(const Archive_read& a, const void** buff, size_t size, int64_t offset) {


}*/
/*
static void read_tar_2(const char* filename) {

	struct archive_entry* ae;
	struct archive *a;

	a = archive_read_new();

	if (a != NULL) {

		// Fill/Create Header
		Header hdr{file_info, link};

    // ...

		archive_read_next_header(a, &ae);

		archive_read_close(a);

		archive_read_free(a);
	}
}*/


/* From libarchive examples */

/*
#include <sys/types.h>		// POSIX - IncludeOS option?
#include <sys/stat.h>		// POSIX - IncludeOS option?

#include <archive.h>		// libarchive - implement
#include <archive_entry.h>	// libarchive - implement

#include <fcntl.h>			// POSIX
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>			// POSIX

static int verbose = 0;

// C -> C++

static void create(const char* filename, int compress, const char** argv) {
	struct archive* a;
	struct archive_entry* entry;
	ssize_t len;
	int fd;

	a = archive_write_new();							// IMPLEMENT

	archive_write_add_filter_none(a);					// IMPLEMENT - necessary?

	if (filename not_eq NULL and strcmp(filename, "-") == 0)
		filename = NULL;

	archive_write_open_filename(a, filename);			// IMPLEMENT

	while (*argv not_eq NULL) {
		struct archive* disk = archive_read_disk_new();	// IMPLEMENT

		// archive_read_disk_set_standard_lookup(disk);

		int r = archive_read_disk_open(disk, *argv);	// IMPLEMENT

		if (r not_eq ARCHIVE_OK) {
			errmsg(archive_error_string(disk));
			errmsg("\n");
			exit(1);
		}

		while (true) {
			int needcr = 0;

			entry = archive_entry_new();				// IMPLEMENT
			r = archive_read_next_header2(disk, entry);	// IMPLEMENT

			if (r == ARCHIVE_EOF)
				break;

			if (r not_eq ARCHIVE_OK) {
				errmsg(archive_error_string(disk));
				errmsg("\n");
				exit(1);
			}

			archive_read_disk_descend(disk);		// IMPLEMENT

			if (verbose) {
				msg("a ");
				msg(archive_entry_pathname(entry));
				needcr = 1;
			}

			r = archive_write_header(a, entry);

			if (r < ARCHIVE_OK) {
				errmsg(": ");
				errrmsg(archive_error_string(a));
				needcr = 1;
			}

			if (r == ARCHIVE_FATAL)					// ADD CONSTANT
				exit(1);

			if (r > ARCHIVE_FAILED) {

				// Copy data into the target archive

				fd = open(archive_entry_sourcepath(entry), O_RDONLY);	// Where?	// IMPLEMENT
				len = read(fd, buff, sizeof(buff));						// Where?

				while(len > 0) {
					archive_write_data(a, buff, len);	// IMPLEMENT
					len = read(fd, buff, sizeof(buff));
				}

				close(fd);
			}

			archive_entry_free(entry);					// IMPLEMENT

			if (needcr)
				msg("\n");
		}

		archive_read_close(disk);
		archive_read_free(disk);
		argv++;
	}

	archive_write_close(a);
	archive_write_free(a);
}

static void extract(const char* filename, int do_extract, int flags) {
	struct archive* a;
	struct archive* ext;
	struct archive_entry* entry;
	int r;

	a = archive_read_new();                      // IMPLEMENT
	ext = archive_write_disk_new();              // IMPLEMENT
	archive_write_disk_set_options(ext, flags);  // IMPLEMENT

	// tar:
	archive_read_support_format_tar(a);          // IMPLEMENT

	if (filename != NULL and strcmp(filename, "-") == 0)
		filename = NULL;

	if (r = archive_read_open_filename(a, filename, 10240)) {	// IMPLEMENT
		errmsg(archive_error_string(a));						// IMPLEMENT
		errmsg("\n");
		exit(r);												// In cstdlib
	}

	while (true) {
		int needcr = 0;
		r = archive_read_next_header(a, &entry);	// IMPLEMENT

		if (r == ARCHIVE_EOF)               // ADD CONSTANT
			break;

		if (r not_eq ARCHIVE_OK) {          // ADD CONSTANT
			errmsg(archive_error_string(a));
			errmsg("\n");
			exit(1);
		}

		if (verbose and do_extract)
			msg("x ");

		if (verbose or (not do_extract)) {
			msg(archive_entry_pathname(entry));		// IMPLEMENT
			msg(" ");
			needcr = 1;
		}

		if (do_extract) {
			r = archive_write_header(ext, entry);	// IMPLEMENT

			if (r not_eq ARCHIVE_OK) {
				errmsg(archive_error_string(a));
				needcr = 1;
			} else {
				r = copy_data(a, ext);				// IMPLEMENT

				if (r not_eq ARCHIVE_OK)
					needcr = 1;
			}
		}

		if (needcr)
			msg("\n");
	}

	archive_read_close(a);						// IMPLEMENT
	archive_read_free(a);						  // IMPLEMENT
	exit(0);
}

static int copy_data(struct archive* ar, struct archive* aw) {
	int r;
	const void* buff;
	size_t size;
	int64_t offset;

	while (true) {
		r = archive_read_data_block(ar, &buff, &size, &offset);		// IMPLEMENT

		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);

		if (r not_eq ARCHIVE_OK) {
			errmsg(archive_error_string(ar));
			return (r);
		}

		r = archive_write_data_block(aw, buff, size, offset);		// IMPLEMENT

		if (r not_eq ARCHIVE_OK) {
			errmsg(archive_error_string(ar));
			return (r);
		}
	}
}

static void msg(const char* m) {
	write(1, m, strlen(m));				// where? stat?
}

static void errmsg(const char* m) {
	if (m == NULL)
		m = "Error: No error description provided.\n";

	write(2, m, strlen(m));
}

// static void usage();

*/

};  // < class Tar_reader

#endif
