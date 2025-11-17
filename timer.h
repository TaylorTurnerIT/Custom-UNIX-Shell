#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Initialize timer subsystem. Call once at program start. */
void timer_init(void);

/* Return current timer ticks (milliseconds since timer_init). */
int64_t timer_ticks(void);

/* Sleep current thread for given ticks (milliseconds). */
void timer_sleep(int64_t ticks);

#endif // TIMER_H
