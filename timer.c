#include "timer.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t timer_cv = PTHREAD_COND_INITIALIZER;
static tick_t current_tick = 0;

void timer_init(void) {
    // Nothing required for now — static inits are fine.
}

tick_t timer_now(void) {
    tick_t t;
    pthread_mutex_lock(&timer_lock);
    t = current_tick;
    pthread_mutex_unlock(&timer_lock);
    return t;
}

tick_t timer_tick(void) {
    pthread_mutex_lock(&timer_lock);
    current_tick += 1;
    tick_t t = current_tick;
    // Wake all sleepers (deterministic: call site controls when tick() happens)
    pthread_cond_broadcast(&timer_cv);
    pthread_mutex_unlock(&timer_lock);
    return t;
}

void timer_sleep_until(tick_t target) {
    pthread_mutex_lock(&timer_lock);
    while (current_tick < target) {
        pthread_cond_wait(&timer_cv, &timer_lock);
    }
    pthread_mutex_unlock(&timer_lock);
}

void timer_sleep_for(tick_t delta) {
    tick_t target = timer_now() + delta;
    timer_sleep_until(target);
}
