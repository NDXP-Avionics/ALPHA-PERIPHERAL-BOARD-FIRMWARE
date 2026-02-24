#include "state_machine.h"
#include "stm32f103xb.h" //for HAL_GetTick
#include "Alpha.h"

/*
=======================================================
STATE MACHINE VARIABLES START
=======================================================
*/

// all variables times 1e5; all values are stored this way to avoid floating point math */

// contact polarity
#define K1_INVERTED false
#define BW1_INVERTED false

// Critical pressures in psi
#define P1_CRITICAL 600 * 1e5
#define P2_CRITICAL 600 * 1e5
#define P3_CRITICAL 550 * 1e5
#define P4_CRITICAL 1350 * 1e5

#define P5_CRITICAL 500 * 1e5
#define P6_CRITICAL 500 * 1e5
#define P7_CRITICAL 500 * 1e5
#define P8_CRITICAL 500 * 1e5

// Burn end pressures in psi

#define P4_BURN_END 1040 * 1e5

// Critical Temps in F (Currently not being used)
#define T1_CRITICAL 300 * 1e5
#define T2_CRITICAL 300 * 1e5
#define T3_CRITICAL 300 * 1e5
#define T4_CRITICAL 300 * 1e5

// Safe to approach temps in F
#define T1_SAFE 150 * 1e5
#define T2_SAFE 150 * 1e5
#define T3_SAFE 150 * 1e5
#define T4_SAFE 150 * 1e5

// Burn Wire Time Limit in ms (time from fire command to ignition to call the launch an abort)
#define BURN_WIRE_TIME_LIMIT (5 * 1000) // 5 seconds

// BURN TIME in ms (time from burn wire breaking to cooldown sequence initiated)
#define BURN_TIME (5 * 1000) // 5 seconds

// COOLDOWN TIME in ms (max time to wait before switching from cooldown to standby state)
#define COOLDOWN_TIME (120 * 1000) // 120 seconds

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
        //Turn off all solenoids
          for (int i = 1; i <= 4; i++)
        {
            ALPHA_SET_SOLENOID(a, i, 0);
        }
        break;

    case FIRE_RECEIVED:
        // start logging data (Logging is handled by a separate python script, no data is saved on the MCU)
        break;

    case IGNITE:
        // fire pyro
        ALPHA_SET_PYRO(a, 1);
        break;

    case BURNING:
        // turn off pyro
        ALPHA_SET_PYRO(a, 0);
        // Open primary GOX/Ethanol Valves 
        ALPHA_SET_SOLENOID(a,3,1);
        ALPHA_SET_SOLENOID(a,4,1);
        break;

    case COOLDOWN:
        //Close SV1, SV3 and SV4 (Ethanol, Nitrous, and GOX Fuel Valves)
        ALPHA_SET_SOLENOID(a, 1, 0);
        ALPHA_SET_SOLENOID(a, 3, 0);
        ALPHA_SET_SOLENOID(a, 4, 0);

        //Open SV2 (Ethanol Vent Valve)
        ALPHA_SET_SOLENOID(a, 2, 1);    
        //Open 
        break;

    case ABORT:
       ALPHA_SET_SOLENOID(a, 1, 0); // Close Nitro valve
       ALPHA_SET_SOLENOID(a, 3, 0); // Close Fuel valve
       ALPHA_SET_SOLENOID(a, 2, 1); // Open vent valve
       ALPHA_SET_SOLENOID(a, 4, 0); // Close GOX valve
        // turn off solenoids
        
        // turn off pyro
        ALPHA_SET_PYRO(a, 0);

        // TODO: send error info??

        break;
    }
}

// definitions for leaving one mode to go to another, called in 100 hz loop. 
void SM_ADVANCE_STATE(Alpha *a)
{

    // for each mode, check exit condition and switch accordingly, otherwise stay in current mode
    switch (a->state)
    {
    // standby, normal mode
    case STANDBY:
        // This state is only exited upon recieving the fire command, which manually sets state to FIRE_RECIEVED.
        break;

    // received fire command
    case FIRE_RECEIVED:
        // If arm switch not continuous, abort 

        if ((a->k1 == K1_INVERTED))
        {
            SM_SET_STATE(a, ABORT);
            break;
        }
        // If burn wire not continuous, abort 
        if ((a->bw1 == BW1_INVERTED))
        {

            SM_SET_STATE(a, ABORT);
            break;
        }
        // Abort if system pressures are not nominal 
        if (!PRESSURES_NOMINAL(a))
        {
           
            SM_SET_STATE(a, ABORT);
            break;
        }

        // all checks pass, ignite
        SM_SET_STATE(a, IGNITE);

        // abort command handled by uart_rx.c
        break;

    // ignite starter motor
    case IGNITE:
        //Switch to burning if burn wire has been broken
        if ((a->bw1 == BW1_INVERTED))
        {
            SM_SET_STATE(a, BURNING);
            break;
        }

        // If the burn wire has not been broken after the BURN_WIRE_TIME_LIMIT is exceeded, abort. 
        if ((HAL_GetTick() - state_start) > BURN_WIRE_TIME_LIMIT)
        {
            SM_SET_STATE(a, ABORT);
        }


        break;

    case BURNING:

        // switch to cooldown if burn time exceeded
        if ((HAL_GetTick() - state_start) > BURN_TIME)
        {
            SM_SET_STATE(a, COOLDOWN);
            break;
        }

        //Check if GOX pressure is below burn end threshold, switch to cooldown if true 
        if (PLUMBING_BURN_END(a))
        {
            SM_SET_STATE(a, COOLDOWN);
        }

        // switch to abort if plumbing pressure is critical 
        if ((!PRESSURES_NOMINAL(a)) )
        {
            SM_SET_STATE(a, ABORT);
            break;
        }

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
        // ABORT can only by exited via a the RST serial command, which manually sets system state to standby.
        break;
    }
}

uint8_t PRESSURES_NOMINAL(Alpha *a)
{
    // return true;
    return (a->p1 < P1_CRITICAL) && (a->p2 < P2_CRITICAL) && (a->p3 < P3_CRITICAL) && (a->p4 < P4_CRITICAL) && (a->p5 < P5_CRITICAL) && (a->p6 < P6_CRITICAL) && (a->p7 < P7_CRITICAL) && (a->p8 < P8_CRITICAL);
}

uint8_t PLUMBING_BURN_END(Alpha *a)
{
    // return false;
    return (a->p4 < P4_BURN_END);
}

uint8_t TEMPS_NOMINAL(Alpha *a)
{
    // return true;
    return (a->temp_1 < T1_CRITICAL && a->temp_2 < T2_CRITICAL && a->temp_3 < T3_CRITICAL && a->temp_4 < T4_CRITICAL);
}

uint8_t TEMPS_SAFE(Alpha *a)
{
    return (a->temp_1 < T1_SAFE && a->temp_2 < T2_SAFE && a->temp_3 < T3_SAFE && a->temp_4 < T4_SAFE);
}
