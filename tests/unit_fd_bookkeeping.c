#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ev.h"

static void die(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

// Structure to hold test context
typedef struct {
    int* callback_count;
    int* pipe_fds;
} test_context_t;

static void io_cb(struct ev_loop* loop, ev_io* w, int revents) {
    (void)loop;
    (void)revents;
    test_context_t* ctx = (test_context_t*)w->data;
    (*ctx->callback_count)++;
    
    // Write to the pipe to potentially trigger another event
    if (write(ctx->pipe_fds[1], "x", 1) != 1) {
        die("write failed in callback");
    }
}

static void reentrant_io_cb(struct ev_loop* loop, ev_io* w, int revents) {
    (void)loop;
    (void)revents;
    test_context_t* ctx = (test_context_t*)w->data;
    (*ctx->callback_count)++;
    
    // Modify the watcher during callback to test reify behavior
    ev_io_stop(loop, w);
    ev_io_set(w, ctx->pipe_fds[0], EV_READ);
    ev_io_start(loop, w);
}

static void test_fd_event_and_reify(void) {
    struct ev_loop* loop = ev_loop_new(0);
    if (!loop)
        die("ev_loop_new failed");
    
    // Create local variables for this test
    int pipe_fds[2];
    int callback_count = 0;
    
    // Create a pipe for testing
    if (pipe(pipe_fds) == -1)
        die("pipe creation failed");
    
    // Set up context
    test_context_t ctx = { &callback_count, pipe_fds };
    
    ev_io watcher;
    ev_init(&watcher, io_cb);
    watcher.data = &ctx;
    
    ev_io_set(&watcher, pipe_fds[0], EV_READ);
    ev_io_start(loop, &watcher);
    
    // Trigger an event by writing to the pipe
    if (write(pipe_fds[1], "test", 4) != 4)
        die("write failed");
    
    // Run the loop to process the event - use EVRUN_ONCE to ensure events are processed
    ev_run(loop, EVRUN_ONCE);
    
    // The callback should have been called once
    if (callback_count != 1)
        die("callback should have been called exactly once");
    
    // Clean up
    ev_io_stop(loop, &watcher);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    ev_loop_destroy(loop);
}

static void test_reentrant_fd_modification(void) {
    struct ev_loop* loop = ev_loop_new(0);
    if (!loop)
        die("ev_loop_new failed");
    
    // Create local variables for this test
    int pipe_fds[2];
    int reentrant_callback_count = 0;
    
    // Create a new pipe for this test
    if (pipe(pipe_fds) == -1)
        die("pipe creation failed");
    
    // Set up context
    test_context_t ctx = { &reentrant_callback_count, pipe_fds };
    
    ev_io watcher;
    ev_init(&watcher, reentrant_io_cb);
    watcher.data = &ctx;
    
    ev_io_set(&watcher, pipe_fds[0], EV_READ);
    ev_io_start(loop, &watcher);
    
    // Trigger an event
    if (write(pipe_fds[1], "test", 4) != 4)
        die("write failed");
    
    // Run the loop - the callback will modify the watcher
    ev_run(loop, EVRUN_ONCE);
    
    // The reentrant callback should have been called once
    if (reentrant_callback_count != 1)
        die("reentrant callback should have been called exactly once");
    
    // Clean up
    ev_io_stop(loop, &watcher);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    ev_loop_destroy(loop);
}

static int timeout_called = 0;

static void timeout_cb(struct ev_loop* loop, ev_timer* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
  timeout_called = 1;
  ev_break(loop, EVBREAK_ALL);
}

static void test_reify_flag_handling(void) {
    struct ev_loop* loop = ev_loop_new(0);
    if (!loop)
        die("ev_loop_new failed");
    
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
        die("pipe creation failed");
    
    // This test is tricky because the reify flag is internal
    // We can test that events are processed normally when reify is not set
    int callback_count = 0;
    test_context_t ctx = { &callback_count, pipe_fds };
    
    ev_io watcher;
    ev_init(&watcher, io_cb);
    watcher.data = &ctx;
    
    ev_io_set(&watcher, pipe_fds[0], EV_READ);
    ev_io_start(loop, &watcher);
    
    // Write to trigger an event
    if (write(pipe_fds[1], "test", 4) != 4)
        die("write failed");
    
    // Run once to process the event
    ev_run(loop, EVRUN_ONCE);
    
    // Callback should be called
    if (callback_count != 1)
        die("callback should have been called once");
    
    // Now test ev_feed_fd_event directly
    callback_count = 0;
    ev_feed_fd_event(loop, pipe_fds[0], EV_READ);
    ev_run(loop, EVRUN_ONCE);
    
    // Callback should be called again
    if (callback_count != 1)
        die("callback should have been called once after ev_feed_fd_event");
    
    // Clean up
    ev_io_stop(loop, &watcher);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    ev_loop_destroy(loop);
}

static void test_ev_feed_fd_event(void) {
    struct ev_loop* loop = ev_loop_new(0);
    if (!loop)
        die("ev_loop_new failed");
    
    // Create local variables for this test
    int pipe_fds[2];
    int callback_count = 0;
    
    // Create a pipe
    if (pipe(pipe_fds) == -1)
        die("pipe creation failed");
    
    // Set up context
    test_context_t ctx = { &callback_count, pipe_fds };
    
    ev_io watcher;
    ev_init(&watcher, io_cb);
    watcher.data = &ctx;
    
    ev_io_set(&watcher, pipe_fds[0], EV_READ);
    ev_io_start(loop, &watcher);
    
    // Ensure the watcher is active
    if (!ev_is_active(&watcher)) {
        die("watcher should be active before feeding event");
    }
    
    // Check if the watcher's events include EV_READ
    if (!(watcher.events & EV_READ)) {
        die("watcher events should include EV_READ");
    }
    
    // Check if the events match what we're feeding
    if (!(watcher.events & EV_READ)) {
        die("watcher events don't include EV_READ, which we're feeding");
    }
    
    // Manually feed an FD event
    ev_feed_fd_event(loop, pipe_fds[0], EV_READ);
    
    // Check if there are pending events
    int pending_count = ev_pending_count(loop);
    if (pending_count == 0) {
        die("no pending events after ev_feed_fd_event");
    }
    
    // Check if our specific watcher is pending
    if (!ev_is_pending(&watcher)) {
        die("watcher should be pending after ev_feed_fd_event");
    }
    
    // Ensure the watcher is still active
    if (!ev_is_active(&watcher)) {
        die("watcher should be active after ev_feed_fd_event");
    }
    
    // Run the loop to process the event - use EVRUN_ONCE to ensure pending events are invoked
    ev_run(loop, EVRUN_ONCE);
    
    // The callback should have been called once
    if (callback_count != 1)
        die("callback should have been called exactly once after ev_feed_fd_event");
    
    // Clean up
    ev_io_stop(loop, &watcher);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    ev_loop_destroy(loop);
}

int main(void) {
    test_fd_event_and_reify();
    test_ev_feed_fd_event();
    test_reentrant_fd_modification();
    test_reify_flag_handling();
    
    printf("All FD bookkeeping tests passed!\n");
    return EXIT_SUCCESS;
}
