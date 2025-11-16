#include "stm32f103xb.h"
#include "app_main.h"

#include "dmacirc.h"
#include "MAX31856.h"

void tc1_cs_low(void)
{
    HAL_GPIO_WritePin(TC4_CS_GPIO_Port, TC4_CS_Pin, GPIO_PIN_RESET);
}

void tc1_cs_high(void)
{
    HAL_GPIO_WritePin(TC4_CS_GPIO_Port, TC4_CS_Pin, GPIO_PIN_SET);
}

// Grab Handles
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

int app_main()
{

    // initialize timers
    uint32_t last_t1 = HAL_GetTick();
    uint32_t last_t2 = HAL_GetTick();
    uint32_t last_t3 = HAL_GetTick();

    // initialize TC
    MAX31856_t tc1;
    tc1.cs_high = tc1_cs_high;
    tc1.cs_low = tc1_cs_low;
    tc1.spi_tx = HAL_SPI_Transmit;
    tc1.spi_rx = HAL_SPI_Receive;
    tc1.s = &hspi1;

    max31856_init(&tc1);

    // set thermocouple type
    max31856_write_register(&tc1, 0x01, 0b00000011);

    // start conversion
    max31856_write_register(&tc1, 0x00, 0b10010000);

    // initialize DMA
    dmasendinit(&huart1);

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

            /*
    // read data
    uint32_t data = 0;
    data = max31856_read_thermocouple(&tc1);

    xp_packet_t pkt;
    pkt.data = (uint64_t)data;
    pkt.end_byte = 0x00;
    pkt.sender_id = 0x33;
    pkt.type = TEMP;
    uint8_t packet[12];
    XPLINK_PACK(packet, &pkt);
    */
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
