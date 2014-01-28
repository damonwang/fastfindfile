#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "fff-common.h"

typedef struct fff_cache_s {
  size_t size;
  void *data;
} *fff_cache_t;


int dump_cache(fff_cache_t cache, int out_fd) {
  size_t offset = 0;
  fff_path_t path = NULL;
  char newline[] = "\n";
  const int newline_size = strlen(newline);

  while(offset < cache->size) {
    path = (fff_path_t) (cache->data + offset);
    assert(offset + path->size < cache->size);
    write(out_fd, &path->path, path->size - 1);
    write(out_fd, newline, newline_size);
    offset += path->size + sizeof(path->size);
  }
  return 0;
}

void usage_and_exit(const char *program_name) {
  printf("usage: %s CACHEFILE\n"
         "\n"
         "dumps the contents of CACHEFILE to stdout\n",
         program_name);
  exit(1);
}

char* parse_args(int argc, char **argv) {
  return 2 == argc ? argv[1] : NULL;
}

int main(int argc, char **argv) {
  char *infname = NULL;
  int in_fd = -1;
  int rv = 0;
  struct stat inf_stat;
  struct fff_cache_s cache = { 0, NULL };

  if (!(infname = parse_args(argc, argv))) usage_and_exit(argv[0]);

  if(stat(infname, &inf_stat)) exit_error("stat'ing cache file");
  cache.size = inf_stat.st_size;

  in_fd = open(infname, O_RDONLY);
  if (in_fd < 0) exit_error("opening cache file");

  cache.data = mmap(NULL,
                    cache.size,
                    PROT_READ,
                    MAP_PRIVATE | MAP_POPULATE,
                    in_fd,
                    0);
  if (MAP_FAILED == cache.data) exit_error("mmap cache file");

  dump_cache(&cache, STDOUT_FILENO);

  return 0;
}
