#ifndef EV_COMPAT_COMMON_H
#define EV_COMPAT_COMMON_H

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "ev.h"

/* function pointer types for the symbols we want to compare */
typedef int (*ev_version_major_fn)(void);
typedef int (*ev_version_minor_fn)(void);
typedef unsigned int (*ev_supported_backends_fn)(void);
typedef void (*ev_sleep_fn)(ev_tstamp);
typedef ev_tstamp (*ev_time_fn)(void);

/* struct holding both the handle and resolved symbols */
struct ev_compat_baseline {
  void* handle;
  ev_version_major_fn ev_version_major;
  ev_version_minor_fn ev_version_minor;
  ev_supported_backends_fn ev_supported_backends;
  ev_sleep_fn ev_sleep;
  ev_time_fn ev_time;
};

static void ev_compat_die(const char* msg) {
  fprintf(stderr, "compat error: %s\n", msg);
  exit(EXIT_FAILURE);
}

static void ev_compat_die_dl(const char* msg) {
  fprintf(stderr, "compat error: %s: %s\n", msg, dlerror());
  exit(EXIT_FAILURE);
}

static void ev_compat_load_baseline(struct ev_compat_baseline* b) {
  const char* path = getenv("LIBEV_BASELINE_LIB");
  if (!path || !*path)
    ev_compat_die("LIBEV_BASELINE_LIB not set in environment");

  b->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!b->handle)
    ev_compat_die_dl("dlopen baseline");

  dlerror(); /* clear any stale error */

  b->ev_version_major = (ev_version_major_fn)dlsym(b->handle, "ev_version_major");
  b->ev_version_minor = (ev_version_minor_fn)dlsym(b->handle, "ev_version_minor");
  b->ev_supported_backends = (ev_supported_backends_fn)dlsym(b->handle, "ev_supported_backends");
  b->ev_sleep = (ev_sleep_fn)dlsym(b->handle, "ev_sleep");
  b->ev_time = (ev_time_fn)dlsym(b->handle, "ev_time");

  const char* err = dlerror();
  if (err)
    ev_compat_die_dl(err);
}

static void ev_compat_unload_baseline(struct ev_compat_baseline* b) {
  if (b->handle) {
    dlclose(b->handle);
    b->handle = NULL;
  }
}

#endif /* EV_COMPAT_COMMON_H */
