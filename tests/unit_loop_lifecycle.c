#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ev.h"

static int sigusr1_hits;
static int child_hits;
static int default_cleanup_hits;
static int alt_cleanup_hits;
static pid_t observed_child_pid;

static void
die_errno(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void
die_msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

static void
cleanup_cb(EV_P_ ev_cleanup *w, int revents)
{
    (void)EV_A;
    (void)revents;

    int *counter = (int *)w->data;
    if (counter)
        ++(*counter);
}

static void
signal_cb(EV_P_ ev_signal *w, int revents)
{
    (void)revents;

    ++sigusr1_hits;
    ev_signal_stop(EV_A_ w);
    ev_break(EV_A_ EVBREAK_ALL);
}

static void
timeout_cb(EV_P_ ev_timer *w, int revents)
{
    (void)revents;

    const char *phase = (const char *)w->data;
    if (!phase)
        phase = "unknown";

    fprintf(stderr, "Timeout reached in %s test\n", phase);
    ev_break(EV_A_ EVBREAK_ALL);
}

static void
child_cb(EV_P_ ev_child *w, int revents)
{
    (void)revents;

    fprintf(stderr, "DEBUG: child_cb called: child_hits=%d, rpid=%d, rstatus=%d\n", 
            child_hits, w->rpid, w->rstatus);
    
    ++child_hits;
    observed_child_pid = w->rpid;

    if (!WIFEXITED(w->rstatus) || WEXITSTATUS(w->rstatus) != 42)
        die_msg("unexpected child exit status");

    fprintf(stderr, "DEBUG: child_cb breaking loop\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

static void
exercise_signal_handling(struct ev_loop *loop)
{
    sigusr1_hits = 0;

    ev_signal sigusr;
    ev_signal_init(&sigusr, signal_cb, SIGUSR1);
    ev_signal_start(loop, &sigusr);

    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 10.0, 0.); /* even more generous timeout */
    timeout_watcher.data = (void*)"signal handling";
    ev_timer_start(loop, &timeout_watcher);

    /* give the event loop a moment to set up the signal handler */
    ev_sleep(0.01); /* 10ms */

    if (raise(SIGUSR1) != 0)
        die_errno("raise");

    ev_run(loop, 0);

    ev_timer_stop(loop, &timeout_watcher);
    ev_signal_stop(loop, &sigusr);

    if (sigusr1_hits != 1)
        die_msg("signal callback did not run exactly once");
}

static void
exercise_child_handling(struct ev_loop *loop)
{
    child_hits = 0;
    observed_child_pid = 0;

    int sync_pipe[2];
    if (pipe(sync_pipe) < 0)
        die_errno("pipe");

    ev_child watcher;
    ev_child_init(&watcher, child_cb, 0 /* any child */, 0);
    ev_child_start(loop, &watcher);

    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 10.0, 0.); /* even more generous timeout */
    timeout_watcher.data = (void*)"child handling";
    ev_timer_start(loop, &timeout_watcher);

    pid_t child = fork();
    if (child < 0)
        die_errno("fork");

    if (child == 0) {
        close(sync_pipe[1]);
        char token;
        if (read(sync_pipe[0], &token, 1) < 0)
            _exit(EXIT_FAILURE);
        close(sync_pipe[0]);
        _exit(42);
    }

    close(sync_pipe[0]);
    if (write(sync_pipe[1], "x", 1) != 1)
        die_errno("write");
    close(sync_pipe[1]);

    ev_run(loop, 0);

    ev_timer_stop(loop, &timeout_watcher);
    ev_child_stop(loop, &watcher);

    if (child_hits != 1)
        die_msg("child callback did not run exactly once");
    if (observed_child_pid != child)
        die_msg("child callback reported unexpected pid");
}

int
main(void)
{
    struct ev_loop *default_loop = ev_default_loop(EVFLAG_AUTO);
    if (!default_loop)
        die_errno("ev_default_loop");

    if (!ev_is_default_loop(default_loop))
        die_msg("default loop is not flagged as default");

    struct ev_loop *alt_loop = ev_loop_new(EVFLAG_AUTO);
    if (!alt_loop)
        die_errno("ev_loop_new");

    if (ev_is_default_loop(alt_loop))
        die_msg("alternate loop incorrectly flagged as default");

    ev_cleanup default_cleanup;
    ev_cleanup_init(&default_cleanup, cleanup_cb);
    default_cleanup.data = &default_cleanup_hits;
    ev_cleanup_start(default_loop, &default_cleanup);

    ev_cleanup alt_cleanup;
    ev_cleanup_init(&alt_cleanup, cleanup_cb);
    alt_cleanup.data = &alt_cleanup_hits;
    ev_cleanup_start(alt_loop, &alt_cleanup);

    /* Test only on default loop for signal & child watchers */
    exercise_signal_handling(default_loop);
    exercise_child_handling(default_loop);

    ev_loop_destroy(alt_loop);
    if (alt_cleanup_hits != 1)
        die_msg("alternate loop cleanup watcher did not fire");

    ev_loop_destroy(default_loop);
    if (default_cleanup_hits != 1)
        die_msg("default loop cleanup watcher did not fire");

    struct ev_loop *recreated_default = ev_default_loop(EVFLAG_AUTO);
    if (!recreated_default)
        die_errno("ev_default_loop recreate");

    if (!ev_is_default_loop(recreated_default))
        die_msg("recreated default loop missing default flag");

    ev_loop_destroy(recreated_default);

    return EXIT_SUCCESS;
}
