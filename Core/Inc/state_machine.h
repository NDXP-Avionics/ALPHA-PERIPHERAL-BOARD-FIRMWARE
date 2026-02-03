
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

typedef struct Alpha Alpha;

typedef enum STATE
{
    STANDBY,
    FIRE_RECEIVED,
    IGNITE,
    BURNING,
    COOLDOWN,
    ABORT,
} STATE;

void SM_SET_STATE(Alpha *a, STATE m);

void SM_ADVANCE_STATE(Alpha *a);

uint8_t PLUMBING_NOMINAL(Alpha *a);

uint8_t PLUMBING_BURN_END(Alpha *a);

uint8_t TEMPS_NOMINAL(Alpha *a);

uint8_t TEMPS_SAFE(Alpha *a);

#endif
