#ifndef __ENCODER_H
#define __ENCODER_H

#include <stdint.h>

// intended to be called from a timer interrupt service routine (ISR)
void velocity_update(void);

// Returns in ticks/second

int32_t velocity_get(void);

#endif /* __ENCODER_H */