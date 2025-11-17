#include "timer.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

/* Simple timer manager:
 * - Maintains a global ordered list of sleeping threads (by wake_tick).
 * - A background timer thread waits until the next wake time, then
 *   unparks (signals) all nodes whose wake_tick <= now.
 * - Waiting threads call `timer_sleep(ms)` and block on a per-node condvar.
 * This avoids busy-waiting and keeps wakes ordered.
 */

struct sleep_node {
    struct sleep_node *next;
    int64_t wake_tick; /* ms */
    pthread_cond_t cond;
    int signaled;
};

static struct sleep_node *sleep_list = NULL; /* head (earliest wake) */
static pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sleep_cond = PTHREAD_COND_INITIALIZER; /* timer thread wait */
static int64_t start_ms = 0;
static int timer_inited = 0;

static int64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int64_t timer_ticks(void) {
    if (!timer_inited) return 0;
    return now_ms() - start_ms;
}

static void insert_ordered(struct sleep_node *node) {
    if (!sleep_list || node->wake_tick < sleep_list->wake_tick) {
        node->next = sleep_list;
        sleep_list = node;
        return;
    }
    struct sleep_node *p = sleep_list;
    while (p->next && p->next->wake_tick <= node->wake_tick) p = p->next;
    node->next = p->next;
    p->next = node;
}

static void *timer_thread_func(void *arg) {
    (void)arg;
    pthread_mutex_lock(&sleep_mutex);
    while (1) {
        if (!sleep_list) {
            /* Nothing to do, wait until a sleeper is added */
            pthread_cond_wait(&sleep_cond, &sleep_mutex);
            continue;
        }

        int64_t now = now_ms() - start_ms;
        int64_t wake = sleep_list->wake_tick;
        if (wake > now) {
            /* Wait until the next wake time (timed wait) */
            int64_t wait_ms = wake - now;
            struct timespec abstime;
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += wait_ms / 1000;
            abstime.tv_nsec += (wait_ms % 1000) * 1000000;
            if (abstime.tv_nsec >= 1000000000) {
                abstime.tv_sec += 1;
                abstime.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&sleep_cond, &sleep_mutex, &abstime);
            continue;
        }

        /* Wake all nodes whose wake_tick <= now */
        while (sleep_list && sleep_list->wake_tick <= now) {
            struct sleep_node *n = sleep_list;
            sleep_list = n->next;
            n->next = NULL;
            n->signaled = 1;
            pthread_cond_signal(&n->cond);
            /* Do not destroy cond here; waiting thread will cleanup */
        }
    }
    pthread_mutex_unlock(&sleep_mutex);
    return NULL;
}

void timer_init(void) {
    if (timer_inited) return;
    start_ms = now_ms();
    pthread_t thr;
    if (pthread_create(&thr, NULL, timer_thread_func, NULL) != 0) {
        perror("timer thread create");
        exit(1);
    }
    pthread_detach(thr);
    timer_inited = 1;
}

void timer_sleep(int64_t ticks) {
    if (ticks <= 0) return;
    if (!timer_inited) timer_init();

    struct sleep_node node;
    node.next = NULL;
    node.wake_tick = timer_ticks() + ticks;
    node.signaled = 0;
    pthread_cond_init(&node.cond, NULL);

    pthread_mutex_lock(&sleep_mutex);
    insert_ordered(&node);
    /* Notify timer thread a new node exists (in case it was sleeping) */
    pthread_cond_signal(&sleep_cond);

    /* Wait until signaled by timer thread */
    while (!node.signaled) {
        pthread_cond_wait(&node.cond, &sleep_mutex);
    }
    /* Done waiting; remove cond */
    pthread_mutex_unlock(&sleep_mutex);
    pthread_cond_destroy(&node.cond);
}
