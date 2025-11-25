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

    // wait to init comms
    ALPHA_COMMS_INIT(&A);

    while (1)
    {

        // 100 Hz loop
        if (HAL_GetTick() - last_t1 > 10)
        {
            // reset timer
            last_t1 = HAL_GetTick();
            HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);

            // check rx
            ALPHA_RX(&A);

            if (A.going)
            {
                // Pressure Sensors
                ALPHA_READ_PRESSURE(&A);
                ALPHA_READ_LOADCELL(&A);
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
                // Read sensors
                ALPHA_READ_TEMP(&A);
                // Send Data
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
