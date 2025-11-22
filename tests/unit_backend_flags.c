#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef unsigned int (*backend_query_fn)(void);

#if defined(_WIN32)
typedef HMODULE baseline_lib_t;
#else
typedef void* baseline_lib_t;
#endif

static baseline_lib_t baseline_lib;
static backend_query_fn baseline_supported;
static backend_query_fn baseline_recommended;
static backend_query_fn baseline_embeddable;

static void die(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static char* xstrdup(const char* src) {
  if (!src)
    return NULL;

  size_t len = strlen(src) + 1;
  char* copy = (char*)malloc(len);
  if (!copy)
    die("malloc");

  memcpy(copy, src, len);
  return copy;
}

#if defined(_WIN32)
static baseline_lib_t load_baseline_library(const char* path) {
  HMODULE handle = LoadLibraryA(path);
  if (!handle) {
    fprintf(stderr, "LoadLibrary failed for %s (error=%lu)\n", path, GetLastError());
    exit(EXIT_FAILURE);
  }
  return handle;
}

static void unload_baseline_library(void) {
  if (baseline_lib)
    FreeLibrary(baseline_lib);
}

static void* load_symbol(const char* name) {
  FARPROC sym = GetProcAddress(baseline_lib, name);
  if (!sym) {
    fprintf(stderr, "GetProcAddress failed for %s (error=%lu)\n", name, GetLastError());
    exit(EXIT_FAILURE);
  }
  return (void*)sym;
}

static void set_libev_flags_env(const char* value) {
  if (value) {
    if (_putenv_s("LIBEV_FLAGS", value) != 0)
      die("failed to set LIBEV_FLAGS");
  } else {
    if (_putenv("LIBEV_FLAGS=") != 0)
      die("failed to unset LIBEV_FLAGS");
  }
}
#else
static baseline_lib_t load_baseline_library(const char* path) {
  void* handle = dlopen(path, RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "dlopen failed for %s: %s\n", path, dlerror());
    exit(EXIT_FAILURE);
  }
  return handle;
}

static void unload_baseline_library(void) {
  if (baseline_lib)
    dlclose(baseline_lib);
}

static void* load_symbol(const char* name) {
  dlerror(); /* clear */
  void* sym = dlsym(baseline_lib, name);
  const char* err = dlerror();
  if (!sym || err) {
    fprintf(stderr, "dlsym failed for %s: %s\n", name, err ? err : "unknown error");
    exit(EXIT_FAILURE);
  }
  return sym;
}

static void set_libev_flags_env(const char* value) {
  if (value) {
    if (setenv("LIBEV_FLAGS", value, 1) != 0) {
      perror("setenv");
      exit(EXIT_FAILURE);
    }
  } else if (unsetenv("LIBEV_FLAGS") != 0 && errno != ENOENT) {
    perror("unsetenv");
    exit(EXIT_FAILURE);
  }
}
#endif

static void restore_libev_flags_env(const char* value) {
  if (value)
    set_libev_flags_env(value);
  else
    set_libev_flags_env(NULL);
}

static void load_baseline_symbols(void) {
  const char* path = getenv("LIBEV_BASELINE_LIB");
  if (!path)
    die("LIBEV_BASELINE_LIB not set");

  baseline_lib = load_baseline_library(path);
  baseline_supported = (backend_query_fn)load_symbol("ev_supported_backends");
  baseline_recommended = (backend_query_fn)load_symbol("ev_recommended_backends");
  baseline_embeddable = (backend_query_fn)load_symbol("ev_embeddable_backends");
}

static unsigned int pick_backend(unsigned int supported, unsigned int avoid) {
  static const unsigned int candidates[] = {
      EVBACKEND_SELECT, EVBACKEND_POLL,   EVBACKEND_EPOLL, EVBACKEND_KQUEUE,
      EVBACKEND_PORT,   EVBACKEND_LINUXAIO, EVBACKEND_IOURING};

  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    unsigned int bit = candidates[i];
    if ((supported & bit) && bit != avoid)
      return bit;
  }

  return 0;
}

static struct ev_loop* must_new_loop(unsigned int flags) {
  struct ev_loop* loop = ev_loop_new(flags);
  if (!loop) {
    fprintf(stderr, "ev_loop_new failed for flags=0x%x\n", flags);
    exit(EXIT_FAILURE);
  }
  return loop;
}

int main(void) {
  load_baseline_symbols();

  unsigned int ours_supported = ev_supported_backends();
  unsigned int ours_recommended = ev_recommended_backends();
  unsigned int ours_embeddable = ev_embeddable_backends();

  if (ours_supported != baseline_supported())
    die("ev_supported_backends mismatch vs baseline");
  if (ours_recommended != baseline_recommended())
    die("ev_recommended_backends mismatch vs baseline");
  if (ours_embeddable != baseline_embeddable())
    die("ev_embeddable_backends mismatch vs baseline");

  const char* original_env = getenv("LIBEV_FLAGS");
  char* original_env_copy = xstrdup(original_env);

  set_libev_flags_env(NULL);
  unsigned int supported = ours_supported;
  if (!supported)
    die("no supported backends reported");

  unsigned int requested_backend = pick_backend(supported, 0);
  if (!requested_backend)
    die("failed to pick requested backend");

  struct ev_loop* loop = must_new_loop(requested_backend);
  unsigned int backend_without_env = ev_backend(loop);
  if (backend_without_env != requested_backend) {
    fprintf(stderr,
            "unexpected backend without env override: expected 0x%x got 0x%x\n",
            requested_backend, backend_without_env);
    exit(EXIT_FAILURE);
  }
  ev_loop_destroy(loop);

  unsigned int env_backend = pick_backend(supported, requested_backend);
  if (!env_backend) {
    fprintf(stderr,
            "only one backend (0x%x) available; skipping ENV override checks\n",
            requested_backend);
    restore_libev_flags_env(original_env_copy);
    unload_baseline_library();
    free(original_env_copy);
    return EXIT_SUCCESS;
  }

  char env_buf[32];
  snprintf(env_buf, sizeof(env_buf), "%u", env_backend);
  set_libev_flags_env(env_buf);

  loop = must_new_loop(requested_backend);
  unsigned int backend_with_env = ev_backend(loop);
  ev_loop_destroy(loop);
  if (backend_with_env != env_backend) {
    fprintf(stderr, "LIBEV_FLAGS failed to override backend: expected 0x%x got 0x%x\n",
            env_backend, backend_with_env);
    exit(EXIT_FAILURE);
  }

  loop = must_new_loop(requested_backend | EVFLAG_NOENV);
  unsigned int backend_with_noenv = ev_backend(loop);
  ev_loop_destroy(loop);
  if (backend_with_noenv != requested_backend) {
    fprintf(stderr,
            "EVFLAG_NOENV failed: backend expected 0x%x got 0x%x\n",
            requested_backend, backend_with_noenv);
    exit(EXIT_FAILURE);
  }

  restore_libev_flags_env(original_env_copy);
  unload_baseline_library();
  free(original_env_copy);
  return EXIT_SUCCESS;
}
