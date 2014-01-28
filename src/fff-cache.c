#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "fff-common.h"

typedef struct fff_stats_s {
  size_t bytes;
  size_t paths;
  time_t start;
} *fff_stats_t;

int make_cache(int in_fd, int out_fd, fff_stats_t stats) {
  const int bufsize = 1024 * 1024;
  char *buf = NULL;               /* start of buffer */
  char *next_to_read = NULL;      /* first byte not processed already */
  char *first_null = NULL;        /* first null after next_to_read */
  char *terminating_null = NULL;  /* last null before garbage at the end of the buffer */
  size_t bytes_read = 0;
  size_t path_size = 0;

  if (!(buf = malloc(bufsize * sizeof(*buf)))) return ENOMEM;
  terminating_null = buf;
  next_to_read = buf;
  while((bytes_read = read(in_fd, terminating_null, bufsize - (terminating_null - buf)))) {
    assert(buf <= terminating_null);
    assert(terminating_null < buf + bufsize);
    assert(next_to_read <= terminating_null);
    assert(buf <= next_to_read);
    terminating_null += bytes_read;
    assert(terminating_null < buf + bufsize);
    *terminating_null = '\0';
    for (next_to_read = buf;
         terminating_null > (first_null = strchr(next_to_read, '\0'));
         next_to_read = first_null + 1) {
      path_size = first_null - next_to_read + 1;  /* +1 for terminal null */
      assert(next_to_read + path_size < buf + bufsize);
      write(out_fd, &path_size, sizeof(path_size));
      write(out_fd, next_to_read, path_size);
      stats->bytes += sizeof(path_size) + path_size;
      stats->paths += 1;

    }
    bytes_read = terminating_null - next_to_read;
    assert(bytes_read < bufsize - 2);
    memcpy(buf, next_to_read, bytes_read);
    terminating_null = buf + bytes_read;
  }
  return 0;
}

int print_stats(fff_stats_t stats) {
  time_t now = -1;
  if (0 > time(&now)) return errno;
  printf("%u bytes written, %d paths, in %u seconds\n",
         stats->bytes, stats->paths, now - stats->start);
  return 0;
}

void usage_and_exit(const char *program_name) {
  printf("usage: %s CACHEFILE\n"
         "\n"
         "takes a list of files on STDIN and writes the cache to CACHEFILE.\n",
         program_name);
  exit(1);
}

char* parse_args(int argc, char **argv) {
  return 2 == argc ? argv[1] : NULL;
}

void exit_error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  char *outfname;
  int out_fd = -1;
  int rv = 0;
  struct fff_stats_s stats = { 0, 0, 0 };

  if (0 > time(&stats.start)) exit_error("getting start time");

  if (!(outfname = parse_args(argc, argv))) usage_and_exit(argv[0]);

  /* CR dwang: open a temp file and atomically move it into place */
  out_fd = open(outfname, O_WRONLY | O_CREAT | O_TRUNC);
  if (out_fd < 0) exit_error("opening outfile");

  if ((rv = make_cache(STDIN_FILENO, out_fd, &stats))) {
    errno = rv;
    exit_error("making cache");
  }

  if ((rv = close(out_fd))) exit_error("closing cache");

  if ((rv = print_stats(&stats))) {
    errno = rv;
    exit_error("printing stats");
  }

  return 0;
}
