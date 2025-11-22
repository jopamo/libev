#include "ev.h"
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

static void io_cb(struct ev_loop *loop, ev_io *w, int revents) {
    (void)loop;
    (void)revents;
    int *cb_called = (int*)w->data;
    *cb_called += 1;
}

static void test_io_start_stop_basic(void) {
    struct ev_loop *loop = ev_default_loop(0);
    int fds[2];
    int cb_called = 0;
    ev_io io_watcher;
    
    // Create a pipe for testing
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    
    // Set non-blocking
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    fcntl(fds[1], F_SETFL, O_NONBLOCK);
    
    ev_io_init(&io_watcher, io_cb, fds[0], EV_READ);
    io_watcher.data = &cb_called;
    
    // Start the watcher
    ev_io_start(loop, &io_watcher);
    assert(ev_is_active(&io_watcher));
    
    // Write to trigger the event
    write(fds[1], "x", 1);
    
    // Run once to process the event
    ev_run(loop, EVRUN_ONCE);
    assert(cb_called == 1);
    
    // Stop the watcher
    ev_io_stop(loop, &io_watcher);
    assert(!ev_is_active(&io_watcher));
    
    close(fds[0]);
    close(fds[1]);
}

static void test_io_multiple_watchers_same_fd(void) {
    struct ev_loop *loop = ev_default_loop(0);
    int fds[2];
    int cb1_called = 0, cb2_called = 0;
    ev_io io_watcher1, io_watcher2;
    
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    fcntl(fds[1], F_SETFL, O_NONBLOCK);
    
    // Set up two watchers on the same fd
    ev_io_init(&io_watcher1, io_cb, fds[0], EV_READ);
    io_watcher1.data = &cb1_called;
    ev_io_init(&io_watcher2, io_cb, fds[0], EV_READ);
    io_watcher2.data = &cb2_called;
    
    ev_io_start(loop, &io_watcher1);
    ev_io_start(loop, &io_watcher2);
    
    // Trigger event
    write(fds[1], "x", 1);
    
    ev_run(loop, EVRUN_ONCE);
    assert(cb1_called == 1);
    assert(cb2_called == 1);
    
    ev_io_stop(loop, &io_watcher1);
    ev_io_stop(loop, &io_watcher2);
    
    close(fds[0]);
    close(fds[1]);
}

static void test_io_event_mask_clamping(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_io io_watcher;
    int fds[2];
    
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    
    // Test that invalid bits are clamped
    // According to the implementation, it should clamp to EV_READ | EV_WRITE
    ev_io_init(&io_watcher, io_cb, fds[0], EV_READ | 0xFFFF0000);
    // This should not crash and the implementation should handle it
    ev_io_start(loop, &io_watcher);
    // Check that the events field was clamped
    assert((io_watcher.events & ~(EV_READ | EV_WRITE)) == 0);
    ev_io_stop(loop, &io_watcher);
    
    close(fds[0]);
    close(fds[1]);
}

static void test_io_ev_iofdset_bookkeeping(void) {
    struct ev_loop *loop = ev_default_loop(0);
    int fds[2];
    ev_io io_watcher;
    int cb_called = 0;
    
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    
    ev_io_init(&io_watcher, io_cb, fds[0], EV_READ);
    io_watcher.data = &cb_called;
    
    // Start and stop to ensure proper bookkeeping
    ev_io_start(loop, &io_watcher);
    assert(ev_is_active(&io_watcher));
    
    // The fd should be tracked in the anfds array
    // This is implementation-specific, but we can at least verify no crashes
    ev_io_stop(loop, &io_watcher);
    assert(!ev_is_active(&io_watcher));
    
    close(fds[0]);
    close(fds[1]);
}

int main(void) {
    test_io_start_stop_basic();
    test_io_multiple_watchers_same_fd();
    test_io_event_mask_clamping();
    test_io_ev_iofdset_bookkeeping();
    return 0;
}
