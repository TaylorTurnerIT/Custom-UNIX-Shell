#include "timer.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/* Deterministic tick-based timer.
 * - `timer_tick()` advances the global tick by one and wakes sleepers.
 * - Sleepers call `timer_sleep_until(target)` or `timer_sleep_for(delta)`
 *   and block on a condition variable until `timer_tick()` advances the tick
 *   to the requested target. This avoids busy-waiting and makes wakes
 *   deterministic because progression occurs only when the test driver
 *   calls `timer_tick()`.
 */

static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t timer_cv = PTHREAD_COND_INITIALIZER;
static tick_t current_tick = 0;
static int64_t start_ms = 0;

static int64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void timer_init(void) {
    /* Initialize start time for real-time timer functions. */
    if (start_ms == 0) start_ms = now_ms();
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
    /* Wake all sleepers: deterministic because tests call timer_tick(). */
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

/* --- Millisecond-based real-time API (used by shell builtin sleep) --- */
int64_t timer_ticks(void) {
    if (start_ms == 0) return 0;
    return now_ms() - start_ms;
}

void timer_sleep(int64_t ticks) {
    if (ticks <= 0) return;
    struct timespec req, rem;
    req.tv_sec = ticks / 1000;
    req.tv_nsec = (ticks % 1000) * 1000000;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
            continue;
        }
        break;
    }
}
