#include "ev.h"
#include <assert.h>
#include <math.h>

static int timer_cb_count = 0;
static void timer_cb(struct ev_loop *loop, ev_timer *w, int revents) {
    (void)loop;
    (void)w;
    (void)revents;
    timer_cb_count++;
}

static void timer1_cb(struct ev_loop *loop, ev_timer *w, int revents) {
    (void)loop;
    (void)revents;
    int *cb_called = (int*)w->data;
    (*cb_called)++;
}

static void timer2_cb(struct ev_loop *loop, ev_timer *w, int revents) {
    (void)loop;
    (void)revents;
    int *cb_called = (int*)w->data;
    (*cb_called)++;
}

static void test_timer_basic(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    timer_cb_count = 0;
    
    ev_timer_init(&timer, timer_cb, 0.01, 0.0);
    ev_timer_start(loop, &timer);
    
    // Run until the timer fires
    ev_run(loop, 0);
    assert(timer_cb_count == 1);
    assert(!ev_is_active(&timer));
}

static void test_timer_repeat(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    timer_cb_count = 0;
    
    ev_timer_init(&timer, timer_cb, 0.01, 0.01);
    ev_timer_start(loop, &timer);
    
    // Run multiple times
    for (int i = 0; i < 3; i++) {
        ev_run(loop, EVRUN_ONCE);
        assert(timer_cb_count == i + 1);
        assert(ev_is_active(&timer));
    }
    
    ev_timer_stop(loop, &timer);
}

static void test_timer_again(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    timer_cb_count = 0;
    
    ev_timer_init(&timer, timer_cb, 0.01, 0.01);
    
    // Start the timer
    ev_timer_start(loop, &timer);
    
    // Let it fire once
    ev_run(loop, EVRUN_ONCE);
    assert(timer_cb_count == 1);
    
    // Reset using ev_timer_again
    ev_timer_again(loop, &timer);
    
    // Let it fire again
    ev_run(loop, EVRUN_ONCE);
    assert(timer_cb_count == 2);
    
    ev_timer_stop(loop, &timer);
}

static void test_timer_remaining(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    
    ev_timer_init(&timer, timer_cb, 0.05, 0.0);
    ev_timer_start(loop, &timer);
    
    ev_tstamp remaining = ev_timer_remaining(loop, &timer);
    assert(remaining > 0.0 && remaining <= 0.05);
    
    ev_timer_stop(loop, &timer);
}

static void test_timer_heap_ordering(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer1, timer2;
    int cb1_called = 0, cb2_called = 0;
    
    // Set up timers with different expiration times
    ev_timer_init(&timer1, timer1_cb, 0.02, 0.0);
    timer1.data = &cb1_called;
    ev_timer_init(&timer2, timer2_cb, 0.01, 0.0);
    timer2.data = &cb2_called;
    
    ev_timer_start(loop, &timer1);
    ev_timer_start(loop, &timer2);
    
    // Run once - timer2 should fire first due to earlier expiration
    ev_run(loop, EVRUN_ONCE);
    assert(cb2_called == 1);
    assert(cb1_called == 0);
    
    // Run again to fire timer1
    ev_run(loop, EVRUN_ONCE);
    assert(cb1_called == 1);
    
    ev_timer_stop(loop, &timer1);
    ev_timer_stop(loop, &timer2);
}

int main(void) {
    test_timer_basic();
    test_timer_repeat();
    test_timer_again();
    test_timer_remaining();
    test_timer_heap_ordering();
    return 0;
}
