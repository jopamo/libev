#include "ev.h"
#include <assert.h>

static int periodic_cb_count = 0;
static void periodic_cb(struct ev_loop *loop, ev_periodic *w, int revents) {
    (void)loop;
    (void)w;
    (void)revents;
    periodic_cb_count++;
}

static ev_tstamp reschedule_cb(ev_periodic *w, ev_tstamp now) {
    return now + 0.1;
}

static void test_periodic_basic(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_periodic periodic;
    periodic_cb_count = 0;
    
    ev_periodic_init(&periodic, periodic_cb, 0.0, 0.1, 0);
    ev_periodic_start(loop, &periodic);
    
    // Run to process the periodic
    ev_run(loop, EVRUN_ONCE);
    assert(periodic_cb_count >= 1);
    
    ev_periodic_stop(loop, &periodic);
}

static void test_periodic_reschedule_cb(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_periodic periodic;
    periodic_cb_count = 0;
    
    ev_periodic_init(&periodic, periodic_cb, 0.0, 0.0, reschedule_cb);
    ev_periodic_start(loop, &periodic);
    
    ev_run(loop, EVRUN_ONCE);
    assert(periodic_cb_count >= 1);
    
    ev_periodic_stop(loop, &periodic);
}

static void test_periodic_again(void) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_periodic periodic;
    periodic_cb_count = 0;
    
    ev_periodic_init(&periodic, periodic_cb, 0.0, 0.1, 0);
    ev_periodic_start(loop, &periodic);
    
    // Let it fire
    ev_run(loop, EVRUN_ONCE);
    int count_before = periodic_cb_count;
    
    // Call again
    ev_periodic_again(loop, &periodic);
    
    // Let it fire again
    ev_run(loop, EVRUN_ONCE);
    assert(periodic_cb_count > count_before);
    
    ev_periodic_stop(loop, &periodic);
}

int main(void) {
    test_periodic_basic();
    test_periodic_reschedule_cb();
    test_periodic_again();
    return 0;
}
