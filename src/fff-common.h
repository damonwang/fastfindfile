#ifndef FFF_COMMON_H
#include <stdlib.h>

typedef struct fff_path_s {
  size_t size;    /* size in bytes of path field, including terminal null */
  char   path[];  /* a null-terminated filename */
} *fff_path_t;

/* An fff-cache file is a concatenation of these structs.  The
   intended usage is to stat the file to find its total size, mmap it,
   and then walk through, examining every string. */

void exit_error(const char *msg) {
  perror(msg);
  exit(1);
}

#define FFF_COMMON_H
#endif

  
