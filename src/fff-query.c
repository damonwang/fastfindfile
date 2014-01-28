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

void search_cache(fff_cache_t cache, char *query, int out_fd) {
  size_t offset = 0;
  fff_path_t path = NULL;
  char newline[] = "\n";
  const int newline_size = strlen(newline);
  char *remaining_query = NULL, *remaining_path = NULL;

  while(offset < cache->size) {
    path = (fff_path_t) (cache->data + offset);
    assert(offset + path->size < cache->size);
    for (remaining_query = query, remaining_path = path->path;
         *remaining_query && *remaining_path;
         ++remaining_path) {
      assert(remaining_path - path->path < path->size);
      if (*remaining_query == *remaining_path) {
        ++remaining_query;
      }
    }
    if(!*remaining_query) {
      write(out_fd, &path->path, path->size - 1);
      write(out_fd, newline, newline_size);
    }
    offset += path->size + sizeof(path->size);
  }

}

void usage_and_exit(const char *program_name) {
  printf("usage: %s CACHEFILE QUERY\n"
         "\n"
         "prints all paths in CACHEFILE matching QUERY",
         program_name);
  exit(1);
}

struct args_s {
  char *infname, *query;
};

int parse_args(int argc, char **argv, struct args_s *args) {
  if (3 == argc) {
    args->infname = argv[1];
    args->query = argv[2];
    return 0;
  } else return 1;
}

int main(int argc, char **argv) {
  struct args_s args;
  struct fff_cache_s cache = { 0, NULL };
  struct stat inf_stat;
  int rv = 0;
  int in_fd = -1;

  if (parse_args(argc, argv, &args)) usage_and_exit(argv[0]);

  if(stat(args.infname, &inf_stat)) exit_error("stat'ing cache file");
  cache.size = inf_stat.st_size;

  in_fd = open(args.infname, O_RDONLY);
  if (in_fd < 0) exit_error("opening cache file");

  cache.data = mmap(NULL,
                    cache.size,
                    PROT_READ,
                    MAP_PRIVATE | MAP_POPULATE,
                    in_fd,
                    0);
  if (MAP_FAILED == cache.data) exit_error("mmap cache file");

  search_cache(&cache, args.query, STDOUT_FILENO);

  return 0;
}
