#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "timer.h"

typedef struct {
    const char *name;
    tick_t wait;
} sleeper_arg;

void *sleeper(void *v) {
    sleeper_arg *arg = (sleeper_arg*)v;
    tick_t before = timer_now();
    printf("[%s] started at tick %llu, sleeping %llu ticks\n", arg->name, (unsigned long long)before, (unsigned long long)arg->wait);
    timer_sleep_for(arg->wait);
    tick_t after = timer_now();
    printf("[%s] woke at tick %llu (target was %llu)\n", arg->name, (unsigned long long)after, (unsigned long long)(before + arg->wait));
    return NULL;
}

int main(void) {
    timer_init();

    const int N = 3;
    pthread_t threads[N];
    sleeper_arg args[N];

    args[0].name = "A"; args[0].wait = 3;
    args[1].name = "B"; args[1].wait = 1;
    args[2].name = "C"; args[2].wait = 5;

    for (int i = 0; i < N; ++i) {
        if (pthread_create(&threads[i], NULL, sleeper, &args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Deterministically tick the timer and print the tick number
    for (int t = 1; t <= 6; ++t) {
        tick_t newt = timer_tick();
        printf("[main] tick -> %llu\n", (unsigned long long)newt);
        // No real-time sleeps: deterministic progression driven solely by calls to timer_tick
        usleep(10000); // small sleep to allow other threads to run (not required for determinism)
    }

    for (int i = 0; i < N; ++i) pthread_join(threads[i], NULL);

    printf("All sleepers done at tick %llu\n", (unsigned long long)timer_now());
    return 0;
}
