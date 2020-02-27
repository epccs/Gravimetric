#ifndef TimersTick_h
#define TimersTick_h

#define MICROSEC_TICK_CORRECTION (( (64 * 256) / ( F_CPU / 1000000UL ) ) % 1000UL)

extern void initTimers(void);
extern uint32_t tickAtomic(void);
extern unsigned long milliseconds(void);
unsigned long elapsed(unsigned long *);

#endif // TimersTick_h
