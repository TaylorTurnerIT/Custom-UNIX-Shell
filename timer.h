#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

typedef uint64_t tick_t;

// Initialize timer (optional)
void timer_init(void);

// Return current tick
tick_t timer_now(void);

// Advance the timer by one tick and wake sleepers. Returns the new tick.
tick_t timer_tick(void);

// Sleep until specified absolute tick (blocks the calling thread)
void timer_sleep_until(tick_t target);

// Sleep for given number of ticks (relative)
void timer_sleep_for(tick_t delta);
/* Initialize timer subsystem. Call once at program start. */
void timer_init(void);

/* Return current timer ticks (milliseconds since timer_init). */
int64_t timer_ticks(void);

/* Sleep current thread for given ticks (milliseconds). */
void timer_sleep(int64_t ticks);

#endif // TIMER_H
