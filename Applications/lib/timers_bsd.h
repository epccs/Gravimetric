#ifndef TimersTick_h
#define TimersTick_h

// somewhat corrected time
#define MILLIS_TO_TICK_CORRECTION(t) ( (t) - ( t * (64 * 256) / ( F_CPU / 1000000UL ) / 1000UL ) )
#define MILLIS_TO_TICK(t) ( (t) - MILLIS_TO_TICK_CORRECTION(t) )


extern void initTimers(void);
extern uint32_t tickAtomic(void);
extern unsigned long milliseconds(void);
unsigned long elapsed(unsigned long *);

#endif // TimersTick_h
