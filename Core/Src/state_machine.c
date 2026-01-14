#include "state_machine.h"
#include "stm32f103xb.h" //for HAL_GetTick
#include "Alpha.h"

/*
=======================================================
STATE MACHINE VARIABLES START
=======================================================
*/

// contact polarity
#define K1_INVERTED false
#define BW1_INVERTED false

// Critical pressures in psi
#define P1_CRITICAL 500
#define P2_CRITICAL 500
#define P3_CRITICAL 500
#define P4_CRITICAL 500
#define P5_CRITICAL 500
#define P6_CRITICAL 500
#define P7_CRITICAL 500
#define P8_CRITICAL 500

// Critical Temps in F
#define T1_CRITICAL 300
#define T2_CRITICAL 300
#define T3_CRITICAL 300
#define T4_CRITICAL 300

// Safe to approach temps in F
#define T1_SAFE 150
#define T2_SAFE 150
#define T3_SAFE 150
#define T4_SAFE 150

// Burn Wire Time Limit in ms (time from fire command to ignition to call the launch an abort)
#define BURN_WIRE_TIME_LIMIT (5 * 1000) // 5 seconds

// BURN TIME in ms (time from burn wire breaking to cooldown sequence initiated)
#define BURN_TIME (20 * 1000) // 20 seconds

// COOLDOWN TIME in ms (max time to wait before switching from cooldown to standby state)
#define COOLDOWN_TIME (20 * 1000) // 120 seconds

/*
=======================================================
STATE MACHINE VARIABLES END
=======================================================
*/

// state_start time variable
static uint32_t state_start = 0;

// definitions for "upon entry" actions
void SM_SET_STATE(Alpha *a, STATE m)
{

    // reset state start time each time a new state is entered.
    state_start = HAL_GetTick();

    // set state to m
    a->state = m;

    // what happens when we enter these modes?
    switch (m)
    {
    case STANDBY:
        // stop logging data
        // enable all services??

        break;

    case FIRE_RECEIVED:
        // start logging data -> we can handle this using states in python
        break;

    case IGNITE:
        // fire pyro
        ALPHA_SET_PYRO(a, 1);
        break;

    case BURNING:
        // turn off pyro
        ALPHA_SET_PYRO(a, 0);
        // open solenoids -> on timer?? we have 0.01 second resolution. doing all at once for now
        for (int i = 1; i <= 4; i++)
        {
            ALPHA_SET_SOLENOID(a, i, 1);
        }

        break;

    case COOLDOWN:
        // close solenoids
        for (int i = 1; i <= 4; i++)
        {
            ALPHA_SET_SOLENOID(a, i, 0);
        }

        break;

    case ABORT:
        // stop logging data
        // turn off solenoids
        for (int i = 0; i < 4; i++)
        {
            ALPHA_SET_SOLENOID(a, i, 0);
        }
        // turn off pyro
        ALPHA_SET_PYRO(a, 0);

        // TODO: send error info??

        break;
    }
}

// definitions for leaving one mode to go to another, ** to be called frequently ~100hz **
void SM_ADVANCE_STATE(Alpha *a)
{

    // for each mode, check exit condition and switch accordingly, otherwise stay in current mode
    switch (a->state)
    {
    // standby, normal mode
    case STANDBY:
        // receive fire command handled by uart_rx.c; nothing to do here
        break;

    // received fire command
    case FIRE_RECEIVED:
        // check keyswitches turned and plumbing pressures nominal -> switch to ignite otherwise abort
        if ((a->k1 == K1_INVERTED))
        {
            // k1 not correct, abort
            SM_SET_STATE(a, ABORT);
            break;
        }
        if ((a->bw1 == BW1_INVERTED))
        {
            // bw1 lacks continuity, abort
            SM_SET_STATE(a, ABORT);
            break;
        }
        // check plumbing pressures nominal
        if (!PLUMBING_NOMINAL(a))
        {
            // plumbing not nominal, abort
            SM_SET_STATE(a, ABORT);
            break;
        }

        // all checks pass, ignite
        SM_SET_STATE(a, IGNITE);

        // abort command handled by uart_rx.c
        break;

    // ignite starter motor
    case IGNITE:
        // switch to burning -> Burn Wire separated
        if ((a->bw1 == BW1_INVERTED))
        {
            // bw1 lacks continuity, switch to burning
            SM_SET_STATE(a, BURNING);
            break;
        }

        // switch to abort -> check burn wire time limit exceeded
        if ((HAL_GetTick() - state_start) > BURN_WIRE_TIME_LIMIT)
        {
            SM_SET_STATE(a, ABORT);
        }

        // abort command handled by uart_rx.c
        break;

    case BURNING:

        // switch to cooldown if burn time exceeded
        if ((HAL_GetTick() - state_start) > BURN_TIME)
        {
            SM_SET_STATE(a, COOLDOWN);
            break;
        }

        // switch to abort if plumbing pressure or temps critical
        if ((!PLUMBING_NOMINAL(a)) || (!TEMPS_NOMINAL(a)))
        {
            SM_SET_STATE(a, ABORT);
            break;
        }

        // abort command handled by uart_rx.c
        break;

    case COOLDOWN:
        // if cooldown time is exceeded, switch to standby
        if ((HAL_GetTick() - state_start) > COOLDOWN_TIME)
        {
            SM_SET_STATE(a, STANDBY);
            break;
        }

        // if temps safe, switch to standby
        if (TEMPS_SAFE(a))
        {
            SM_SET_STATE(a, STANDBY);
            break;
        }

        // abort command handled by uart_rx.c
        break;

    case ABORT:
        // switch to standby upon recieved reset command -> handled in uart_rx.c; nothing to do here
        break;
    }
}

//**IMPORTANT TODO, make sure these are actually translated to psi at this point */
uint8_t PLUMBING_NOMINAL(Alpha *a)
{
    return true;
    return (a->p1 < P1_CRITICAL) && (a->p2 < P2_CRITICAL) && (a->p3 < P3_CRITICAL) && (a->p4 < P4_CRITICAL) && (a->p5 < P5_CRITICAL) && (a->p6 < P6_CRITICAL) && (a->p7 < P7_CRITICAL) && (a->p8 < P8_CRITICAL);
}

//**IMPORTANT TODO, make sure these are actually translated to degrees F at this point */
uint8_t TEMPS_NOMINAL(Alpha *a)
{
    return true;
    return (a->temp_1 < T1_CRITICAL && a->temp_2 < T2_CRITICAL && a->temp_3 < T3_CRITICAL && a->temp_4 < T4_CRITICAL);
}

//**IMPORTANT TODO, make sure these are actually translated to degrees F at this point */
uint8_t TEMPS_SAFE(Alpha *a)
{
    return (a->temp_1 < T1_SAFE && a->temp_2 < T2_SAFE && a->temp_3 < T3_SAFE && a->temp_4 < T4_SAFE);
}
