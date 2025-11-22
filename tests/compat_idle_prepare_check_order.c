#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ev.h"
#include "compat_common.h"

#define MAX_STEPS 10

static char seq_local[MAX_STEPS];
static int step_local = 0;

static void idle_cb_local(EV_P_ ev_idle* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_local[step_local++] = 'I';
  ev_idle_stop(EV_A_ w);
  ev_break(EV_A_ EVBREAK_ONE);
}

static void prepare_cb_local(EV_P_ ev_prepare* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_local[step_local++] = 'P';
}

static void check_cb_local(EV_P_ ev_check* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_local[step_local++] = 'C';
}

static char seq_base[MAX_STEPS];
static int step_base = 0;

static void idle_cb_base(EV_P_ ev_idle* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_base[step_base++] = 'I';
  ev_idle_stop(EV_A_ w);
  ev_break(EV_A_ EVBREAK_ONE);
}

static void prepare_cb_base(EV_P_ ev_prepare* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_base[step_base++] = 'P';
}

static void check_cb_base(EV_P_ ev_check* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  seq_base[step_base++] = 'C';
}

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static void check_sequence(const char* label, const char* seq) {
  if (strchr(seq, 'P') == NULL || strchr(seq, 'C') == NULL || strchr(seq, 'I') == NULL)
    die_msg(label);
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  struct ev_loop* loop_local = ev_loop_new(0);
  seq_local[0] = '\0';
  step_local = 0;
  ev_prepare prep;
  ev_check chk;
  ev_idle idl;
  ev_prepare_init(&prep, prepare_cb_local);
  ev_check_init(&chk, check_cb_local);
  ev_idle_init(&idl, idle_cb_local);
  ev_prepare_start(loop_local, &prep);
  ev_check_start(loop_local, &chk);
  ev_idle_start(loop_local, &idl);
  ev_run(loop_local, 0);
  if (loop_local)
    ev_loop_destroy(loop_local);
  seq_local[step_local] = '\0';

  struct ev_loop* loop_base = ev_loop_new(0);
  seq_base[0] = '\0';
  step_base = 0;
  ev_prepare_init(&prep, prepare_cb_base);
  ev_check_init(&chk, check_cb_base);
  ev_idle_init(&idl, idle_cb_base);
  ev_prepare_start(loop_base, &prep);
  ev_check_start(loop_base, &chk);
  ev_idle_start(loop_base, &idl);
  ev_run(loop_base, 0);
  if (loop_base)
    ev_loop_destroy(loop_base);
  seq_base[step_base] = '\0';

  check_sequence("local sequence", seq_local);
  check_sequence("baseline sequence", seq_base);

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
