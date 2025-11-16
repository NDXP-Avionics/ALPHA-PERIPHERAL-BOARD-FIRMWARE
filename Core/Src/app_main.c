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

    ALPHA_SENSORS_INIT(&A);
    ALPHA_COMMS_INIT(&A);

    while (1)
    {

        // 20 Hz loop
        if (HAL_GetTick() - last_t1 > 50)
        {
            // reset timer
            last_t1 = HAL_GetTick();

            HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
        }

        // 10 Hz loop
        if (HAL_GetTick() - last_t2 > 100)
        {
            // reset timer
            last_t2 = HAL_GetTick();
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);

            // read sensors
            ALPHA_READ_TEMP(&A);

            ALPHA_SEND_10HZ(&A);
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
