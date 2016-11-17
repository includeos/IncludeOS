#include <service>
#include <info>
#include <ftw.h>
#include <cassert>
#include <errno.h>
#include <vector>


int display_info(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf);
int add_filesize(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf);
int add_items(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf);
bool is_before(const std::vector<std::string>& c, const char* item1, const char* item2);

static size_t total_size = 0;
static std::vector<std::string> items;

void ftw_tests()
{
  int res;

  printf("nftw /\n");
  res = nftw("/", add_items, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  auto directory_first = is_before(items, "/FOLDER1", "/FOLDER1/FOLDERA");
  CHECKSERT(directory_first, "nftw() visits a directory before the directory's files");

  items.clear();

  printf("nftw /\n");
  res = nftw("/", add_items, 20, FTW_PHYS | FTW_DEPTH);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  auto files_first = is_before(items, "/FOLDER1/FOLDERA", "/FOLDER1");
  CHECKSERT(files_first, "nftw() visits the directory's files before the directory when FTW_DEPTH is specified");

  res = nftw("MISSING_FILE", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  res = nftw("FILE1", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER1\n");
  res = nftw("/FOLDER1", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER2\n");
  res = nftw("/FOLDER2", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER3\n");
  res = nftw("/FOLDER3", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /\n");
  res = nftw("/", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /\n");
  res = nftw("/", add_filesize, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }
  printf("Total size: %ld\n", total_size);
}

int display_info(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf)
{
  printf("%ld\t%s (%d)\n", sb->st_size, fpath, flag);
#ifdef EXTRAVERBOSE
  printf("Base: %d\n", ftwbuf->base);
  printf("Level: %d\n", ftwbuf->level);
#endif
  return 0;
}

int add_filesize(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf)
{
  (void) fpath;
  (void) flag;
  (void) ftwbuf;
  total_size += sb->st_size;
  return 0;
}

int add_items(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf)
{
  (void) ftwbuf;
  (void) sb;
  (void) flag;
  items.push_back(fpath);
  return 0;
}

bool is_before(const std::vector<std::string>& c, const char* item1, const char* item2)
{
  auto i1 = std::find(std::cbegin(c), std::cend(c), item1);
  auto i2 = std::find(std::cbegin(c), std::cend(c), item2);
  if (i1 == std::cend(c) || i2 == std::cend(c))
  {
    throw std::domain_error("Alas, you cannot find what isn't there");
  }
  int pos1 = std::distance(begin(c), i1);
  int pos2 = std::distance(begin(c), i2);
  return pos1 < pos2;
}
