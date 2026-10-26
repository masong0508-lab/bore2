#pragma once
// stub of newlib's sys/iosupport.h: just what libnds' console and keyboard name (no device files in this build)
#include <stddef.h>
#include <sys/types.h>
struct _reent; struct stat;
typedef struct { const char *name; size_t structSize;
  int (*open_r)(struct _reent*, void*, const char*, int, int); int (*close_r)(struct _reent*, void*);
  ssize_t (*write_r)(struct _reent*, void*, const char*, size_t); ssize_t (*read_r)(struct _reent*, void*, char*, size_t);
  off_t (*seek_r)(struct _reent*, void*, off_t, int); int (*fstat_r)(struct _reent*, void*, struct stat*); } devoptab_t;
extern const devoptab_t *devoptab_list[];
int AddDevice(const devoptab_t *d);
#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2
#define __SYSCALL(name) __syscall_##name
