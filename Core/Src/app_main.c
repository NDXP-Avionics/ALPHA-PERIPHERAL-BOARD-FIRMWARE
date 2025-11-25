#include "stm32f103xb.h"
#include "app_main.h"

#include "Alpha.h"

int app_main()
{

    // initialize timers
    uint32_t last_t1 = HAL_GetTick();
    uint32_t last_t2 = HAL_GetTick();
    uint32_t last_t3 = HAL_GetTick();

    // initialize alpha
    Alpha A;

    ALPHA_STATE_INIT(&A);
    ALPHA_SENSORS_INIT(&A);
    ALPHA_COMMS_INIT(&A);

    HAL_GPIO_WritePin(PYRO1_GPIO_Port, PYRO1_Pin, 0);

    // Initialize state machine
    A.current_state = STATE_TESTING_FILL;
    A.mission_start_time = HAL_GetTick();
    A.high_rate_logging = 0;
    A.ignition_serial_received = 0;
    A.burn_wire_cut = 0;
    A.shutdown_timer_satisfied = 0;
    A.time_limit_exceeded = 0;
    
    while (1)
    {
        // Run state machine
        ALPHA_STATE_MACHINE(&A);
        // 100 Hz loop
        if (HAL_GetTick() - last_t1 > 10)
        {
            // reset timer
            last_t1 = HAL_GetTick();
            HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);

            // check rx
            ALPHA_RX(&A);
            //only send higgh rate data when logging is active 
            if (A.going&& A.high_rate_logging)
            {
                // Temp Sensors
                ALPHA_READ_TEMP(&A);
                Alpha_Send_100HZ(&A);
            }

            if (A.going)
            {
                // Pressure Sensors
                ALPHA_READ_PRESSURE(&A);
                Alpha_Send_100HZ(&A);
            }
        }

        // 10 Hz loop
        if (HAL_GetTick() - last_t2 > 100)
        {
            // reset timer
            last_t2 = HAL_GetTick();
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);

            if (A.going)
            {
                // read sensors
                ALPHA_READ_TEMP(&A);
                ALPHA_SEND_10HZ(&A);
            }
        }

        // 0.75 Hz loop
        if (HAL_GetTick() - last_t3 > 750)
        {
            // reset timer
            last_t3 = HAL_GetTick();

            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        }
    }

    return 0;
}
