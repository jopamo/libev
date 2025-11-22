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

  /* Check that supported backends are a superset of baseline */
  unsigned int local_backends = ev_supported_backends();
  unsigned int baseline_backends = base.ev_supported_backends();

  if ((local_backends & baseline_backends) != baseline_backends)
    die_msg("local implementation does not support all baseline backends");

  /* Check watcher initialization */
  ev_io local_io_watcher;
  ev_io_init(&local_io_watcher, NULL, -1, 0);

  /* The priority should be initialized to 0 */
  if (local_io_watcher.priority != 0)
    die_msg("local ev_io watcher priority not initialized to 0");

  ev_timer local_timer_watcher;
  ev_timer_init(&local_timer_watcher, NULL, 0.0, 0.0);

  /* The priority should be initialized to 0 */
  if (local_timer_watcher.priority != 0)
    die_msg("local ev_timer watcher priority not initialized to 0");

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
