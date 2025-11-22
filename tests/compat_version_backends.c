#include <stdio.h>
#include <stdlib.h>
#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  if (ev_version_major() != base.ev_version_major())
    die_msg("ev_version_major differs local vs baseline");
  if (ev_version_minor() != base.ev_version_minor())
    die_msg("ev_version_minor differs local vs baseline");

  unsigned sup_local = ev_supported_backends();
  unsigned sup_base = base.ev_supported_backends();
  if (sup_local != sup_base)
    die_msg("ev_supported_backends differs local vs baseline");

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
